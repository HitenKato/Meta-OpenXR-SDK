# Copyright (c) Meta Platforms, Inc. and affiliates.
# All rights reserved.
#
# Licensed under the Oculus SDK License Agreement (the "License");
# you may not use the Oculus SDK except in compliance with the License,
# which is provided at the time of installation or download, or which
# otherwise accompanies this software in either electronic or hard copy form.
#
# You may obtain a copy of the License at
# https://developer.oculus.com/licenses/oculussdk/
#
# Unless required by applicable law or agreed to in writing, the Oculus SDK
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
cmake_minimum_required(VERSION 3.10.2)

project(MetaOpenXRSDK C CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # 警告をエラーとして扱わない（/WX を打ち消す）
    add_compile_options(/WX-)
    # 細かすぎる非推奨警告・変換警告を無視
    add_compile_options(/wd5272 /wd5287 /wd4996)
endif()

add_subdirectory(3rdParty)
# ...以降の既存コードはそのまま...
add_subdirectory(SampleXrFramework)
add_subdirectory(XrSamples)