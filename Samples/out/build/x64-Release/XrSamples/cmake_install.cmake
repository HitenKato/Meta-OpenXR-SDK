# Install script for directory: C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/XrSamples

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
  # Include the install script for each subdirectory.
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/BulletProject1/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrBodyFaceEyeSocial/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrColocationDiscovery/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrColorSpaceFB/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrCompositor_NativeActivity/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrControllers/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrDynamicObjects/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrHandDataSource/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrHandTrackingWideMotionMode/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrHandsAndControllers/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrHandsFB/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrInput/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrMicrogestures/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrOverlayKeyboard/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrPassthrough/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrPassthroughOcclusion/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrSceneModel/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrSceneSharing/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrSpaceWarp/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrSpatialAnchor/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/XrVirtualKeyboard/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/out/build/x64-Release/XrSamples/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
