#!/usr/bin/env python3
"""
YUP NumPy Audio Demo

Generates modulated white noise using NumPy for efficient DSP.
Uses AudioSource + AudioSourcePlayer.

NOTE: Requires 'numpy' (pip install numpy).
Port of popsicle's numpy_audio.py.
"""

import yup_init
import yup
import math

try:
    import numpy as np
except ImportError:
    raise ImportError("This demo requires numpy. Install with: pip install numpy")


class NoiseSource(yup.AudioSource):
    """AudioSource generating modulated white noise with NumPy."""

    def __init__(self):
        yup.AudioSource.__init__(self)
        self.sampleRate = 44100.0
        self.phase = 0.0
        self.gain = 0.1

    def prepareToPlay(self, samplesPerBlockExpected: int, sampleRate: float):
        self.sampleRate = sampleRate
        print(f"Audio started: {sampleRate:.0f} Hz")

    def releaseResources(self):
        print("Audio stopped")

    def getNextAudioBlock(self, bufferToFill):
        n = bufferToFill.numSamples
        numCh = bufferToFill.buffer.getNumChannels()

        # Generate noise with NumPy
        noise = np.random.uniform(-1.0, 1.0, n).astype(np.float32)
        noise *= self.gain

        # Slow amplitude modulation
        t = np.arange(n) / self.sampleRate + self.phase
        noise *= 0.5 + 0.5 * np.sin(2.0 * math.pi * 0.5 * t)

        self.phase += n / self.sampleRate

        # Write to output buffer
        for ch in range(numCh):
            for s in range(n):
                bufferToFill.buffer.setSample(ch, s + bufferToFill.startSample, noise[s])


def main():
    manager = yup.AudioDeviceManager()
    result = manager.initialise(0, 2, None, True)
    if result:
        print(f"Error initialising audio: {result}")
        return

    source = NoiseSource()
    player = yup.AudioSourcePlayer()
    player.setSource(source)
    manager.addAudioCallback(player)

    print("Playing modulated white noise... Press Enter to stop.")
    input()

    manager.removeAudioCallback(player)
    manager.closeAudioDevice()


if __name__ == "__main__":
    main()
