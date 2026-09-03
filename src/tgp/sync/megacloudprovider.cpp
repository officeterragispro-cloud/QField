#include "megacloudprovider.h"

using namespace Tgp;

MegaCloudProvider::MegaCloudProvider( QObject *parent )
  : CloudProvider( parent )
{}

QString MegaCloudProvider::providerName() const
{
  return QStringLiteral( "MEGA" );
}

bool MegaCloudProvider::isReady() const
{
  return false;
}

void MegaCloudProvider::authenticate()
{
  emit authenticationFinished( false, tr( "MEGA SDK is not linked yet." ) );
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
  emit transferFinished( objectId, false, tr( "MEGA SDK is not linked yet." ) );
}
