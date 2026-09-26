cmake_minimum_required (VERSION 3.31)
project (my_app VERSION 1.0.0)

include (FetchContent)

FetchContent_Declare (yup
    GIT_REPOSITORY https://github.com/kunitoki/yup.git
    GIT_TAG        main)

set (YUP_BUILD_EXAMPLES OFF)
set (YUP_BUILD_TESTS OFF)
FetchContent_MakeAvailable (yup)

yup_standalone_app (
    TARGET_NAME my_app
    TARGET_VERSION 1.0.0
    TARGET_IDE_GROUP "MyApp"
    TARGET_APP_NAMESPACE "com.mycompany"
    TARGET_CXX_STANDARD 20
    MODULES
        yup::yup_gui
        yup::yup_audio_devices)

target_sources (my_app PRIVATE main.cpp)
