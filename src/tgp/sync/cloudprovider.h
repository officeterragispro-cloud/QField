#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>

namespace Tgp
{
  struct CloudObject
  {
      QString id;
      QString path;
      QByteArray sha256;
      qint64 size = 0;
  };

  class CloudProvider : public QObject
  {
      Q_OBJECT
      Q_PROPERTY( QString providerName READ providerName CONSTANT )
      Q_PROPERTY( bool ready READ isReady NOTIFY readyChanged )
    public:
      using QObject::QObject;
      ~CloudProvider() override = default;

      virtual QString providerName() const = 0;
      virtual void authenticate() = 0;
      virtual void listProjects() = 0;
      virtual void downloadSnapshot( const QString &projectId, const QString &versionId, const QUrl &destination ) = 0;
      virtual bool isReady() const = 0;
      virtual void uploadObject( const QString &objectId, const QString &remotePath, const QUrl &source, const QByteArray &sha256 ) = 0;

    signals:
      void readyChanged();
      void authenticationFinished( bool success, const QString &message );
      void projectsListed( const QVariantList &projects );
      void transferProgress( const QString &objectId, qint64 completed, qint64 total );
      void transferFinished( const QString &objectId, bool success, const QString &message );
  };
} // namespace Tgp
