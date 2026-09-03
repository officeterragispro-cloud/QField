#include "offlineprojectexporter.h"

#include "offlineexportqueue.h"
#include "qffileutils.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QSet>
#include <QVariantMap>
#include <QUuid>

#include <utility>

#include <qgsziputils.h>
#include <qgsmaplayer.h>
#include <qgsproject.h>
#include <qgsproviderregistry.h>

using namespace Tgp;

namespace
{
  bool copyFileChecked( const QString &source, const QString &destination, QString *error )
  {
    const QFileInfo sourceInfo( source );
    if ( !sourceInfo.isFile() )
    {
      if ( error ) *error = QObject::tr( "Missing source file: %1" ).arg( source );
      return false;
    }
    QDir().mkpath( QFileInfo( destination ).absolutePath() );
    QFile::remove( destination );
    if ( !QFile::copy( source, destination ) )
    {
      if ( error ) *error = QObject::tr( "Could not copy %1" ).arg( source );
      return false;
    }
    return true;
  }
}

OfflineProjectExporter::OfflineProjectExporter( const QString &storageDirectory, QObject *parent )
  : QObject( parent )
  , mStorageDirectory( storageDirectory )
  , mQueue( std::make_unique<OfflineExportQueue>( QDir( storageDirectory ).filePath( QStringLiteral( "queue" ) ), this ) )
{
  connect( mQueue.get(), &OfflineExportQueue::changed, this, &OfflineProjectExporter::jobsChanged );
}

OfflineProjectExporter::~OfflineProjectExporter() = default;

QString OfflineProjectExporter::storageDirectory() const
{
  return mStorageDirectory;
}

OfflineExportQueue *OfflineProjectExporter::queue() const
{
  return mQueue.get();
}

QVariantList OfflineProjectExporter::jobs() const
{
  QVariantList result;
  for ( const OfflineExportJob &job : mQueue->jobs() )
  {
    result.append( QVariantMap {
      { QStringLiteral( "jobId" ), job.jobId },
      { QStringLiteral( "projectId" ), job.projectId },
      { QStringLiteral( "archivePath" ), job.archivePath },
      { QStringLiteral( "remotePath" ), job.remotePath },
      { QStringLiteral( "size" ), job.size },
      { QStringLiteral( "createdAt" ), job.createdAt },
      { QStringLiteral( "attempts" ), job.attempts },
      { QStringLiteral( "state" ), exportStateName( job.state ) },
      { QStringLiteral( "lastError" ), job.lastError }
    } );
  }
  return result;
}

int OfflineProjectExporter::pendingCount() const
{
  return mQueue->pendingJobs().size();
}

void OfflineProjectExporter::enqueueCurrentProject( QObject *projectObject, const QString &projectId, const QString &workspaceId, bool keepLocalCopy )
{
  QgsProject *project = qobject_cast<QgsProject *>( projectObject );
  if ( !project || project->fileName().isEmpty() )
  {
    emit exportStateChanged( {}, exportStateName( ExportState::Failed ), tr( "Save or open a QGIS project before exporting." ) );
    return;
  }

  OfflineExportRequest request;
  request.projectId = projectId.trimmed().isEmpty() ? QFileInfo( project->fileName() ).completeBaseName() : projectId.trimmed();
  request.workspaceId = workspaceId.trimmed().isEmpty() ? QStringLiteral( "default" ) : workspaceId.trimmed();
  request.qgisProject = QUrl::fromLocalFile( project->fileName() );
  request.keepLocalCopy = keepLocalCopy;

  QSet<QString> geopackagePaths;
  const auto layers = project->mapLayers();
  for ( QgsMapLayer *layer : layers )
  {
    if ( !layer ) continue;
    const QVariantMap decoded = QgsProviderRegistry::instance()->decodeUri( layer->providerType(), layer->source() );
    QString path = decoded.value( QStringLiteral( "path" ) ).toString();
    if ( path.isEmpty() ) path = layer->source().section( QLatin1Char( '|' ), 0, 0 );
    if ( QFileInfo( path ).suffix().compare( QLatin1String( "gpkg" ), Qt::CaseInsensitive ) == 0 )
      geopackagePaths.insert( QFileInfo( path ).absoluteFilePath() );
  }
  for ( const QString &path : std::as_const( geopackagePaths ) )
    request.geopackages.append( QUrl::fromLocalFile( path ) );

  const QString attachments = QDir( project->homePath() ).filePath( QStringLiteral( "attachments" ) );
  if ( QFileInfo::exists( attachments ) ) request.attachmentsDirectory = QUrl::fromLocalFile( attachments );
  enqueueExport( request );
}

void OfflineProjectExporter::enqueueExport( const OfflineExportRequest &request )
{
  const QString jobId = QUuid::createUuid().toString( QUuid::WithoutBraces );
  const QString timestamp = QDateTime::currentDateTimeUtc().toString( QStringLiteral( "yyyyMMddTHHmmssZ" ) );
  const QString jobDirectory = QDir( mStorageDirectory ).filePath( QStringLiteral( "exports/%1" ).arg( jobId ) );
  const QString stagingDirectory = QDir( jobDirectory ).filePath( QStringLiteral( "staging" ) );
  QDir().mkpath( QDir( stagingDirectory ).filePath( QStringLiteral( "data" ) ) );

  OfflineExportJob job;
  job.jobId = jobId;
  job.projectId = request.projectId;
  job.createdAt = QDateTime::currentDateTimeUtc().toString( Qt::ISODateWithMs );
  job.keepLocalCopy = request.keepLocalCopy;
  job.state = ExportState::Preparing;

  QString error;
  const QString projectSource = request.qgisProject.toLocalFile();
  const QString projectSuffix = QFileInfo( projectSource ).suffix().toLower();
  if ( projectSuffix != QLatin1String( "qgz" ) && projectSuffix != QLatin1String( "qgs" ) )
    error = tr( "The project must be a .qgz or .qgs file." );

  const QString stagedProject = QDir( stagingDirectory ).filePath( QStringLiteral( "project.%1" ).arg( projectSuffix ) );
  if ( error.isEmpty() ) copyFileChecked( projectSource, stagedProject, &error );

  QJsonArray geopackageEntries;
  QStringList archiveFiles;
  if ( error.isEmpty() ) archiveFiles.append( stagedProject );
  for ( const QUrl &url : request.geopackages )
  {
    if ( !error.isEmpty() ) break;
    const QString source = url.toLocalFile();
    if ( QFileInfo( source ).suffix().compare( QLatin1String( "gpkg" ), Qt::CaseInsensitive ) != 0 )
    {
      error = tr( "Not a GeoPackage: %1" ).arg( source );
      break;
    }
    const QString relative = QStringLiteral( "data/%1" ).arg( QFileInfo( source ).fileName() );
    const QString destination = QDir( stagingDirectory ).filePath( relative );
    if ( !copyFileChecked( source, destination, &error ) ) break;
    archiveFiles.append( destination );
    geopackageEntries.append( relative );
  }

  const QString attachmentsSource = request.attachmentsDirectory.toLocalFile();
  const QString attachmentsDestination = QDir( stagingDirectory ).filePath( QStringLiteral( "attachments" ) );
  if ( error.isEmpty() && !attachmentsSource.isEmpty() && QFileInfo::exists( attachmentsSource ) )
  {
    if ( !QfFileUtils::copyRecursively( attachmentsSource, attachmentsDestination, nullptr, true ) )
      error = tr( "Could not snapshot attachments." );
    else
    {
      QDirIterator iterator( attachmentsDestination, QDir::Files, QDirIterator::Subdirectories );
      while ( iterator.hasNext() ) archiveFiles.append( iterator.next() );
    }
  }

  const QJsonObject manifest {
    { QStringLiteral( "schemaVersion" ), 1 },
    { QStringLiteral( "projectId" ), request.projectId },
    { QStringLiteral( "workspaceId" ), request.workspaceId },
    { QStringLiteral( "createdAt" ), job.createdAt },
    { QStringLiteral( "qgisProject" ), QFileInfo( stagedProject ).fileName() },
    { QStringLiteral( "geopackages" ), geopackageEntries },
    { QStringLiteral( "attachments" ), QStringLiteral( "attachments/" ) }
  };
  const QString manifestPath = QDir( stagingDirectory ).filePath( QStringLiteral( "manifest.json" ) );
  QFile manifestFile( manifestPath );
  if ( error.isEmpty() && ( !manifestFile.open( QIODevice::WriteOnly ) || manifestFile.write( QJsonDocument( manifest ).toJson( QJsonDocument::Indented ) ) < 0 ) )
    error = tr( "Could not write export manifest." );
  manifestFile.close();
  archiveFiles.append( manifestPath );

  const QString archiveName = QStringLiteral( "%1_%2.tgpfield.zip" ).arg( request.projectId, timestamp );
  job.archivePath = QDir( jobDirectory ).filePath( archiveName );
  job.remotePath = QStringLiteral( "/TGP-FIELD/%1/%2/exports/%3/%4" ).arg( request.workspaceId, request.projectId, jobId, archiveName );

  if ( error.isEmpty() && !QgsZipUtils::zip( job.archivePath, archiveFiles ) )
    error = tr( "Could not create TGP-FIELD archive." );

  if ( error.isEmpty() )
  {
    job.sha256 = QfFileUtils::fileChecksum( job.archivePath, QCryptographicHash::Sha256 );
    job.size = QFileInfo( job.archivePath ).size();
    if ( job.sha256.isEmpty() ) error = tr( "Could not calculate archive checksum." );
  }

  if ( !error.isEmpty() )
  {
    job.state = ExportState::Failed;
    job.lastError = error;
    mQueue->upsert( job );
    emit exportStateChanged( jobId, exportStateName( job.state ), error );
    return;
  }

  job.state = ExportState::WaitingForNetwork;
  if ( !mQueue->upsert( job, &error ) )
  {
    emit exportStateChanged( jobId, exportStateName( ExportState::Failed ), error );
    return;
  }
  emit exportQueued( jobId, QUrl::fromLocalFile( job.archivePath ) );
  emit exportStateChanged( jobId, exportStateName( job.state ), tr( "Export queued for MEGA." ) );
}

void OfflineProjectExporter::resumePendingExports()
{
  QString error;
  if ( !mQueue->reload( &error ) )
    emit exportStateChanged( {}, exportStateName( ExportState::Failed ), error );
  else
    emit jobsChanged();
}

void OfflineProjectExporter::cancelExport( const QString &jobId )
{
  for ( OfflineExportJob job : mQueue->jobs() )
  {
    if ( job.jobId != jobId ) continue;
    job.state = ExportState::Cancelled;
    mQueue->upsert( job );
    emit exportStateChanged( jobId, exportStateName( job.state ), tr( "Export cancelled." ) );
    return;
  }
}
