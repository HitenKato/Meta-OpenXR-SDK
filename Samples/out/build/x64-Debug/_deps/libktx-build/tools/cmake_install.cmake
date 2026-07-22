# Install script for directory: C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/3rdParty/khronos/ktx_src/tools

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/install/x64-Debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
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
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-build/tools/imageio/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-build/tools/ktx2check/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-build/tools/ktx2ktx2/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-build/tools/ktxinfo/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-build/tools/ktxsc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-build/tools/toktx/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "tools" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/Debug/ktx2check.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "tools" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/Debug/ktx2ktx2.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "tools" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/Debug/ktxinfo.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "tools" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/Debug/ktxsc.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "tools" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/Debug/toktx.exe")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-build/tools/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
