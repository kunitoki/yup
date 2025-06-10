#include <yup_gui/yup_gui.h>

class MyWindow : public yup::DocumentWindow
{
public:
    MyWindow()
        : yup::DocumentWindow (yup::ComponentNative::Options(), {})
    {
        setTitle ("MyWindow");

        takeFocus();
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (0xffff0000);
        g.fillAll();
    }

    void userTriedToCloseWindow() override
    {
        yup::YUPApplication::getInstance()->systemRequestedQuit();
    }
};

struct MyApplication : yup::YUPApplication
{
    MyApplication() = default;

    const yup::String getApplicationName() override
    {
        return "MyApplication";
    }

    const yup::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const yup::String& commandLineParameters) override
    {
        window = std::make_unique<MyWindow>();
        window->centreWithSize ({ 1080, 2400 });
        window->setVisible (true);
        window->toFront (true);
    }

    void shutdown() override
    {
        window.reset();
    }

private:
    std::unique_ptr<MyWindow> window;
};

START_YUP_APPLICATION (MyApplication)
