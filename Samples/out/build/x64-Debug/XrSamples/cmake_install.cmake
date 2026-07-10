# Install script for directory: C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/XrSamples

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
  # Include the install script for each subdirectory.
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/BulletProject1/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrBodyFaceEyeSocial/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrColocationDiscovery/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrColorSpaceFB/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrCompositor_NativeActivity/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrControllers/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrDynamicObjects/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrHandDataSource/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrHandTrackingWideMotionMode/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrHandsAndControllers/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrHandsFB/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrInput/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrMicrogestures/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrOverlayKeyboard/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrPassthrough/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrPassthroughOcclusion/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrSceneModel/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrSceneSharing/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrSpaceWarp/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrSpatialAnchor/cmake_install.cmake")
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/XrVirtualKeyboard/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/XrSamples/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
