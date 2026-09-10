#!/usr/bin/env python3
"""
YUP FlexBox & Grid Layout Demo

Demonstrates CSS-style FlexBox and Grid layout engines for
arranging components in a window.
Port of popsicle's layout_flexgrid.py.
"""

import yup_init
import yup


class LayoutFlexGridComponent(yup.Component):
    """Demonstrates FlexBox and Grid layouts."""

    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)

        # Child components
        self.header = yup.Label("Header")
        self.header.setOpaque(True)
        self.header.setColor(yup.Label.Style.backgroundColorId, yup.Colors.darkblue)
        self.header.setColor(yup.Label.Style.textFillColorId, yup.Colors.white)
        self.header.setText("FlexBox & Grid Layout Demo",
                            yup.NotificationType.dontSendNotification)
        self.addAndMakeVisible(self.header)

        self.sidebarLeft = yup.Label("Sidebar Left")
        self.sidebarLeft.setOpaque(True)
        self.sidebarLeft.setColor(yup.Label.Style.backgroundColorId, yup.Colors.darkgreen)
        self.sidebarLeft.setColor(yup.Label.Style.textFillColorId, yup.Colors.white)
        self.sidebarLeft.setText("Sidebar\nLeft",
                                 yup.NotificationType.dontSendNotification)
        self.addAndMakeVisible(self.sidebarLeft)

        self.sidebarRight = yup.Label("Sidebar Right")
        self.sidebarRight.setOpaque(True)
        self.sidebarRight.setColor(yup.Label.Style.backgroundColorId, yup.Colors.darkgreen)
        self.sidebarRight.setColor(yup.Label.Style.textFillColorId, yup.Colors.white)
        self.sidebarRight.setText("Sidebar\nRight",
                                  yup.NotificationType.dontSendNotification)
        self.addAndMakeVisible(self.sidebarRight)

        self.content = yup.Label("Content")
        self.content.setOpaque(True)
        self.content.setColor(yup.Label.Style.backgroundColorId, yup.Colors.darkgray)
        self.content.setColor(yup.Label.Style.textFillColorId, yup.Colors.white)
        self.content.setText("Main Content Area",
                             yup.NotificationType.dontSendNotification)
        self.addAndMakeVisible(self.content)

        self.footer = yup.Label("Footer")
        self.footer.setOpaque(True)
        self.footer.setColor(yup.Label.Style.backgroundColorId, yup.Colors.darkred)
        self.footer.setColor(yup.Label.Style.textFillColorId, yup.Colors.white)
        self.footer.setText("Footer - Status Bar",
                            yup.NotificationType.dontSendNotification)
        self.addAndMakeVisible(self.footer)

    def resized(self):
        bounds = self.getLocalBounds()

        # Use FlexBox for the main layout (column direction)
        flex = yup.FlexBox(
            yup.FlexBox.Direction.column,
            yup.FlexBox.Wrap.noWrap,
            yup.FlexBox.AlignItems.stretch,
            yup.FlexBox.JustifyContent.flexStart,
            yup.FlexBox.AlignContent.stretch,
        )
        flex.gap = 4

        flex.items.add(
            yup.FlexItem(self.header, 0, 40)
            .withMinHeight(30)
        )
        flex.items.add(
            yup.FlexItem(self.content, 0, 0)
            .withFlex(1.0)
            .withMinHeight(100)
        )

        # Body area: use a nested FlexBox for sidebar-content-sidebar
        bodyFlex = yup.FlexBox(
            yup.FlexBox.Direction.row,
            yup.FlexBox.Wrap.noWrap,
            yup.FlexBox.AlignItems.stretch,
            yup.FlexBox.JustifyContent.flexStart,
            yup.FlexBox.AlignContent.stretch,
        )
        bodyFlex.gap = 4
        bodyFlex.items.add(
            yup.FlexItem(self.sidebarLeft, 120, 0)
            .withMinWidth(80)
        )
        bodyFlex.items.add(yup.FlexItem(self.content, 0, 0).withFlex(1.0))
        bodyFlex.items.add(
            yup.FlexItem(self.sidebarRight, 120, 0)
            .withMinWidth(80)
        )

        flex.items.add(
            yup.FlexItem(self.footer, 0, 30)
            .withMinHeight(25)
        )

        flex.performLayout(bounds)

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.black)
        g.fillAll()


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        LayoutFlexGridComponent,
        name="FlexBox & Grid Layout",
        width=800,
        height=500,
    )
