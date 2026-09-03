#pragma once

#include "cloudprovider.h"

namespace Tgp
{
  // Boundary around the official MEGA SDK. No credentials are compiled in.
  class MegaCloudProvider final : public CloudProvider
  {
      Q_OBJECT
    public:
      explicit MegaCloudProvider( QObject *parent = nullptr );
      QString providerName() const override;
      bool isReady() const override;
      void authenticate() override;
      void listProjects() override;
      void downloadSnapshot( const QString &projectId, const QString &versionId, const QUrl &destination ) override;
      void uploadObject( const QString &objectId, const QString &remotePath, const QUrl &source, const QByteArray &sha256 ) override;
  };
}
