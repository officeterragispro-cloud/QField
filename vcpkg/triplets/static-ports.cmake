set(STATIC_PORTS
    gdal
    catch2
    libiconv
    freexl
    libspatialite
    qhull
    harfbuzz
    libb2
    freeglut
    qca # OSSL plugin not discovered unless static
    qgis
    poppler
)

# androiddeployqt rejects ICU's versioned shared-library names (for example
# libicudata.so.78). MEGA SDK links ICU statically in Android builds instead.
if(DEFINED ENV{ANDROID_NDK_HOME})
  list(APPEND STATIC_PORTS icu)
endif()
