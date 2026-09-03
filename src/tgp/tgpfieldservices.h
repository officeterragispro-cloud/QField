#pragma once

#include <QObject>

#include <memory>

namespace Tgp
{
  class MegaCloudProvider;
  class OfflineProjectExporter;
  class SyncEngine;

  class TgpFieldServices final : public QObject
  {
      Q_OBJECT
      Q_PROPERTY( QObject *cloudProvider READ cloudProvider CONSTANT )
      Q_PROPERTY( QObject *offlineExporter READ offlineExporter CONSTANT )
      Q_PROPERTY( QObject *syncEngine READ syncEngine CONSTANT )
    public:
      explicit TgpFieldServices( const QString &storageDirectory, QObject *parent = nullptr );
      ~TgpFieldServices() override;

      QObject *cloudProvider() const;
      QObject *offlineExporter() const;
      QObject *syncEngine() const;

    private:
      std::unique_ptr<MegaCloudProvider> mCloudProvider;
      std::unique_ptr<OfflineProjectExporter> mOfflineExporter;
      std::unique_ptr<SyncEngine> mSyncEngine;
  };
}
