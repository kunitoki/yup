#!/usr/bin/env python3
"""
YUP Radio Buttons and Checkboxes Demo

Demonstrates ToggleButton as radio buttons and checkboxes.
Port of popsicle's radio_buttons_checkboxes.py.
"""

import yup_init
import yup


class RadioCheckComponent(yup.Component):
    def __init__(self):
        yup.Component.__init__(self)

        self.radio1 = yup.ToggleButton("Option A")
        self.radio1.setToggleState(True, yup.NotificationType.dontSendNotification)
        self.radio1.onClick = lambda: self.onRadioChanged(0)
        self.addAndMakeVisible(self.radio1)

        self.radio2 = yup.ToggleButton("Option B")
        self.radio2.onClick = lambda: self.onRadioChanged(1)
        self.addAndMakeVisible(self.radio2)

        self.radio3 = yup.ToggleButton("Option C")
        self.radio3.onClick = lambda: self.onRadioChanged(2)
        self.addAndMakeVisible(self.radio3)

        self.check1 = yup.ToggleButton("Enable Feature X")
        self.check1.onClick = self.onCheckChanged
        self.addAndMakeVisible(self.check1)

        self.check2 = yup.ToggleButton("Enable Feature Y")
        self.check2.onClick = self.onCheckChanged
        self.addAndMakeVisible(self.check2)

        self.statusLabel = yup.Label()
        self.statusLabel.setText(
            "Selected: Option A | Features: none",
            yup.NotificationType.dontSendNotification,
        )
        self.addAndMakeVisible(self.statusLabel)

        self.setOpaque(True)

    def onRadioChanged(self, index: int):
        self.radio1.setToggleState(index == 0, yup.NotificationType.dontSendNotification)
        self.radio2.setToggleState(index == 1, yup.NotificationType.dontSendNotification)
        self.radio3.setToggleState(index == 2, yup.NotificationType.dontSendNotification)
        options = ["Option A", "Option B", "Option C"]
        self.updateStatus()

    def onCheckChanged(self):
        self.updateStatus()

    def updateStatus(self):
        selected = None
        if self.radio1.getToggleState():
            selected = "Option A"
        elif self.radio2.getToggleState():
            selected = "Option B"
        elif self.radio3.getToggleState():
            selected = "Option C"

        features = []
        if self.check1.getToggleState():
            features.append("X")
        if self.check2.getToggleState():
            features.append("Y")
        feature_str = ", ".join(features) if features else "none"

        self.statusLabel.setText(
            f"Selected: {selected} | Features: {feature_str}",
            yup.NotificationType.dontSendNotification,
        )

    def resized(self):
        y = 20
        self.radio1.setBounds(20, y, 120, 24)
        y += 30
        self.radio2.setBounds(20, y, 120, 24)
        y += 30
        self.radio3.setBounds(20, y, 120, 24)
        y += 50
        self.check1.setBounds(20, y, 160, 24)
        y += 30
        self.check2.setBounds(20, y, 160, 24)
        y += 40
        self.statusLabel.setBounds(20, y, 300, 24)

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.darkgrey)
        g.fillAll()


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        RadioCheckComponent,
        name="Radio Buttons & Checkboxes",
        width=400,
        height=350,
    )
