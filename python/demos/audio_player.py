#!/usr/bin/env python3
"""
YUP Audio Player Demo

Demonstrates audio file playback using AudioFormatManager,
AudioFormatReaderSource, and AudioTransportSource.
Port of popsicle's audio_player.py.

Usage:
    python audio_player.py [path/to/audio/file.wav]
"""

import yup_init
import yup
import sys
import os


class AudioPlayer:
    """Simple audio file player."""

    def __init__(self):
        self.deviceManager = yup.AudioDeviceManager()
        self.formatManager = yup.AudioFormatManager()
        self.formatManager.registerDefaultFormats()

        self.transportSource = yup.AudioTransportSource()
        self.player = yup.AudioSourcePlayer()
        self.player.setSource(self.transportSource)

        self.readerSource = None
        self.currentFile = None

    def initialise(self) -> str:
        return self.deviceManager.initialise(0, 2, None, True)

    def loadFile(self, filePath: str) -> bool:
        """Load an audio file for playback."""
        file = yup.File(filePath)
        if not file.existsAsFile():
            print(f"File not found: {filePath}")
            return False

        # Stop any current playback
        self.transportSource.stop()
        self.readerSource = None

        # Create reader and source
        reader = self.formatManager.createReaderFor(file)
        if reader is None:
            print(f"Could not read file: {filePath}")
            return False

        print(f"Loaded: {os.path.basename(filePath)}")
        print(f"  Format: {reader.getFormatName()}")
        print(f"  Sample Rate: {reader.sampleRate} Hz")
        print(f"  Channels: {reader.numChannels}")
        print(f"  Duration: {reader.lengthInSamples / reader.sampleRate:.2f}s")

        self.readerSource = yup.AudioFormatReaderSource(reader, True)
        self.transportSource.setSource(self.readerSource)
        self.currentFile = filePath
        return True

    def play(self):
        """Start or resume playback."""
        if self.readerSource is None:
            print("No file loaded.")
            return

        self.deviceManager.addAudioCallback(self.player)
        self.transportSource.start()
        print("Playing...")

    def stop(self):
        """Stop playback."""
        self.transportSource.stop()
        self.deviceManager.removeAudioCallback(self.player)
        print("Stopped.")

    def getPosition(self) -> float:
        """Get current playback position in seconds."""
        return self.transportSource.getCurrentPosition()

    def getLength(self) -> float:
        """Get total length in seconds."""
        return self.transportSource.getLengthInSeconds()

    def isPlaying(self) -> bool:
        return self.transportSource.isPlaying()


def main():
    player = AudioPlayer()

    result = player.initialise()
    if result:
        print(f"Error initialising audio: {result}")
        return

    # Get file path from arguments or use default
    if len(sys.argv) > 1:
        filePath = sys.argv[1]
    else:
        print("Usage: python audio_player.py <path/to/audio/file>")
        print("\nProvide a WAV, MP3, or other supported audio file.")
        return

    if not player.loadFile(filePath):
        return

    player.play()
    print("\nControls: [space] play/pause, [q] quit")

    import threading
    import time

    running = True

    def input_thread():
        nonlocal running
        while running:
            try:
                cmd = input().strip().lower()
                if cmd == "q":
                    running = False
                elif cmd == "" or cmd == " ":
                    if player.isPlaying():
                        player.transportSource.stop()
                        print("Paused.")
                    else:
                        player.transportSource.start()
                        print("Playing...")
            except EOFError:
                break

    thread = threading.Thread(target=input_thread, daemon=True)
    thread.start()

    try:
        while running:
            if player.isPlaying():
                pos = player.getPosition()
                length = player.getLength()
                bar_width = 40
                filled = int(bar_width * pos / length) if length > 0 else 0
                bar = "#" * filled + "-" * (bar_width - filled)
                print(f"\r[{bar}] {pos:.1f}s / {length:.1f}s", end="")
            time.sleep(0.1)
    except KeyboardInterrupt:
        pass
    finally:
        running = False
        player.stop()
        print("\nDone.")


if __name__ == "__main__":
    main()
