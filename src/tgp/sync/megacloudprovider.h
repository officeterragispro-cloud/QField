#pragma once

#include "cloudprovider.h"

#include <memory>

class MegaCloudProviderPrivate;

namespace Tgp
{
  // Boundary around the official MEGA SDK. No credentials are compiled in.
  class MegaCloudProvider final : public CloudProvider
  {
      Q_OBJECT
      Q_PROPERTY( bool sdkAvailable READ sdkAvailable CONSTANT )
      Q_PROPERTY( bool authenticating READ authenticating NOTIFY authenticatingChanged )
      Q_PROPERTY( bool hasStoredSession READ hasStoredSession NOTIFY storedSessionChanged )
      Q_PROPERTY( QString accountEmail READ accountEmail NOTIFY accountEmailChanged )
      Q_PROPERTY( QString statusMessage READ statusMessage NOTIFY statusMessageChanged )
    public:
      explicit MegaCloudProvider( const QString &storageDirectory, QObject *parent = nullptr );
      ~MegaCloudProvider() override;

      QString providerName() const override;
      bool isReady() const override;
      bool sdkAvailable() const;
      bool authenticating() const;
      bool hasStoredSession() const;
      QString accountEmail() const;
      QString statusMessage() const;

      void authenticate() override;
      Q_INVOKABLE void login( const QString &email, const QString &password, bool rememberSession = true );
      Q_INVOKABLE void logout();
      void listProjects() override;
      void downloadSnapshot( const QString &projectId, const QString &versionId, const QUrl &destination ) override;
      void uploadObject( const QString &objectId, const QString &remotePath, const QUrl &source, const QByteArray &sha256 ) override;

    signals:
      void authenticatingChanged();
      void storedSessionChanged();
      void accountEmailChanged();
      void statusMessageChanged();

    private:
      friend class ::MegaCloudProviderPrivate;

      void setReady( bool ready );
      void setAuthenticating( bool authenticating );
      void setAccountEmail( const QString &email );
      void setStatusMessage( const QString &message );
      QByteArray storedSession() const;
      void saveSession( const QByteArray &session );
      void clearStoredSession();
      void finishAuthentication( bool success, const QString &message, const QByteArray &session = QByteArray() );

      QString mAccountEmail;
      QString mStatusMessage;
      QString mSessionConfigId;
      bool mReady = false;
      bool mAuthenticating = false;
      bool mRememberSession = true;
      std::unique_ptr<::MegaCloudProviderPrivate> mSdk;
  };
}
