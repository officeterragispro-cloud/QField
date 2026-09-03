#include "offlineexportqueue.h"

#include <QTemporaryDir>

#include <catch2/catch_test_macros.hpp>

TEST_CASE( "TGP offline export queue persists jobs atomically", "[tgp][sync]" )
{
  QTemporaryDir directory;
  REQUIRE( directory.isValid() );

  Tgp::OfflineExportQueue queue( directory.path() );
  Tgp::OfflineExportJob job;
  job.jobId = QStringLiteral( "job-1" );
  job.projectId = QStringLiteral( "project-1" );
  job.archivePath = QStringLiteral( "/tmp/project.tgpfield.zip" );
  job.remotePath = QStringLiteral( "/TGP-FIELD/workspace/project-1/exports/job-1/project.tgpfield.zip" );
  job.sha256 = QByteArray( 32, '\x2a' );
  job.size = 1024;
  job.createdAt = QStringLiteral( "2026-09-02T10:00:00.000Z" );
  job.state = Tgp::ExportState::WaitingForNetwork;

  QString error;
  REQUIRE( queue.upsert( job, &error ) );
  REQUIRE( error.isEmpty() );
  REQUIRE( queue.pendingJobs().size() == 1 );

  Tgp::OfflineExportQueue reloaded( directory.path() );
  REQUIRE( reloaded.jobs().size() == 1 );
  CHECK( reloaded.jobs().constFirst().jobId == job.jobId );
  CHECK( reloaded.jobs().constFirst().sha256 == job.sha256 );

  job.state = Tgp::ExportState::Complete;
  REQUIRE( reloaded.upsert( job, &error ) );
  CHECK( reloaded.pendingJobs().isEmpty() );
}

TEST_CASE( "TGP export states have stable serialized names", "[tgp][sync]" )
{
  CHECK( Tgp::exportStateName( Tgp::ExportState::WaitingForNetwork ) == QStringLiteral( "waiting_for_network" ) );
  CHECK( Tgp::exportStateFromName( QStringLiteral( "uploading" ) ) == Tgp::ExportState::Uploading );
  CHECK( Tgp::exportStateFromName( QStringLiteral( "unknown" ) ) == Tgp::ExportState::Failed );
}
