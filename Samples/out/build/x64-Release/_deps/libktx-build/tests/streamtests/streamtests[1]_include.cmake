if(EXISTS "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Release/RelWithDebInfo/streamtests.exe")
  if(NOT EXISTS "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/tests/streamtests/streamtests[1]_tests.cmake" OR
     NOT "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/tests/streamtests/streamtests[1]_tests.cmake" IS_NEWER_THAN "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Release/RelWithDebInfo/streamtests.exe" OR
     NOT "C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/tests/streamtests/streamtests[1]_tests.cmake" IS_NEWER_THAN "${CMAKE_CURRENT_LIST_FILE}")
    include("C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/share/cmake-3.31/Modules/GoogleTestAddTests.cmake")
    gtest_discover_tests_impl(
      TEST_EXECUTABLE [==[C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Release/RelWithDebInfo/streamtests.exe]==]
      TEST_EXECUTOR [==[]==]
      TEST_WORKING_DIR [==[C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/tests/streamtests]==]
      TEST_EXTRA_ARGS [==[C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/3rdParty/khronos/ktx_src/tests/testimages/]==]
      TEST_PROPERTIES [==[]==]
      TEST_PREFIX [==[streamtest]==]
      TEST_SUFFIX [==[]==]
      TEST_FILTER [==[]==]
      NO_PRETTY_TYPES [==[FALSE]==]
      NO_PRETTY_VALUES [==[FALSE]==]
      TEST_LIST [==[streamtests_TESTS]==]
      CTEST_FILE [==[C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/tests/streamtests/streamtests[1]_tests.cmake]==]
      TEST_DISCOVERY_TIMEOUT [==[20]==]
      TEST_DISCOVERY_EXTRA_ARGS [==[]==]
      TEST_XML_OUTPUT_DIR [==[]==]
    )
  endif()
  include("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/out/build/x64-Release/_deps/libktx-build/tests/streamtests/streamtests[1]_tests.cmake")
else()
  add_test(streamtests_NOT_BUILT streamtests_NOT_BUILT)
endif()
