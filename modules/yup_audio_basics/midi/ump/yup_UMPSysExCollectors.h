/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#ifndef DOXYGEN

namespace yup::ump
{

class SysEx7Collector
{
public:
    using Callback = std::function<void (const SysEx7&)>;

    explicit SysEx7Collector (Callback cb)
        : callback (std::move (cb))
    {
    }

    void setCallback (Callback cb) { callback = std::move (cb); }

    void setMaxSysExDataSize (size_t maxSize)
    {
        if ((maxSysExDataSize = maxSize))
            sysex.data.reserve (maxSysExDataSize);
    }

    void feed (const UniversalPacket& packet)
    {
        if (! isSysEx7Packet (packet))
            return;

        const auto view = SysEx7PacketView { packet };

        switch (DataStatus (view.getStatus()))
        {
            case DataStatus::sysex7_complete:
            case DataStatus::sysex7_start:
                if (state != Status (DataStatus::sysex7_start))
                    reset();
                break;
            default:
                if (state != Status (DataStatus::sysex7_continue))
                {
                    reset();
                    return;
                }
                break;
        }

        const auto numBytes = view.getPayloadSize();
        if (sysex.data.size() + numBytes > sysex.data.capacity())
        {
            const bool limitedSize = (maxSysExDataSize > 0);
            size_t newCapacity = std::max (size_t { 128 }, 2 * sysex.data.capacity());
            if (limitedSize)
                newCapacity = std::min (newCapacity, maxSysExDataSize);

            sysex.data.reserve (newCapacity);

            if (limitedSize && (sysex.data.size() + numBytes > sysex.data.capacity()))
            {
                state = Status (DataStatus::sysex7_start);
                return;
            }
        }

        for (size_t b = 0; b < numBytes; ++b)
        {
            const auto byte = view.getPayloadByte (b);
            switch (manufacturerIdBytesRead)
            {
                case 0:
                    if (byte != 0)
                    {
                        sysex.manufacturerId = ManufacturerId (byte) << 16;
                        manufacturerIdBytesRead = 3;
                    }
                    else
                    {
                        manufacturerIdBytesRead = 1;
                    }
                    break;
                case 1:
                    sysex.manufacturerId = ManufacturerId (byte) << 8;
                    manufacturerIdBytesRead = 2;
                    break;
                case 2:
                    sysex.manufacturerId |= ManufacturerId (byte);
                    manufacturerIdBytesRead = 3;
                    break;
                default:
                    sysex.data.push_back (byte);
                    break;
            }
        }

        switch (DataStatus (view.getStatus()))
        {
            case DataStatus::sysex7_complete:
            case DataStatus::sysex7_end:
                if (callback)
                    callback (sysex);
                reset();
                break;
            default:
                state = Status (DataStatus::sysex7_continue);
                break;
        }
    }

    void reset()
    {
        sysex.clear();
        state = Status (DataStatus::sysex7_start);
        manufacturerIdBytesRead = 0;
    }

private:
    SysEx7 sysex;
    size_t maxSysExDataSize { 0 };
    Status state { Status (DataStatus::sysex7_start) };
    uint8_t manufacturerIdBytesRead { 0 };
    Callback callback;
};

class SysEx8Collector
{
public:
    using Callback = std::function<void (const SysEx8&, uint8_t streamId)>;

    explicit SysEx8Collector (Callback cb)
        : callback (std::move (cb))
    {
    }

    void setCallback (Callback cb) { callback = std::move (cb); }

    void setMaxSysExDataSize (size_t maxSize)
    {
        if ((maxSysExDataSize = maxSize))
            sysex.data.reserve (maxSysExDataSize);
    }

    void feed (const UniversalPacket& packet)
    {
        if (! isSysEx8Packet (packet))
            return;

        const auto view = SysEx8PacketView { packet };

        switch (view.getFormat())
        {
            case PacketFormat::complete:
            case PacketFormat::start:
                if (state != PacketFormat::start)
                    reset();
                streamId = view.getStreamId();
                break;
            default:
                if (state != PacketFormat::cont)
                {
                    reset();
                    return;
                }
                if (view.getStreamId() != streamId)
                    return;
                break;
        }

        const auto numBytes = view.getPayloadSize();
        if (sysex.data.size() + numBytes > sysex.data.capacity())
        {
            const bool limitedSize = (maxSysExDataSize > 0);
            size_t newCapacity = std::max (size_t { 128 }, 2 * sysex.data.capacity());
            if (limitedSize)
                newCapacity = std::min (newCapacity, maxSysExDataSize);

            sysex.data.reserve (newCapacity);

            if (limitedSize && (sysex.data.size() + numBytes > sysex.data.capacity()))
            {
                state = PacketFormat::start;
                return;
            }
        }

        for (size_t b = 0; b < numBytes; ++b)
        {
            const auto byte = view.getPayloadByte (b);
            switch (manufacturerIdState)
            {
                case ManufacturerIdState::detect:
                    if (byte & 0x80)
                    {
                        sysex.manufacturerId = ManufacturerId ((byte & 0x7f) << 8);
                        manufacturerIdState = ManufacturerIdState::threeBytes;
                    }
                    else
                    {
                        sysex.manufacturerId = 0;
                        manufacturerIdState = (byte == 0) ? ManufacturerIdState::oneByte
                                                          : ManufacturerIdState::invalid;
                    }
                    break;
                case ManufacturerIdState::oneByte:
                    sysex.manufacturerId = ManufacturerId ((byte & 0x7f) << 16);
                    manufacturerIdState = ManufacturerIdState::done;
                    break;
                case ManufacturerIdState::threeBytes:
                    sysex.manufacturerId |= ManufacturerId (byte & 0x7f);
                    manufacturerIdState = ManufacturerIdState::done;
                    break;
                case ManufacturerIdState::invalid:
                    manufacturerIdState = ManufacturerIdState::done;
                    break;
                default:
                    sysex.data.push_back (byte);
                    break;
            }
        }

        switch (view.getFormat())
        {
            case PacketFormat::complete:
            case PacketFormat::end:
                if (callback)
                    callback (sysex, streamId);
                reset();
                break;
            default:
                state = PacketFormat::cont;
                break;
        }
    }

    void reset()
    {
        sysex.clear();
        streamId = 0;
        state = PacketFormat::start;
        manufacturerIdState = ManufacturerIdState::detect;
    }

    uint8_t getStreamId() const { return streamId; }

private:
    enum class ManufacturerIdState
    {
        detect,
        oneByte,
        threeBytes,
        invalid,
        done
    };

    uint8_t streamId { 0 };
    SysEx8 sysex;
    size_t maxSysExDataSize { 0 };
    PacketFormat state { PacketFormat::start };
    ManufacturerIdState manufacturerIdState { ManufacturerIdState::detect };
    Callback callback;
};

} // namespace yup::ump

#endif
