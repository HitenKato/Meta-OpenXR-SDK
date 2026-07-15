# Install script for directory: C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/3rdParty/khronos/ktx_src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/install/x64-Release")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "RelWithDebInfo")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/interface/basisu_c_binding/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/lib/astc-encoder/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/other_projects/fmt/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/tools/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/tests/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/ktx.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/3rdParty/khronos/ktx_src/include/ktx.h"
    "C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/3rdParty/khronos/ktx_src/include/ktxvulkan.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/lib/astc-encoder/Source/astcenc-avx2-static.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/KHR" TYPE FILE FILES "C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/3rdParty/khronos/ktx_src/include/KHR/khr_df.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "library" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ktx/KtxTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ktx/KtxTargets.cmake"
         "C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/CMakeFiles/Export/eb899b7ebdca6f1dd2b833b33a58189b/KtxTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ktx/KtxTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ktx/KtxTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ktx" TYPE FILE FILES "C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/CMakeFiles/Export/eb899b7ebdca6f1dd2b833b33a58189b/KtxTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ktx" TYPE FILE FILES "C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/CMakeFiles/Export/eb899b7ebdca6f1dd2b833b33a58189b/KtxTargets-relwithdebinfo.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "library" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ktx" TYPE FILE FILES
    "C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/3rdParty/khronos/ktx_src/cmake/KtxConfig.cmake"
    "C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/KtxConfigVersion.cmake"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
