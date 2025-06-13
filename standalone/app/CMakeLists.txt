cmake_minimum_required (VERSION 3.28)

set (target_name my_app)
set (target_version "1.0.0")
project (${target_name} VERSION ${target_version})

include (FetchContent)

FetchContent_Declare(
  yup
  GIT_REPOSITORY https://github.com/kunitoki/yup.git
  GIT_TAG        main)

set (YUP_BUILD_EXAMPLES OFF)
set (YUP_BUILD_TESTS OFF)
FetchContent_MakeAvailable(yup)

yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "MyApp"
    TARGET_APP_ID "my.company.${target_name}"
    TARGET_APP_NAMESPACE "my.company"
    INITIAL_MEMORY 268435456
    MODULES yup::yup_gui)

if (NOT YUP_TARGET_ANDROID)
    file (GLOB sources "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
    source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
    target_sources (${target_name} PRIVATE ${sources})
endif()
