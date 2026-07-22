# Copyright (c) Meta Platforms, Inc. and affiliates.

project(bulletproject1)

if(NOT TARGET OpenXR::openxr_loader)
    find_package(OpenXR REQUIRED)
endif()

# Bullet の必要な主要モジュール (CPU版) の .cpp のみを限定して自動収集
set(BULLET_SHARED_DIR "${CMAKE_CURRENT_LIST_DIR}/shared")

file(GLOB_RECURSE BULLET_SOURCES
    "${BULLET_SHARED_DIR}/src/BulletCollision/*.cpp"
    "${BULLET_SHARED_DIR}/src/BulletDynamics/*.cpp"
    "${BULLET_SHARED_DIR}/src/LinearMath/*.cpp"
    # ※もし src ではなく inc 側にソースがある場合は以下
    "${BULLET_SHARED_DIR}/inc/BulletCollision/*.cpp"
    "${BULLET_SHARED_DIR}/inc/BulletDynamics/*.cpp"
    "${BULLET_SHARED_DIR}/inc/LinearMath/*.cpp"
)

file(GLOB_RECURSE SRC_FILES
    Src/*.c
    Src/*.cpp
)

if(WIN32)
    add_definitions(-D_USE_MATH_DEFINES)
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

# インクルードパスの設定
target_include_directories(${PROJECT_NAME} PRIVATE Src)
target_include_directories(${PROJECT_NAME} PRIVATE "${BULLET_SHARED_DIR}/inc")
target_include_directories(${PROJECT_NAME} PRIVATE "${BULLET_SHARED_DIR}/src")

target_link_libraries(${PROJECT_NAME} PRIVATE samplexrframework)