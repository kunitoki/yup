#!/usr/bin/env python3
"""
YUP Audio Player with Waveform Demo

Audio file playback with real-time waveform visualization.

Usage:
    python audio_player_waveform.py [path/to/audio/file.wav]
"""

import yup_init
import yup
import sys
import os
import threading


# The peak envelope is reduced to this many buckets, in one pass taken before
# playback starts. The reader is shared with playback - every read seeks its
# stream and allocates, and it is not thread-safe - so reading it from the paint
# thread while the audio thread pulls it makes both stutter. 4096 buckets is more
# resolution than any window width needs.
PEAK_BUCKETS = 4096


def compute_peaks(reader, buckets=PEAK_BUCKETS):
    """Returns the peak level of each bucket across the whole file, in one pass."""
    numSamples = reader.lengthInSamples
    numChannels = reader.numChannels

    if numSamples <= 0 or numChannels <= 0:
        return []

    step = int(max(1, numSamples // buckets))
    chunk = yup.AudioBuffer[float](numChannels, step)

    peaks = []
    for start in range(0, numSamples, step):
        count = min(step, numSamples - start)
        reader.read(chunk, 0, count, start, True, True)
        peaks.append(chunk.getMagnitude(0, 0, count))

    return peaks


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
        self.peaks = []

    def initialise(self) -> str:
        return self.deviceManager.initialise(0, 2, None, True)

    def loadFile(self, filePath: str) -> bool:
        file = yup.File(filePath)
        if not file.existsAsFile():
            return False

        self.transportSource.stop()
        self.readerSource = None
        self.peaks = []

        reader = self.formatManager.createReaderFor(file)
        if reader is None:
            return False

        # Read the envelope here, before playback can start pulling the same reader.
        self.peaks = compute_peaks(reader)

        self.readerSource = yup.AudioFormatReaderSource(reader)
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

    def getPeaks(self):
        """The peak envelope computed when the file was loaded."""
        return self.peaks


class WaveformComponent(yup.Component):
    """Component that displays an audio waveform and playback position."""

    def __init__(self, player: AudioPlayer):
        yup.Component.__init__(self)
        self.player = player
        self.setOpaque(True)

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.black)
        g.fillAll()

        peaks = self.player.getPeaks()
        if not peaks:
            g.setFillColor(yup.Colors.white)
            font = yup.ApplicationTheme.getGlobalTheme().getDefaultFont().withHeight(16.0)
            g.fillFittedText(
                "No audio file loaded",
                font,
                yup.Rectangle[float](0, 0, self.getWidth(), self.getHeight()),
                yup.Justification.center,
            )
            return

        w = self.getWidth()
        h = self.getHeight()

        # Draw the waveform, a column per pixel, without touching the audio file:
        # the envelope was read once, when the file was loaded.
        g.setStrokeColor(yup.Colors.green)
        g.setStrokeWidth(1)

        mid_y = h / 2
        scale = h / 2

        num_peaks = len(peaks)
        for x in range(int(w)):
            first = x * num_peaks // w
            last = max(first + 1, (x + 1) * num_peaks // w)
            y = max(peaks[int(first):int(last)]) * scale
            g.strokeLine(x, mid_y - y, x, mid_y + y)

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
        font = yup.ApplicationTheme.getGlobalTheme().getDefaultFont().withHeight(14.0)
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

            def showWindow():
                yup.Process.makeForegroundProcess()
                self.win.setVisible(True)
                self.win.centreWithSize(yup.Size[int](800, 300))

            yup.MessageManager.callAsync(showWindow)

        def shutdown(self):
            player.stop()

        def systemRequestedQuit(self):
            self.quit()

    yup.START_YUP_APPLICATION(PlayerApp)


if __name__ == "__main__":
    main()
