import yup_init
import yup

from typing import Optional


class MainContentComponent(yup.Component):
    def __init__(self):
        yup.Component.__init__(self)

        self.setOpaque(True)

    def refreshDisplay(self, lastFrameTimeSeconds: float):
        self.repaint()

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.black)
        g.fillAll()

        g.setStrokeWidth(1)

        random = yup.Random.getSystemRandom()
        rect = yup.Rectangle[float](0, 0, 20, 20)

        for _ in range(100):
            rect.setCenter(random.nextFloat() * self.getWidth(), random.nextFloat() * self.getHeight())

            g.setStrokeColor(yup.Color.fromRGBA(
                random.nextInt(255),
                random.nextInt(255),
                random.nextInt(255),
                255))

            g.strokeRect(rect)

    #def mouseDown(self, event: yup.MouseEvent):
    #    print("mouseDown", event)

    #def mouseMove(self, event: yup.MouseEvent):
    #    print("mouseMove", event.position.x, event.position.y)

    #def mouseUp(self, event: yup.MouseEvent):
    #    print("mouseUp", event)


class MainWindow(yup.DocumentWindow):
    component: Optional[yup.Component] = None

    def __init__(self):
        super().__init__()

        self.setTitle(yup.YUPApplication.getInstance().getApplicationName())

        self.component = MainContentComponent()
        self.addAndMakeVisible(self.component)

        #self.setResizable(True, True)
        #self.setContentNonOwned(self.component, True)

    def __del__(self):
        #self.clearContentComponent()
        self.removeAllChildren()

        if self.component:
            del self.component

    def resized(self):
        self.component.setBounds(self.getLocalBounds())

    def userTriedToCloseWindow(self):
        yup.YUPApplication.getInstance().systemRequestedQuit()


class Application(yup.YUPApplication):
    window = None

    def __init__(self):
        super().__init__()

    def getApplicationName(self):
        return "YUP-o-matic"

    def getApplicationVersion(self):
        return "1.0"

    def initialise(self, commandLineParameters: str):
        self.window = MainWindow()

        def showWindow():
            yup.Process.makeForegroundProcess()
            self.window.setVisible(True)
            self.window.centreWithSize(yup.Size[int](800, 600))

        yup.MessageManager.callAsync(showWindow)

    def shutdown(self):
        if self.window:
            del self.window

    def systemRequestedQuit(self):
        self.quit()


if __name__ == "__main__":
    yup.START_YUP_APPLICATION(Application)
