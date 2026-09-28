#include <yup_gui/yup_gui.h>

class Application
    : public yup::YUPApplication
    , public yup::Timer
{
public:
    yup::String getApplicationName() override {
        return "yup app!";
    }

    yup::String getApplicationVersion() override {
        return "1.0";
    }

    void initialise (const yup::String& commandLineParameters) override {
        yup::Logger::outputDebugString (
            "Starting app " + commandLineParameters);

        startTimer (1000);
    }

    void shutdown() override {
        yup::Logger::outputDebugString ("Shutting down");
    }

    void timerCallback() override {
        stopTimer();

        yup::MessageManager::callAsync ([this] { systemRequestedQuit(); });
    }
};

START_YUP_APPLICATION (Application)
