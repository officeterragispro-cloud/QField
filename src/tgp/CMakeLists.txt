set(TGP_SYNC_SRCS
  sync/offlineexportqueue.cpp
  sync/offlineprojectexporter.cpp
  sync/megacloudprovider.cpp
  sync/syncengine.cpp
  tgpfieldservices.cpp
)

set(TGP_SYNC_HDRS
  sync/cloudprovider.h
  sync/offlineexportqueue.h
  sync/offlineprojectexporter.h
  sync/megacloudprovider.h
  sync/syncengine.h
  sync/synctypes.h
  tgpfieldservices.h
)

add_library(tgp_sync STATIC ${TGP_SYNC_SRCS} ${TGP_SYNC_HDRS})
target_include_directories(tgp_sync PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/sync)
target_link_libraries(tgp_sync PUBLIC Qt6::Core QGIS::Core qfield_core)
set_target_properties(tgp_sync PROPERTIES AUTOMOC TRUE)
