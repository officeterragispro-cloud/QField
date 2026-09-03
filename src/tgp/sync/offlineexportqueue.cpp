#include "offlineexportqueue.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <utility>

using namespace Tgp;

namespace
{
  QJsonObject toJson( const OfflineExportJob &job )
  {
    return {
      { QStringLiteral( "schemaVersion" ), 1 },
      { QStringLiteral( "jobId" ), job.jobId },
      { QStringLiteral( "projectId" ), job.projectId },
      { QStringLiteral( "archivePath" ), job.archivePath },
      { QStringLiteral( "remotePath" ), job.remotePath },
      { QStringLiteral( "sha256" ), QString::fromLatin1( job.sha256.toHex() ) },
      { QStringLiteral( "size" ), static_cast<double>( job.size ) },
      { QStringLiteral( "createdAt" ), job.createdAt },
      { QStringLiteral( "attempts" ), job.attempts },
      { QStringLiteral( "state" ), exportStateName( job.state ) },
      { QStringLiteral( "lastError" ), job.lastError },
      { QStringLiteral( "keepLocalCopy" ), job.keepLocalCopy }
    };
  }

  OfflineExportJob fromJson( const QJsonObject &object )
  {
    OfflineExportJob job;
    job.jobId = object.value( QStringLiteral( "jobId" ) ).toString();
    job.projectId = object.value( QStringLiteral( "projectId" ) ).toString();
    job.archivePath = object.value( QStringLiteral( "archivePath" ) ).toString();
    job.remotePath = object.value( QStringLiteral( "remotePath" ) ).toString();
    job.sha256 = QByteArray::fromHex( object.value( QStringLiteral( "sha256" ) ).toString().toLatin1() );
    job.size = object.value( QStringLiteral( "size" ) ).toInteger();
    job.createdAt = object.value( QStringLiteral( "createdAt" ) ).toString();
    job.attempts = object.value( QStringLiteral( "attempts" ) ).toInt();
    job.state = exportStateFromName( object.value( QStringLiteral( "state" ) ).toString() );
    job.lastError = object.value( QStringLiteral( "lastError" ) ).toString();
    job.keepLocalCopy = object.value( QStringLiteral( "keepLocalCopy" ) ).toBool( true );
    return job;
  }
}

OfflineExportQueue::OfflineExportQueue( const QString &storageDirectory, QObject *parent )
  : QObject( parent )
  , mStorageDirectory( storageDirectory )
{
  reload();
}

QString OfflineExportQueue::queueFilePath() const
{
  return QDir( mStorageDirectory ).filePath( QStringLiteral( "offline-export-queue.json" ) );
}

QList<OfflineExportJob> OfflineExportQueue::jobs() const
{
  return mJobs;
}

QList<OfflineExportJob> OfflineExportQueue::pendingJobs() const
{
  QList<OfflineExportJob> result;
  for ( const OfflineExportJob &job : mJobs )
  {
    if ( job.state != ExportState::Complete && job.state != ExportState::Cancelled )
      result.append( job );
  }
  return result;
}

bool OfflineExportQueue::reload( QString *error )
{
  mJobs.clear();
  QFile file( queueFilePath() );
  if ( !file.exists() ) return true;
  if ( !file.open( QIODevice::ReadOnly ) )
  {
    if ( error ) *error = file.errorString();
    return false;
  }
  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson( file.readAll(), &parseError );
  if ( parseError.error != QJsonParseError::NoError || !document.isArray() )
  {
    if ( error ) *error = parseError.errorString();
    return false;
  }
  for ( const QJsonValue &value : document.array() )
  {
    const OfflineExportJob job = fromJson( value.toObject() );
    if ( !job.jobId.isEmpty() ) mJobs.append( job );
  }
  return true;
}

bool OfflineExportQueue::save( QString *error )
{
  if ( !QDir().mkpath( mStorageDirectory ) )
  {
    if ( error ) *error = tr( "Could not create export queue directory." );
    return false;
  }
  QJsonArray array;
  for ( const OfflineExportJob &job : std::as_const( mJobs ) ) array.append( toJson( job ) );
  QSaveFile file( queueFilePath() );
  if ( !file.open( QIODevice::WriteOnly ) )
  {
    if ( error ) *error = file.errorString();
    return false;
  }
  file.write( QJsonDocument( array ).toJson( QJsonDocument::Indented ) );
  if ( !file.commit() )
  {
    if ( error ) *error = file.errorString();
    return false;
  }
  emit changed();
  return true;
}

bool OfflineExportQueue::upsert( const OfflineExportJob &job, QString *error )
{
  for ( OfflineExportJob &existing : mJobs )
  {
    if ( existing.jobId == job.jobId )
    {
      existing = job;
      return save( error );
    }
  }
  mJobs.append( job );
  return save( error );
}

bool OfflineExportQueue::remove( const QString &jobId, QString *error )
{
  for ( qsizetype i = 0; i < mJobs.size(); ++i )
  {
    if ( mJobs.at( i ).jobId == jobId )
    {
      mJobs.removeAt( i );
      return save( error );
    }
  }
  return true;
}
