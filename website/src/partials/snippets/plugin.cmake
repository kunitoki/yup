# After FetchContent_MakeAvailable (yup)
yup_audio_plugin (
    TARGET_NAME my_plugin
    TARGET_VERSION 1.0.0
    TARGET_IDE_GROUP "MyPlugin"
    TARGET_APP_ID "com.mycompany.my_plugin"
    TARGET_APP_NAMESPACE "com.mycompany"
    TARGET_CXX_STANDARD 20
    PLUGIN_ID "com.mycompany.MyPlugin"
    PLUGIN_NAME "MyPlugin"
    PLUGIN_VENDOR "com.mycompany"
    PLUGIN_VERSION "1.0.0"
    PLUGIN_IS_SYNTH OFF
    PLUGIN_CREATE_CLAP ON
    PLUGIN_CREATE_VST3 ON
    PLUGIN_CREATE_AU ON
    PLUGIN_CREATE_STANDALONE ON
    MODULES
        yup::yup_gui
        yup::yup_audio_processors)

target_sources (my_plugin_shared PUBLIC plugin.cpp)
