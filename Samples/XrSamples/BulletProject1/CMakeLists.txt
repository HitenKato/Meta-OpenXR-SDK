# Copyright (c) Meta Platforms, Inc. and affiliates.
# All rights reserved.
# (省略: ライセンス文)

project(bulletproject1)

if(NOT TARGET OpenXR::openxr_loader)
    find_package(OpenXR REQUIRED)
endif()

file(GLOB_RECURSE SRC_FILES
    Src/*.c
    Src/*.cpp
)

if(ANDROID)
    add_library(${PROJECT_NAME} MODULE ${SRC_FILES})
    target_include_directories(${PROJECT_NAME} PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        android
        EGL
        GLESv3
        log
        ktx
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-u ANativeActivity_onCreate")
elseif(WIN32)
    add_definitions(-D_USE_MATH_DEFINES)
    add_executable(${PROJECT_NAME} ${SRC_FILES} "Src/utils.h")
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
target_link_libraries(${PROJECT_NAME} PRIVATE samplexrframework)


# ==========================================
# Bullet Physics Integration
# ==========================================
# Bulletの共有フォルダのパスを相対パスで定義
set(BULLET_SHARED_DIR "${CMAKE_CURRENT_LIST_DIR}/shared")

# 1. ヘッダーファイル (.h) の場所をプロジェクトに教える
target_include_directories(${PROJECT_NAME} PRIVATE "${BULLET_SHARED_DIR}/inc")

# 2. コンパイル済みのライブラリ (.lib) をリンクする (Windows環境のみ)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "${BULLET_SHARED_DIR}/lib/BulletDynamics_vs2010_x64_release.lib"
        "${BULLET_SHARED_DIR}/lib/BulletCollision_vs2010_x64_release.lib"
        "${BULLET_SHARED_DIR}/lib/BulletSoftBody_vs2010_x64_release.lib"
        "${BULLET_SHARED_DIR}/lib/LinearMath_vs2010_x64_release.lib"
    )
endif()