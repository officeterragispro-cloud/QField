#include "megacloudprovider.h"

#include <QDir>
#include <QMetaObject>
#include <QSettings>

#include <qgsapplication.h>
#include <qgsauthmanager.h>
#include <qgsauthmethodconfig.h>

#ifdef TGP_WITH_MEGA_SDK
#include <megaapi.h>
#endif

using namespace Tgp;

namespace
{
  constexpr auto SESSION_CONFIG_KEY = "/TGPField/Mega/sessionConfigId";
  constexpr auto ACCOUNT_EMAIL_KEY = "/TGPField/Mega/accountEmail";
  constexpr auto SESSION_VALUE_KEY = "tgp-mega-session";
}

#ifdef TGP_WITH_MEGA_SDK
class MegaCloudProviderPrivate final : public mega::MegaRequestListener
{
  public:
    MegaCloudProviderPrivate( MegaCloudProvider *owner, const QString &storageDirectory )
      : q( owner )
    {
      const QString cacheDirectory = QDir( storageDirectory ).filePath( QStringLiteral( "mega-sdk" ) );
      QDir().mkpath( cacheDirectory );
      api = std::make_unique<mega::MegaApi>( TGP_MEGA_APP_KEY, cacheDirectory.toUtf8().constData(), "TGP-FIELD/1.0" );
    }

    void login( const QString &email, const QString &password )
    {
      pendingEmail = email;
      api->login( email.toUtf8().constData(), password.toUtf8().constData(), this );
    }

    void resume( const QByteArray &session )
    {
      api->fastLogin( session.constData(), this );
    }

    void logout()
    {
      api->logout( this );
    }

    void onRequestFinish( mega::MegaApi *, mega::MegaRequest *request, mega::MegaError *error ) override
    {
      const int requestType = request->getType();
      const bool success = error->getErrorCode() == mega::MegaError::API_OK;
      const QString errorMessage = QString::fromUtf8( error->getErrorString() );

      if ( requestType == mega::MegaRequest::TYPE_LOGIN )
      {
        if ( success )
          api->fetchNodes( this );
        else
          dispatchAuthentication( false, errorMessage, {} );
        return;
      }

      if ( requestType == mega::MegaRequest::TYPE_FETCH_NODES )
      {
        if ( !success )
        {
          dispatchAuthentication( false, errorMessage, {} );
          return;
        }

        QByteArray session;
        if ( char *rawSession = api->dumpSession() )
        {
          session = QByteArray( rawSession );
          delete[] rawSession;
        }
        dispatchAuthentication( true, MegaCloudProvider::tr( "Connected to MEGA." ), session );
        return;
      }

      if ( requestType == mega::MegaRequest::TYPE_LOGOUT )
      {
        QMetaObject::invokeMethod( q, [this]() {
          q->setReady( false );
          q->setStatusMessage( MegaCloudProvider::tr( "MEGA disconnected." ) );
        }, Qt::QueuedConnection );
      }
    }

    QString pendingEmail;
    std::unique_ptr<mega::MegaApi> api;

  private:
    void dispatchAuthentication( bool success, const QString &message, const QByteArray &session )
    {
      QMetaObject::invokeMethod( q, [this, success, message, session]() {
        if ( !pendingEmail.isEmpty() ) q->setAccountEmail( pendingEmail );
        q->finishAuthentication( success, message, session );
      }, Qt::QueuedConnection );
    }

    MegaCloudProvider *q = nullptr;
};
#else
class MegaCloudProviderPrivate
{};
#endif

MegaCloudProvider::MegaCloudProvider( const QString &storageDirectory, QObject *parent )
  : CloudProvider( parent )
  , mAccountEmail( QSettings().value( QLatin1String( ACCOUNT_EMAIL_KEY ) ).toString() )
  , mSessionConfigId( QSettings().value( QLatin1String( SESSION_CONFIG_KEY ) ).toString() )
{
#ifdef TGP_WITH_MEGA_SDK
  mSdk = std::make_unique<MegaCloudProviderPrivate>( this, storageDirectory );
  setStatusMessage( hasStoredSession() ? tr( "Saved MEGA session found." ) : tr( "MEGA is ready to connect." ) );
  if ( hasStoredSession() ) QMetaObject::invokeMethod( this, &MegaCloudProvider::authenticate, Qt::QueuedConnection );
#else
  setStatusMessage( tr( "MEGA SDK is not included in this build." ) );
#endif
}

MegaCloudProvider::~MegaCloudProvider() = default;

QString MegaCloudProvider::providerName() const
{
  return QStringLiteral( "MEGA" );
}

bool MegaCloudProvider::isReady() const
{
  return mReady;
}

bool MegaCloudProvider::sdkAvailable() const
{
#ifdef TGP_WITH_MEGA_SDK
  return true;
#else
  return false;
#endif
}

bool MegaCloudProvider::authenticating() const
{
  return mAuthenticating;
}

bool MegaCloudProvider::hasStoredSession() const
{
  return !storedSession().isEmpty();
}

QString MegaCloudProvider::accountEmail() const
{
  return mAccountEmail;
}

QString MegaCloudProvider::statusMessage() const
{
  return mStatusMessage;
}

void MegaCloudProvider::authenticate()
{
  if ( !sdkAvailable() )
  {
    finishAuthentication( false, tr( "MEGA SDK is not included in this build." ) );
    return;
  }

  const QByteArray session = storedSession();
  if ( session.isEmpty() )
  {
    finishAuthentication( false, tr( "No saved MEGA session. Enter your account details." ) );
    return;
  }

  setAuthenticating( true );
  setStatusMessage( tr( "Restoring the MEGA session…" ) );
#ifdef TGP_WITH_MEGA_SDK
  mSdk->resume( session );
#endif
}

void MegaCloudProvider::login( const QString &email, const QString &password, bool rememberSession )
{
  const QString normalizedEmail = email.trimmed();
  if ( normalizedEmail.isEmpty() || password.isEmpty() )
  {
    finishAuthentication( false, tr( "Email and password are required." ) );
    return;
  }
  if ( !sdkAvailable() )
  {
    finishAuthentication( false, tr( "This installer was built without the MEGA SDK." ) );
    return;
  }

  mRememberSession = rememberSession;
  setAccountEmail( normalizedEmail );
  setAuthenticating( true );
  setStatusMessage( tr( "Connecting to MEGA…" ) );
#ifdef TGP_WITH_MEGA_SDK
  mSdk->login( normalizedEmail, password );
#endif
}

void MegaCloudProvider::logout()
{
  clearStoredSession();
  setReady( false );
  setAuthenticating( false );
  setStatusMessage( tr( "MEGA disconnected." ) );
#ifdef TGP_WITH_MEGA_SDK
  mSdk->logout();
#endif
}

void MegaCloudProvider::listProjects()
{
  emit projectsListed( {} );
}

void MegaCloudProvider::downloadSnapshot( const QString &, const QString &, const QUrl & )
{
  emit transferFinished( {}, false, tr( "MEGA SDK is not linked yet." ) );
}

void MegaCloudProvider::uploadObject( const QString &objectId, const QString &, const QUrl &, const QByteArray & )
{
  emit transferFinished( objectId, false, isReady() ? tr( "MEGA upload support is the next integration stage." ) : tr( "Connect to MEGA before uploading." ) );
}

void MegaCloudProvider::setReady( bool ready )
{
  if ( mReady == ready ) return;
  mReady = ready;
  emit readyChanged();
}

void MegaCloudProvider::setAuthenticating( bool authenticating )
{
  if ( mAuthenticating == authenticating ) return;
  mAuthenticating = authenticating;
  emit authenticatingChanged();
}

void MegaCloudProvider::setAccountEmail( const QString &email )
{
  if ( mAccountEmail == email ) return;
  mAccountEmail = email;
  QSettings().setValue( QLatin1String( ACCOUNT_EMAIL_KEY ), email );
  emit accountEmailChanged();
}

void MegaCloudProvider::setStatusMessage( const QString &message )
{
  if ( mStatusMessage == message ) return;
  mStatusMessage = message;
  emit statusMessageChanged();
}

QByteArray MegaCloudProvider::storedSession() const
{
  if ( mSessionConfigId.isEmpty() ) return {};
  QgsAuthMethodConfig config;
  if ( !QgsApplication::authManager()->loadAuthenticationConfig( mSessionConfigId, config, true ) ) return {};
  return config.config( QLatin1String( SESSION_VALUE_KEY ) ).toLatin1();
}

void MegaCloudProvider::saveSession( const QByteArray &session )
{
  if ( session.isEmpty() ) return;
  QgsAuthMethodConfig config;
  if ( !mSessionConfigId.isEmpty() )
    QgsApplication::authManager()->loadAuthenticationConfig( mSessionConfigId, config, true );
  if ( !config.isValid() )
  {
    config.setName( QStringLiteral( "TGP-FIELD MEGA session" ) );
    config.setMethod( QStringLiteral( "Basic" ) );
  }
  config.setConfig( QLatin1String( SESSION_VALUE_KEY ), session );
  if ( QgsApplication::authManager()->storeAuthenticationConfig( config, true ) )
  {
    mSessionConfigId = config.id();
    QSettings().setValue( QLatin1String( SESSION_CONFIG_KEY ), mSessionConfigId );
    emit storedSessionChanged();
  }
}

void MegaCloudProvider::clearStoredSession()
{
  if ( !mSessionConfigId.isEmpty() )
    QgsApplication::authManager()->removeAuthenticationConfig( mSessionConfigId );
  mSessionConfigId.clear();
  QSettings().remove( QLatin1String( SESSION_CONFIG_KEY ) );
  emit storedSessionChanged();
}

void MegaCloudProvider::finishAuthentication( bool success, const QString &message, const QByteArray &session )
{
  setAuthenticating( false );
  setReady( success );
  setStatusMessage( message );
  if ( success )
  {
    if ( mRememberSession )
      saveSession( session );
    else
      clearStoredSession();
  }
  emit authenticationFinished( success, message );
}
