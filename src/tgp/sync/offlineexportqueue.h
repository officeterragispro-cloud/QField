#pragma once

#include "synctypes.h"

#include <QObject>

namespace Tgp
{
  class OfflineExportQueue final : public QObject
  {
      Q_OBJECT
    public:
      explicit OfflineExportQueue( const QString &storageDirectory, QObject *parent = nullptr );

      QList<OfflineExportJob> jobs() const;
      QList<OfflineExportJob> pendingJobs() const;
      bool upsert( const OfflineExportJob &job, QString *error = nullptr );
      bool remove( const QString &jobId, QString *error = nullptr );
      bool reload( QString *error = nullptr );
      QString queueFilePath() const;

    signals:
      void changed();

    private:
      bool save( QString *error );
      QString mStorageDirectory;
      QList<OfflineExportJob> mJobs;
  };
}
