#!/usr/bin/env python3
"""
YUP Slider Decibels Demo

Demonstrates a slider with decibel-range mapping for audio gain control.
Port of popsicle's slider_decibels.py.
"""

import yup_init
import yup


class DecibelSliderComponent(yup.Component):
    def __init__(self):
        yup.Component.__init__(self)

        self.gainSlider = yup.Slider(yup.SliderType.Rotary)
        self.gainSlider.setRange(-96.0, 12.0, 0.1)
        self.gainSlider.setValue(0.0)
        self.gainSlider.setSkewFactorFromMidpoint(-12.0)
        self.gainSlider.setNumDecimalPlacesToDisplay(1)
        self.gainSlider.setTextBoxStyle(
            yup.TextEntryBoxPosition.TextBoxBelow, False, 60, 20
        )
        self.gainSlider.onValueChanged = self.onSliderChanged
        self.addAndMakeVisible(self.gainSlider)

        self.valueLabel = yup.Label()
        self.valueLabel.setText(
            "0.0 dB", yup.NotificationType.dontSendNotification
        )
        self.addAndMakeVisible(self.valueLabel)

        self.linearLabel = yup.Label()
        self.linearLabel.setText(
            "Linear: 1.000", yup.NotificationType.dontSendNotification
        )
        self.addAndMakeVisible(self.linearLabel)

        self.setOpaque(True)

    def onSliderChanged(self, value: float):
        self.valueLabel.setText(
            f"{value:.1f} dB", yup.NotificationType.dontSendNotification
        )
        linear = yup.Decibels.decibelsToGain(value)
        self.linearLabel.setText(
            f"Linear: {linear:.3f}", yup.NotificationType.dontSendNotification
        )

    def resized(self):
        self.gainSlider.setBounds(20, 20, 120, 120)
        self.valueLabel.setBounds(160, 40, 200, 24)
        self.linearLabel.setBounds(160, 70, 200, 24)

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.darkgrey)
        g.fillAll()


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        DecibelSliderComponent,
        name="Decibel Slider",
        width=400,
        height=200,
    )
