#include "megacloudprovider.h"
#include "offlineprojectexporter.h"
#include "syncengine.h"
#include "tgpfieldservices.h"

using namespace Tgp;

TgpFieldServices::TgpFieldServices( const QString &storageDirectory, QObject *parent )
  : QObject( parent )
  , mCloudProvider( std::make_unique<MegaCloudProvider>( storageDirectory ) )
  , mOfflineExporter( std::make_unique<OfflineProjectExporter>( storageDirectory ) )
  , mSyncEngine( std::make_unique<SyncEngine>( mOfflineExporter->queue(), mCloudProvider.get() ) )
{
  mCloudProvider->setParent( this );
  mOfflineExporter->setParent( this );
  mSyncEngine->setParent( this );
}

TgpFieldServices::~TgpFieldServices() = default;

QObject *TgpFieldServices::cloudProvider() const
{
  return mCloudProvider.get();
}

QObject *TgpFieldServices::offlineExporter() const
{
  return mOfflineExporter.get();
}

QObject *TgpFieldServices::syncEngine() const
{
  return mSyncEngine.get();
}
