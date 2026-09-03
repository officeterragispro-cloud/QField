#include "syncengine.h"

#include "cloudprovider.h"
#include "offlineexportqueue.h"

#include <QFileInfo>
#include <QUrl>

using namespace Tgp;

SyncEngine::SyncEngine( OfflineExportQueue *queue, CloudProvider *provider, QObject *parent )
  : QObject( parent )
  , mQueue( queue )
  , mProvider( provider )
{
  Q_ASSERT( mQueue );
  Q_ASSERT( mProvider );
  connect( provider, &CloudProvider::transferProgress, this, &SyncEngine::handleTransferProgress );
  connect( provider, &CloudProvider::transferFinished, this, &SyncEngine::handleTransferFinished );
}

bool SyncEngine::busy() const
{
  return mBusy;
}

void SyncEngine::setBusy( bool busy )
{
  if ( mBusy == busy ) return;
  mBusy = busy;
  emit busyChanged();
}

void SyncEngine::synchronize()
{
  if ( busy() ) return;
  if ( !mProvider || !mProvider->isReady() )
  {
    emit jobStateChanged( {}, exportStateName( ExportState::WaitingForNetwork ), tr( "Cloud provider is not ready." ) );
    return;
  }
  setBusy( true );
  processNext();
}

void SyncEngine::retry( const QString &jobId )
{
  for ( OfflineExportJob job : mQueue->jobs() )
  {
    if ( job.jobId != jobId ) continue;
    job.state = ExportState::WaitingForNetwork;
    job.lastError.clear();
    mQueue->upsert( job );
    break;
  }
  synchronize();
}

void SyncEngine::processNext()
{
  for ( OfflineExportJob job : mQueue->pendingJobs() )
  {
    if ( job.state == ExportState::Preparing || job.state == ExportState::Cancelled ) continue;
    if ( !QFileInfo::exists( job.archivePath ) )
    {
      job.state = ExportState::Failed;
      job.lastError = tr( "Local archive no longer exists." );
      mQueue->upsert( job );
      emit jobStateChanged( job.jobId, exportStateName( job.state ), job.lastError );
      continue;
    }
    job.state = ExportState::Uploading;
    ++job.attempts;
    mQueue->upsert( job );
    mActiveJobId = job.jobId;
    emit jobStateChanged( job.jobId, exportStateName( job.state ), tr( "Uploading to MEGA." ) );
    mProvider->uploadObject( job.jobId, job.remotePath, QUrl::fromLocalFile( job.archivePath ), job.sha256 );
    return;
  }
  mActiveJobId.clear();
  setBusy( false );
}

void SyncEngine::handleTransferProgress( const QString &objectId, qint64 completed, qint64 total )
{
  if ( objectId == mActiveJobId ) emit jobProgress( objectId, completed, total );
}

void SyncEngine::handleTransferFinished( const QString &objectId, bool success, const QString &message )
{
  if ( objectId != mActiveJobId ) return;
  for ( OfflineExportJob job : mQueue->jobs() )
  {
    if ( job.jobId != objectId ) continue;
    job.state = success ? ExportState::Complete : ExportState::Failed;
    job.lastError = success ? QString() : message;
    mQueue->upsert( job );
    emit jobStateChanged( job.jobId, exportStateName( job.state ), message );
    break;
  }
  mActiveJobId.clear();
  processNext();
}
