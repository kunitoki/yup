#!/usr/bin/env python3
"""
YUP Wavetable Oscillator Demo

Generates a sine wave using a wavetable oscillator.
Pure Python version (no NumPy required). Uses AudioSource + AudioSourcePlayer
instead of raw AudioIODeviceCallback (which can't be overridden from Python
due to raw float** pointer marshalling limits).

Port of popsicle's wavetable_oscillator.py.
"""

import yup_init
import yup
import math


class SineWaveOscillator:
    """A simple sine wave wavetable oscillator."""

    def __init__(self, sampleRate: float = 44100.0, frequency: float = 440.0):
        self.sampleRate = sampleRate
        self.tableSize = 1024
        self.phase = 0.0
        self.phaseIncrement = 0.0
        self.wavetable = [0.0] * self.tableSize

        self._buildTable()
        self.setFrequency(frequency)

    def _buildTable(self):
        for i in range(self.tableSize):
            self.wavetable[i] = math.sin(2.0 * math.pi * i / self.tableSize)

    def setFrequency(self, frequency: float):
        self.phaseIncrement = frequency * self.tableSize / self.sampleRate

    def getNextSample(self) -> float:
        index = int(self.phase)
        frac = self.phase - index
        nextIndex = (index + 1) % self.tableSize
        sample = (self.wavetable[index] * (1.0 - frac)
                  + self.wavetable[nextIndex] * frac)
        self.phase += self.phaseIncrement
        while self.phase >= self.tableSize:
            self.phase -= self.tableSize
        return sample

    def fillBuffer(self, buffer, numSamples: int, numChannels: int, gain: float = 0.3):
        """Fill an AudioBuffer with oscillator output."""
        for sample in range(numSamples):
            value = self.getNextSample() * gain
            for channel in range(numChannels):
                buffer.setSample(channel, sample, value)


class SineWaveSource(yup.AudioSource):
    """AudioSource wrapping a sine wave oscillator."""

    def __init__(self):
        yup.AudioSource.__init__(self)
        self.oscillator = None
        self.sampleRate = 44100.0
        self.blockSize = 512

    def prepareToPlay(self, samplesPerBlockExpected: int, sampleRate: float):
        self.sampleRate = sampleRate
        self.blockSize = samplesPerBlockExpected
        self.oscillator = SineWaveOscillator(sampleRate, 440.0)
        print(f"Audio started: {sampleRate:.0f} Hz, block: {samplesPerBlockExpected}")

    def releaseResources(self):
        print("Audio stopped")
        self.oscillator = None

    def getNextAudioBlock(self, bufferToFill):
        if self.oscillator is None:
            bufferToFill.clearActiveBufferRegion()
            return

        self.oscillator.fillBuffer(
            bufferToFill.buffer,
            bufferToFill.numSamples,
            bufferToFill.buffer.getNumChannels(),
        )


def main():
    manager = yup.AudioDeviceManager()
    result = manager.initialise(0, 2, None, True)
    if result:
        print(f"Error initialising audio: {result}")
        return

    source = SineWaveSource()
    player = yup.AudioSourcePlayer()
    player.setSource(source)
    manager.addAudioCallback(player)

    print("Playing 440 Hz sine wave... Press Enter to stop.")
    input()

    manager.removeAudioCallback(player)
    manager.closeAudioDevice()


if __name__ == "__main__":
    main()
