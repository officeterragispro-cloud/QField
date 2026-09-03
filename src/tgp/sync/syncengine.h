#pragma once

#include "synctypes.h"

#include <QObject>
#include <QPointer>

namespace Tgp
{
  class CloudProvider;
  class OfflineExportQueue;

  class SyncEngine final : public QObject
  {
      Q_OBJECT
      Q_PROPERTY( bool busy READ busy NOTIFY busyChanged )
    public:
      SyncEngine( OfflineExportQueue *queue, CloudProvider *provider, QObject *parent = nullptr );

      bool busy() const;
      Q_INVOKABLE void synchronize();
      Q_INVOKABLE void retry( const QString &jobId );

    signals:
      void busyChanged();
      void jobProgress( const QString &jobId, qint64 completed, qint64 total );
      void jobStateChanged( const QString &jobId, const QString &state, const QString &message );

    private slots:
      void handleTransferProgress( const QString &objectId, qint64 completed, qint64 total );
      void handleTransferFinished( const QString &objectId, bool success, const QString &message );

    private:
      void processNext();
      void setBusy( bool busy );
      OfflineExportQueue *mQueue = nullptr;
      QPointer<CloudProvider> mProvider;
      QString mActiveJobId;
      bool mBusy = false;
  };
}
