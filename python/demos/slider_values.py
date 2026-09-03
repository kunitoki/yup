#!/usr/bin/env python3
"""
YUP Slider Values Demo

Demonstrates different slider types (linear, rotary) with value displays.
Port of popsicle's slider_values.py.
"""

import yup_init
import yup


class SliderValuesComponent(yup.Component):
    def __init__(self):
        yup.Component.__init__(self)

        self.frequencySlider = yup.Slider(yup.SliderType.Rotary)
        self.frequencySlider.setRange(20.0, 20000.0)
        self.frequencySlider.setValue(1000.0)
        self.frequencySlider.setTextBoxStyle(
            yup.TextEntryBoxPosition.TextBoxBelow, False, 60, 20
        )
        self.frequencySlider.onValueChanged = self.onFreqChanged
        self.addAndMakeVisible(self.frequencySlider)

        self.gainSlider = yup.Slider(yup.SliderType.LinearVertical)
        self.gainSlider.setRange(0.0, 1.0)
        self.gainSlider.setValue(0.75)
        self.gainSlider.setTextBoxStyle(
            yup.TextEntryBoxPosition.TextBoxBelow, False, 60, 20
        )
        self.gainSlider.onValueChanged = self.onGainChanged
        self.addAndMakeVisible(self.gainSlider)

        self.freqLabel = yup.Label()
        self.freqLabel.setText(
            "Frequency: 1000.0 Hz", yup.NotificationType.dontSendNotification
        )
        self.addAndMakeVisible(self.freqLabel)

        self.gainLabel = yup.Label()
        self.gainLabel.setText(
            "Gain: 0.75", yup.NotificationType.dontSendNotification
        )
        self.addAndMakeVisible(self.gainLabel)

        self.setOpaque(True)

    def onFreqChanged(self, value: float):
        self.freqLabel.setText(
            f"Frequency: {value:.1f} Hz", yup.NotificationType.dontSendNotification
        )

    def onGainChanged(self, value: float):
        self.gainLabel.setText(
            f"Gain: {value:.2f}", yup.NotificationType.dontSendNotification
        )

    def resized(self):
        bounds = self.getLocalBounds()
        self.frequencySlider.setBounds(20, 20, 100, 100)
        self.gainSlider.setBounds(160, 20, 60, 200)
        self.freqLabel.setBounds(20, 130, 200, 24)
        self.gainLabel.setBounds(160, 230, 200, 24)

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.darkgrey)
        g.fillAll()


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        SliderValuesComponent,
        name="Slider Values",
        width=400,
        height=300,
    )
