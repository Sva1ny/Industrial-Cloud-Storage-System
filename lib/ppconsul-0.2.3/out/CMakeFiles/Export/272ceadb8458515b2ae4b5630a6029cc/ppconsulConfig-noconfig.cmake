#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "ppconsul" for configuration ""
set_property(TARGET ppconsul APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(ppconsul PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libppconsul.0.1.dylib"
  IMPORTED_SONAME_NOCONFIG "@rpath/libppconsul.0.1.dylib"
  )

list(APPEND _cmake_import_check_targets ppconsul )
list(APPEND _cmake_import_check_files_for_ppconsul "${_IMPORT_PREFIX}/lib/libppconsul.0.1.dylib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
