# CMakeLists.txt -- Build system for palimpsest
#
# Copyright 2022 Stéphane Caron
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

find_package(Threads REQUIRED)

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(DictionaryDifference DictionaryDifference.cpp)
target_include_directories(DictionaryDifference PUBLIC ../third_party/cppcodec)
target_link_libraries(DictionaryDifference PUBLIC Eigen3::Eigen Threads::Threads gtest gtest_main mpack palimpsest)
gtest_discover_tests(DictionaryDifference)

add_executable(DictionaryTest DictionaryTest.cpp)
target_include_directories(DictionaryTest PUBLIC ../third_party/cppcodec)
target_link_libraries(DictionaryTest PUBLIC Eigen3::Eigen Threads::Threads gtest gtest_main mpack palimpsest)
gtest_discover_tests(DictionaryTest)
