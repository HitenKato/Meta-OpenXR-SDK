# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/3rdParty/khronos/ktx_src")
  file(MAKE_DIRECTORY "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/3rdParty/khronos/ktx_src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-build"
  "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-subbuild/libktx-populate-prefix"
  "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-subbuild/libktx-populate-prefix/tmp"
  "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-subbuild/libktx-populate-prefix/src/libktx-populate-stamp"
  "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-subbuild/libktx-populate-prefix/src"
  "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-subbuild/libktx-populate-prefix/src/libktx-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-subbuild/libktx-populate-prefix/src/libktx-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Debug/_deps/libktx-subbuild/libktx-populate-prefix/src/libktx-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
