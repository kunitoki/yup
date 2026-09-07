#!/usr/bin/env python3
"""
YUP Audio Device Info Demo

Lists available audio devices and their capabilities.
Port of popsicle's audio_device.py.
"""

import yup_init
import yup


def print_device_info():
    """Print information about available audio devices."""
    manager = yup.AudioDeviceManager()

    print("=" * 60)
    print("YUP Audio Device Info")
    print("=" * 60)

    # List device types
    device_types = manager.getAvailableDeviceTypes()
    print(f"\nAvailable device types: {len(device_types)}")
    for i, device_type in enumerate(device_types):
        print(f"\n--- Device Type {i + 1}: {device_type.getTypeName()} ---")

        # Scan for devices of this type
        device_type.scanForDevices()
        device_names = device_type.getDeviceNames()
        print(f"  Devices found: {len(device_names)}")

        for name in device_names:
            print(f"    - {name}")

    # Try to get current device info
    current_device = manager.getCurrentAudioDevice()
    if current_device:
        print(f"\nCurrent Device:")
        print(f"  Name: {current_device.getName()}")
        print(f"  Type: {current_device.getTypeName()}")
        print(f"  Sample Rate: {current_device.getCurrentSampleRate()} Hz")
        print(f"  Buffer Size: {current_device.getCurrentBufferSizeSamples()} samples")
        print(f"  Bit Depth: {current_device.getCurrentBitDepth()}")
        print(f"  Output Channels: {current_device.getActiveOutputChannels()}")
        print(f"  Output Latency: {current_device.getOutputLatencyInSamples()} samples")
        print(f"  Input Latency: {current_device.getInputLatencyInSamples()} samples")

        sample_rates = current_device.getAvailableSampleRates()
        print(f"  Available Sample Rates: {[f'{r:.0f}' for r in sample_rates]}")

        buffer_sizes = current_device.getAvailableBufferSizes()
        print(f"  Available Buffer Sizes: {[str(b) for b in buffer_sizes]}")
    else:
        print("\nNo audio device currently open.")

    print("\n" + "=" * 60)


if __name__ == "__main__":
    print_device_info()
