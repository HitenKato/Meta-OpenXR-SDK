# Copyright (c) Meta Platforms, Inc. and affiliates.

project(bulletproject1)

if(NOT TARGET OpenXR::openxr_loader)
    find_package(OpenXR REQUIRED)
endif()

# 1. Bulletのソースコード (.cpp) を自動収集
set(BULLET_SHARED_DIR "${CMAKE_CURRENT_LIST_DIR}/shared")

file(GLOB_RECURSE BULLET_SOURCES
    "${BULLET_SHARED_DIR}/src/*.cpp"
    "${BULLET_SHARED_DIR}/inc/*.cpp"
)

file(GLOB_RECURSE SRC_FILES
    Src/*.c
    Src/*.cpp
)

if(WIN32)
    add_definitions(-D_USE_MATH_DEFINES)
    # Bulletのソースコードをターゲットに組み込む
    add_executable(${PROJECT_NAME} ${SRC_FILES} ${BULLET_SOURCES} "Src/utils.h")
    
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_LIST_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
        VERBATIM)

    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/SampleXrFramework/res/raw"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/font/res/raw"
        VERBATIM)
endif()

# Common across platforms
target_include_directories(${PROJECT_NAME} PRIVATE Src)
target_include_directories(${PROJECT_NAME} PRIVATE "${BULLET_SHARED_DIR}/inc")
target_include_directories(${PROJECT_NAME} PRIVATE "${BULLET_SHARED_DIR}/src")

target_link_libraries(${PROJECT_NAME} PRIVATE samplexrframework)

# ※ 旧.libの target_link_libraries 記述はすべて削除します