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

option(TGP_WITH_MEGA_SDK "Build TGP-FIELD with the official MEGA SDK" OFF)
set(TGP_MEGA_SDK_SOURCE_DIR "" CACHE PATH "Path to a checkout of the official MEGA SDK")
set(TGP_MEGA_APP_KEY "" CACHE STRING "MEGA application key for TGP-FIELD")

if(TGP_WITH_MEGA_SDK)
  if(TGP_MEGA_SDK_SOURCE_DIR AND NOT TARGET MEGA::SDKlib)
    add_subdirectory("${TGP_MEGA_SDK_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/mega-sdk" EXCLUDE_FROM_ALL)
  endif()

  if(NOT TARGET MEGA::SDKlib)
    message(FATAL_ERROR "TGP_WITH_MEGA_SDK requires TGP_MEGA_SDK_SOURCE_DIR or an existing MEGA::SDKlib target")
  endif()
  if(TGP_MEGA_APP_KEY STREQUAL "")
    message(FATAL_ERROR "TGP_WITH_MEGA_SDK requires a TGP_MEGA_APP_KEY")
  endif()

  target_link_libraries(tgp_sync PRIVATE MEGA::SDKlib)
  target_compile_definitions(tgp_sync PRIVATE
    TGP_WITH_MEGA_SDK=1
    TGP_MEGA_APP_KEY="${TGP_MEGA_APP_KEY}"
  )
endif()
