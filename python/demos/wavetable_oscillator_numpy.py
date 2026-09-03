#!/usr/bin/env python3
"""
YUP NumPy Wavetable Oscillator Demo

Wavetable synthesis with NumPy for efficient processing.
Uses AudioSource + AudioSourcePlayer (not raw AudioIODeviceCallback).

NOTE: Requires 'numpy' (pip install numpy).
Port of popsicle's wavetable_oscillator_numpy.py.
"""

import yup_init
import yup
import math

try:
    import numpy as np
except ImportError:
    raise ImportError("This demo requires numpy. Install with: pip install numpy")


class WavetableOscillator:
    """A wavetable oscillator using NumPy for efficient vectorised output."""

    def __init__(self, sampleRate: float = 44100.0):
        self.sampleRate = sampleRate
        self.tableSize = 2048
        self.phase = 0.0
        self.frequency = 440.0

        t = np.arange(self.tableSize) / self.tableSize
        self.wavetable = np.sin(2.0 * math.pi * t).astype(np.float32)

    def setFrequency(self, freq: float):
        self.frequency = freq

    def fillBlock(self, numSamples: int, numChannels: int):
        """Generate a block of mono samples, return as numpy array."""
        phaseInc = self.frequency * self.tableSize / self.sampleRate

        # Generate phase ramp
        phases = self.phase + np.arange(numSamples, dtype=np.float64) * phaseInc
        self.phase = (phases[-1] + phaseInc) % self.tableSize

        # Wrap phases
        phases = np.fmod(phases, self.tableSize)

        # Table lookup with linear interpolation
        idx = phases.astype(np.int32)
        frac = (phases - idx).astype(np.float32)
        nextIdx = (idx + 1) % self.tableSize

        samples = (self.wavetable[idx] * (1.0 - frac)
                   + self.wavetable[nextIdx] * frac)

        return samples * 0.3


class WavetableSource(yup.AudioSource):
    """AudioSource wrapping a NumPy wavetable oscillator."""

    def __init__(self):
        yup.AudioSource.__init__(self)
        self.oscillator = None
        self.sampleRate = 44100.0
        self.blockSize = 512

    def prepareToPlay(self, samplesPerBlockExpected: int, sampleRate: float):
        self.sampleRate = sampleRate
        self.blockSize = samplesPerBlockExpected
        self.oscillator = WavetableOscillator(sampleRate)
        print(f"Audio started: {sampleRate:.0f} Hz, block: {samplesPerBlockExpected}")

    def releaseResources(self):
        print("Audio stopped")
        self.oscillator = None

    def getNextAudioBlock(self, bufferToFill):
        if self.oscillator is None:
            bufferToFill.clearActiveBufferRegion()
            return

        samples = self.oscillator.fillBlock(
            bufferToFill.numSamples, bufferToFill.buffer.getNumChannels()
        )
        numCh = bufferToFill.buffer.getNumChannels()

        for ch in range(numCh):
            for s in range(bufferToFill.numSamples):
                bufferToFill.buffer.setSample(ch, s + bufferToFill.startSample, samples[s])


def main():
    manager = yup.AudioDeviceManager()
    result = manager.initialise(0, 2, None, True)
    if result:
        print(f"Error initialising audio: {result}")
        return

    source = WavetableSource()
    player = yup.AudioSourcePlayer()
    player.setSource(source)
    manager.addAudioCallback(player)

    print("Playing 440 Hz sine wave (NumPy wavetable)... Press Enter to stop.")
    input()

    manager.removeAudioCallback(player)
    manager.closeAudioDevice()


if __name__ == "__main__":
    main()
