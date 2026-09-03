#pragma once

#include <QMetaType>
#include <QString>

namespace Tgp
{
  enum class ExportState
  {
    Preparing,
    WaitingForNetwork,
    Uploading,
    Verifying,
    Complete,
    Failed,
    Cancelled
  };

  inline QString exportStateName( ExportState state )
  {
    switch ( state )
    {
      case ExportState::Preparing: return QStringLiteral( "preparing" );
      case ExportState::WaitingForNetwork: return QStringLiteral( "waiting_for_network" );
      case ExportState::Uploading: return QStringLiteral( "uploading" );
      case ExportState::Verifying: return QStringLiteral( "verifying" );
      case ExportState::Complete: return QStringLiteral( "complete" );
      case ExportState::Failed: return QStringLiteral( "failed" );
      case ExportState::Cancelled: return QStringLiteral( "cancelled" );
    }
    return QStringLiteral( "failed" );
  }

  inline ExportState exportStateFromName( const QString &name )
  {
    if ( name == QLatin1String( "preparing" ) ) return ExportState::Preparing;
    if ( name == QLatin1String( "waiting_for_network" ) ) return ExportState::WaitingForNetwork;
    if ( name == QLatin1String( "uploading" ) ) return ExportState::Uploading;
    if ( name == QLatin1String( "verifying" ) ) return ExportState::Verifying;
    if ( name == QLatin1String( "complete" ) ) return ExportState::Complete;
    if ( name == QLatin1String( "cancelled" ) ) return ExportState::Cancelled;
    return ExportState::Failed;
  }

  struct OfflineExportJob
  {
    QString jobId;
    QString projectId;
    QString archivePath;
    QString remotePath;
    QByteArray sha256;
    qint64 size = 0;
    QString createdAt;
    int attempts = 0;
    ExportState state = ExportState::Preparing;
    QString lastError;
    bool keepLocalCopy = true;
  };
}

Q_DECLARE_METATYPE( Tgp::OfflineExportJob )
