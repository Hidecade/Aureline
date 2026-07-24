#include "App/MainComponent.h"

#include <juce_gui_extra/juce_gui_extra.h>

class AurelineApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Aureline"; }
    const juce::String getApplicationVersion() override { return "1.0.1"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        window = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override { window.reset(); }
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted(const juce::String&) override {}

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(juce::String name)
            : DocumentWindow(std::move(name), juce::Colour(0xff17140f),
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new AurelineMainComponent(), true);
            centreWithSize(getWidth(), getHeight());
            setResizable(true, true);
            setResizeLimits(820, 535, 1536, 1002);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> window;
};

START_JUCE_APPLICATION(AurelineApplication)
