#include "megacloudprovider.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QSettings>
#include <QStringList>
#include <qgsapplication.h>
#include <qgsauthmanager.h>

#include <utility>

#ifdef TGP_WITH_MEGA_SDK
#include <megaapi.h>
#endif

using namespace Tgp;

namespace
{
  constexpr auto SESSION_CONFIG_KEY = "/TGPField/Mega/sessionConfigId";
  constexpr auto ACCOUNT_EMAIL_KEY = "/TGPField/Mega/accountEmail";
  constexpr auto SESSION_VALUE_KEY = "tgp-mega-session";
} // namespace

#ifdef TGP_WITH_MEGA_SDK
class MegaCloudProviderPrivate final : public mega::MegaRequestListener, public mega::MegaTransferListener
{
  public:
    MegaCloudProviderPrivate( MegaCloudProvider *owner, const QString &storageDirectory )
      : q( owner )
    {
      const QString cacheDirectory = QDir( storageDirectory ).filePath( QStringLiteral( "mega-sdk" ) );
      QDir().mkpath( cacheDirectory );
      api = std::make_unique<mega::MegaApi>( TGP_MEGA_APP_KEY, cacheDirectory.toUtf8().constData(), "TGP-FIELD/1.0" );
    }

    ~MegaCloudProviderPrivate() override
    {
      // Stop SDK callbacks before destroying the state used by the listeners.
      api.reset();
    }

    bool uploading() const
    {
      return !uploadId.isEmpty();
    }

    void upload( const QString &objectId, const QString &remotePath, const QString &localPath )
    {
      uploadId = objectId;
      uploadSource = localPath;
      uploadParts = remotePath.split( QLatin1Char( '/' ), Qt::SkipEmptyParts );
      uploadName = uploadParts.takeLast();
      folderIndex = 0;
      std::unique_ptr<mega::MegaNode> root( api->getRootNode() );
      if ( !root )
      {
        finishUpload( false, MegaCloudProvider::tr( "MEGA cloud drive is unavailable. Reconnect and retry." ) );
        return;
      }
      parentHandle = root->getHandle();
      advanceUpload();
    }

    void onTransferUpdate( mega::MegaApi *, mega::MegaTransfer *transfer ) override
    {
      const QString id = QString::fromUtf8( transfer->getAppData() );
      const qint64 completed = transfer->getTransferredBytes();
      const qint64 total = transfer->getTotalBytes();
      QMetaObject::invokeMethod(
        q, [this, id, completed, total]() {
          if ( id == uploadId )
            emit q->transferProgress( id, completed, total );
        },
        Qt::QueuedConnection );
    }

    void onTransferFinish( mega::MegaApi *, mega::MegaTransfer *transfer, mega::MegaError *error ) override
    {
      const QString id = QString::fromUtf8( transfer->getAppData() );
      const bool success = error->getErrorCode() == mega::MegaError::API_OK;
      const QString message = success ? MegaCloudProvider::tr( "Archive uploaded to MEGA." ) : QString::fromUtf8( error->getErrorString() );
      QMetaObject::invokeMethod(
        q, [this, id, success, message]() {
          if ( id == uploadId )
            finishUpload( success, message );
        },
        Qt::QueuedConnection );
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
#ifdef ENABLE_SYNC
      api->logout( false, this );
#else
      api->logout( this );
#endif
    }

    void onRequestFinish( mega::MegaApi *, mega::MegaRequest *request, mega::MegaError *error ) override
    {
      const int requestType = request->getType();
      const bool success = error->getErrorCode() == mega::MegaError::API_OK;
      const QString errorMessage = QString::fromUtf8( error->getErrorString() );

      if ( requestType == mega::MegaRequest::TYPE_CREATE_FOLDER )
      {
        const auto handle = request->getNodeHandle();
        QMetaObject::invokeMethod(
          q, [this, success, errorMessage, handle]() {
            if ( uploadId.isEmpty() )
              return;
            if ( !success )
            {
              finishUpload( false, errorMessage );
              return;
            }
            parentHandle = handle;
            ++folderIndex;
            advanceUpload();
          },
          Qt::QueuedConnection );
        return;
      }

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
        QMetaObject::invokeMethod(
          q, [this]() {
            q->setReady( false );
            q->setStatusMessage( MegaCloudProvider::tr( "MEGA disconnected." ) );
          },
          Qt::QueuedConnection );
      }
    }

    QString pendingEmail;
    std::unique_ptr<mega::MegaApi> api;

  private:
    QString uploadId;
    QString uploadSource;
    QString uploadName;
    QStringList uploadParts;
    qsizetype folderIndex = 0;
    mega::MegaHandle parentHandle = mega::INVALID_HANDLE;

    void finishUpload( bool success, const QString &message )
    {
      const QString id = uploadId;
      uploadId.clear();
      uploadParts.clear();
      q->setStatusMessage( message );
      emit q->transferFinished( id, success, message );
    }

    void advanceUpload()
    {
      std::unique_ptr<mega::MegaNode> parent( api->getNodeByHandle( parentHandle ) );
      if ( !parent || !parent->isFolder() )
      {
        finishUpload( false, MegaCloudProvider::tr( "MEGA destination folder is unavailable." ) );
        return;
      }
      while ( folderIndex < uploadParts.size() )
      {
        const QByteArray name = uploadParts.at( folderIndex ).toUtf8();
        std::unique_ptr<mega::MegaNode> child( api->getChildNode( parent.get(), name.constData() ) );
        if ( !child )
        {
          api->createFolder( name.constData(), parent.get(), this );
          return;
        }
        if ( !child->isFolder() )
        {
          finishUpload( false, MegaCloudProvider::tr( "A file occupies the destination folder path in MEGA." ) );
          return;
        }
        parentHandle = child->getHandle();
        parent = std::move( child );
        ++folderIndex;
      }
      api->startUpload( uploadSource.toUtf8().constData(), parent.get(), uploadName.toUtf8().constData(),
                        mega::MegaApi::INVALID_CUSTOM_MOD_TIME, uploadId.toUtf8().constData(), false, false, nullptr, this );
    }

    void dispatchAuthentication( bool success, const QString &message, const QByteArray &session )
    {
      QMetaObject::invokeMethod(
        q, [this, success, message, session]() {
          if ( !pendingEmail.isEmpty() )
            q->setAccountEmail( pendingEmail );
          q->finishAuthentication( success, message, session );
        },
        Qt::QueuedConnection );
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
  if ( hasStoredSession() )
    QMetaObject::invokeMethod( this, &MegaCloudProvider::authenticate, Qt::QueuedConnection );
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
#ifdef TGP_WITH_MEGA_SDK
  const QByteArray session = storedSession();
  if ( session.isEmpty() )
  {
    finishAuthentication( false, tr( "No saved MEGA session. Enter your account details." ) );
    return;
  }

  setAuthenticating( true );
  setStatusMessage( tr( "Restoring the MEGA session…" ) );
  mSdk->resume( session );
#else
  finishAuthentication( false, tr( "MEGA SDK is not included in this build." ) );
#endif
}

void MegaCloudProvider::login( const QString &email, const QString &password, bool rememberSession )
{
#ifdef TGP_WITH_MEGA_SDK
  if ( mSdk->uploading() )
  {
    setStatusMessage( tr( "Wait for the current MEGA transfer before changing accounts." ) );
    return;
  }
#endif
  const QString normalizedEmail = email.trimmed();
  if ( normalizedEmail.isEmpty() || password.isEmpty() )
  {
    finishAuthentication( false, tr( "Email and password are required." ) );
    return;
  }
#ifdef TGP_WITH_MEGA_SDK
  mRememberSession = rememberSession;
  setAccountEmail( normalizedEmail );
  setAuthenticating( true );
  setStatusMessage( tr( "Connecting to MEGA…" ) );
  mSdk->login( normalizedEmail, password );
#else
  Q_UNUSED( rememberSession )
  finishAuthentication( false, tr( "This installer was built without the MEGA SDK." ) );
#endif
}

void MegaCloudProvider::logout()
{
#ifdef TGP_WITH_MEGA_SDK
  if ( mSdk->uploading() )
  {
    setStatusMessage( tr( "Wait for the current MEGA transfer to finish before disconnecting." ) );
    return;
  }
#endif
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
  emit transferFinished( {}, false, tr( "Downloading snapshots inside TGP-FIELD is not implemented yet. Use MEGA to download the exported archive." ) );
}

void MegaCloudProvider::uploadObject( const QString &objectId, const QString &remotePath, const QUrl &source, const QByteArray &sha256 )
{
#ifdef TGP_WITH_MEGA_SDK
  if ( !isReady() || mSdk->uploading() )
  {
    emit transferFinished( objectId, false, tr( "MEGA is disconnected or another upload is active." ) );
    return;
  }
  const QStringList parts = remotePath.split( QLatin1Char( '/' ), Qt::SkipEmptyParts );
  if ( objectId.isEmpty() || !remotePath.startsWith( QLatin1String( "/TGP-FIELD/" ) ) || parts.size() < 2
       || parts.contains( QStringLiteral( "." ) ) || parts.contains( QStringLiteral( ".." ) )
       || remotePath.contains( QLatin1Char( '\\' ) ) || remotePath.contains( QChar( 0 ) ) || remotePath.endsWith( QLatin1Char( '/' ) ) )
  {
    emit transferFinished( objectId, false, tr( "Invalid MEGA destination path." ) );
    return;
  }
  QFile file( source.toLocalFile() );
  QCryptographicHash hash( QCryptographicHash::Sha256 );
  if ( !source.isLocalFile() || !QFileInfo( file ).isFile() || !file.open( QIODevice::ReadOnly ) || !hash.addData( &file ) )
  {
    emit transferFinished( objectId, false, tr( "Cannot read the local export archive." ) );
    return;
  }
  if ( sha256.size() != 32 || hash.result() != sha256 )
  {
    emit transferFinished( objectId, false, tr( "The archive changed since export. Create a new export before uploading." ) );
    return;
  }
  file.close();
  setStatusMessage( tr( "Uploading archive to MEGA…" ) );
  mSdk->upload( objectId, remotePath, source.toLocalFile() );
#else
  Q_UNUSED( remotePath )
  Q_UNUSED( source )
  Q_UNUSED( sha256 )
  emit transferFinished( objectId, false, tr( "This installer was built without the MEGA SDK." ) );
#endif
}

void MegaCloudProvider::setReady( bool ready )
{
  if ( mReady == ready )
    return;
  mReady = ready;
  emit readyChanged();
}

void MegaCloudProvider::setAuthenticating( bool authenticating )
{
  if ( mAuthenticating == authenticating )
    return;
  mAuthenticating = authenticating;
  emit authenticatingChanged();
}

void MegaCloudProvider::setAccountEmail( const QString &email )
{
  if ( mAccountEmail == email )
    return;
  mAccountEmail = email;
  QSettings().setValue( QLatin1String( ACCOUNT_EMAIL_KEY ), email );
  emit accountEmailChanged();
}

void MegaCloudProvider::setStatusMessage( const QString &message )
{
  if ( mStatusMessage == message )
    return;
  mStatusMessage = message;
  emit statusMessageChanged();
}

QByteArray MegaCloudProvider::storedSession() const
{
  if ( mSessionConfigId.isEmpty() )
    return {};
  QgsAuthMethodConfig config;
  if ( !QgsApplication::authManager()->loadAuthenticationConfig( mSessionConfigId, config, true ) )
    return {};
  return config.config( QLatin1String( SESSION_VALUE_KEY ) ).toLatin1();
}

void MegaCloudProvider::saveSession( const QByteArray &session )
{
  if ( session.isEmpty() )
    return;
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
