#!/usr/bin/env python3
"""
YUP Audio Player with Waveform Demo

Audio file playback with real-time waveform visualization.
Port of popsicle's audio_player_waveform.py.

Usage:
    python audio_player_waveform.py [path/to/audio/file.wav]
"""

import yup_init
import yup
import sys
import os
import threading


class AudioPlayer:
    """Simple audio file player (shared logic with audio_player.py)."""

    def __init__(self):
        self.deviceManager = yup.AudioDeviceManager()
        self.formatManager = yup.AudioFormatManager()
        self.formatManager.registerDefaultFormats()

        self.transportSource = yup.AudioTransportSource()
        self.player = yup.AudioSourcePlayer()
        self.player.setSource(self.transportSource)

        self.readerSource = None

    def initialise(self) -> str:
        return self.deviceManager.initialise(0, 2, None, True)

    def loadFile(self, filePath: str) -> bool:
        file = yup.File(filePath)
        if not file.existsAsFile():
            return False

        self.transportSource.stop()
        self.readerSource = None

        reader = self.formatManager.createReaderFor(file)
        if reader is None:
            return False

        self.readerSource = yup.AudioFormatReaderSource(reader, True)
        self.transportSource.setSource(self.readerSource)
        return True

    def play(self):
        if self.readerSource:
            self.deviceManager.addAudioCallback(self.player)
            self.transportSource.start()

    def stop(self):
        self.transportSource.stop()
        self.deviceManager.removeAudioCallback(self.player)

    def isPlaying(self) -> bool:
        return self.transportSource.isPlaying()

    def getPosition(self) -> float:
        return self.transportSource.getCurrentPosition()

    def getLength(self) -> float:
        return self.transportSource.getLengthInSeconds()

    def getReader(self):
        """Get the underlying format reader for waveform analysis."""
        if self.readerSource:
            return self.readerSource.getAudioFormatReader()
        return None


class WaveformComponent(yup.Component):
    """Component that displays an audio waveform and playback position."""

    def __init__(self, player: AudioPlayer):
        yup.Component.__init__(self)
        self.player = player
        self.setOpaque(True)

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.black)
        g.fillAll()

        reader = self.player.getReader()
        if reader is None:
            g.setFillColor(yup.Colors.white)
            font = yup.Font(yup.FontOptions(16.0))
            g.fillFittedText(
                "No audio file loaded",
                font,
                yup.Rectangle[float](0, 0, self.getWidth(), self.getHeight()),
                yup.Justification.centred,
            )
            return

        w = self.getWidth()
        h = self.getHeight()

        # Draw waveform
        numSamples = reader.lengthInSamples
        numChannels = reader.numChannels
        sampleRate = reader.sampleRate

        # Downsample to fit the width
        downsample = max(1, numSamples // w)
        g.setStrokeColor(yup.Colors.green)
        g.setStrokeWidth(1)

        mid_y = h / 2
        scale = h / 2

        # Read samples and draw waveform
        buffer = yup.AudioBuffer[float](numChannels, downsample)
        x = 0.0
        for i in range(0, numSamples, downsample):
            samples_to_read = min(downsample, numSamples - i)
            reader.read(buffer, 0, samples_to_read, i, True, True)

            # Find peak in this chunk
            peak = 0.0
            for s in range(samples_to_read):
                peak = max(peak, abs(buffer.getSample(0, s)))

            y = peak * scale
            g.strokeLine(x, mid_y - y, x, mid_y + y)
            x += 1.0

        # Draw playback position
        pos = self.player.getPosition()
        length = self.player.getLength()
        if length > 0:
            pos_x = (pos / length) * w
            g.setStrokeColor(yup.Colors.red)
            g.setStrokeWidth(2)
            g.strokeLine(pos_x, 0, pos_x, h)

        # Draw time info
        g.setFillColor(yup.Colors.white)
        font = yup.Font(yup.FontOptions(14.0))
        time_str = f"{pos:.1f}s / {length:.1f}s"
        g.fillFittedText(
            time_str,
            font,
            yup.Rectangle[float](10, h - 30, 200, 20),
            yup.Justification.left,
        )

    def refreshDisplay(self, lastFrameTimeSeconds: float):
        self.repaint()


def main():
    if len(sys.argv) < 2:
        print("Usage: python audio_player_waveform.py <path/to/audio/file>")
        return

    filePath = sys.argv[1]
    player = AudioPlayer()
    result = player.initialise()
    if result:
        print(f"Error: {result}")
        return

    if not player.loadFile(filePath):
        print(f"Could not load: {filePath}")
        return

    player.play()

    # Create and show the waveform component
    class PlayerApp(yup.YUPApplication):
        def getApplicationName(self):
            return "Audio Player"

        def getApplicationVersion(self):
            return "1.0"

        def initialise(self, cmdLine):
            class Win(yup.DocumentWindow):
                def __init__(self):
                    super().__init__()
                    self.setTitle("Audio Player")
                    self.comp = WaveformComponent(player)
                    self.addAndMakeVisible(self.comp)

                def resized(self):
                    self.comp.setBounds(self.getLocalBounds())

                def userTriedToCloseWindow(self):
                    yup.YUPApplication.getInstance().systemRequestedQuit()

            self.win = Win()
            self.win.setVisible(True)
            self.win.centreWithSize(yup.Size[int](800, 300))

        def shutdown(self):
            player.stop()

        def systemRequestedQuit(self):
            self.quit()

    yup.START_YUP_APPLICATION(PlayerApp)


if __name__ == "__main__":
    main()
