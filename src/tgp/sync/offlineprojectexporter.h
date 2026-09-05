#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>

#include <memory>

class QgsProject;

namespace Tgp
{
  class OfflineExportQueue;

  struct OfflineExportRequest
  {
      QString projectId;
      QString workspaceId;
      QUrl qgisProject;
      QList<QUrl> geopackages;
      QUrl attachmentsDirectory;
      bool keepLocalCopy = true;
  };

  // Creates an immutable, QGIS-readable snapshot and persists an upload job.
  // The implementation must checkpoint/close GeoPackage writers before copy.
  class OfflineProjectExporter final : public QObject
  {
      Q_OBJECT
      Q_PROPERTY( QString storageDirectory READ storageDirectory CONSTANT )
      Q_PROPERTY( QVariantList jobs READ jobs NOTIFY jobsChanged )
      Q_PROPERTY( int pendingCount READ pendingCount NOTIFY jobsChanged )
    public:
      explicit OfflineProjectExporter( const QString &storageDirectory, QObject *parent = nullptr );
      ~OfflineProjectExporter() override;

      Q_INVOKABLE void enqueueExport( const OfflineExportRequest &request );
      Q_INVOKABLE void enqueueCurrentProject( QObject *projectObject, const QString &projectId, const QString &workspaceId, bool keepLocalCopy = true );
      Q_INVOKABLE void resumePendingExports();
      Q_INVOKABLE void cancelExport( const QString &jobId );

      QString storageDirectory() const;
      QVariantList jobs() const;
      int pendingCount() const;
      OfflineExportQueue *queue() const;

    signals:
      void exportQueued( const QString &jobId, const QUrl &archive );
      void exportProgress( const QString &jobId, qint64 completed, qint64 total );
      void exportStateChanged( const QString &jobId, const QString &state, const QString &message );
      void jobsChanged();

    private:
      QString mStorageDirectory;
      std::unique_ptr<OfflineExportQueue> mQueue;
  };
} // namespace Tgp

Q_DECLARE_METATYPE( Tgp::OfflineExportRequest )
