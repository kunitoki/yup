/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#ifndef DOXYGEN

namespace juce::universal_midi_packets
{
/**
        Allows conversion from bytestream- or Universal MIDI Packet-formatted
        messages to MIDI 1.0 messages in UMP format.

        @tags{Audio}
    */
struct ToUMP1Converter
{
    template <typename Fn>
    void convert (const BytestreamMidiView& m, Fn&& fn)
    {
        Conversion::toMidi1 (m, std::forward<Fn> (fn));
    }

    template <typename Fn>
    void convert (const View& v, Fn&& fn)
    {
        Conversion::midi2ToMidi1DefaultTranslation (v, std::forward<Fn> (fn));
    }
};

/**
        Allows conversion from bytestream- or Universal MIDI Packet-formatted
        messages to MIDI 2.0 messages in UMP format.

        @tags{Audio}
    */
struct ToUMP2Converter
{
    template <typename Fn>
    void convert (const BytestreamMidiView& m, Fn&& fn)
    {
        Conversion::toMidi1 (m, [&] (const View& v)
                             {
                                 translator.dispatch (v, fn);
                             });
    }

    template <typename Fn>
    void convert (const View& v, Fn&& fn)
    {
        translator.dispatch (v, std::forward<Fn> (fn));
    }

    void reset()
    {
        translator.reset();
    }

    Midi1ToMidi2DefaultTranslator translator;
};

/**
        Allows conversion from bytestream- or Universal MIDI Packet-formatted
        messages to UMP format.

        The packet protocol can be selected using the constructor parameter.

        @tags{Audio}
    */
class GenericUMPConverter
{
    template <typename This, typename... Args>
    static void visit (This& t, Args&&... args)
    {
        if (t.mode == PacketProtocol::MIDI_1_0)
            convertImpl (std::get<0> (t.converters), std::forward<Args> (args)...);
        else
            convertImpl (std::get<1> (t.converters), std::forward<Args> (args)...);
    }

public:
    explicit GenericUMPConverter (PacketProtocol m)
        : mode (m)
    {
    }

    void reset()
    {
        std::get<1> (converters).reset();
    }

    template <typename Converter, typename Fn>
    static void convertImpl (Converter& converter, const BytestreamMidiView& m, Fn&& fn)
    {
        converter.convert (m, std::forward<Fn> (fn));
    }

    template <typename Converter, typename Fn>
    static void convertImpl (Converter& converter, const View& m, Fn&& fn)
    {
        converter.convert (m, std::forward<Fn> (fn));
    }

    template <typename Converter, typename Fn>
    static void convertImpl (Converter& converter, Iterator b, Iterator e, Fn&& fn)
    {
        std::for_each (b, e, [&] (const auto& v)
                       {
                           convertImpl (converter, v, fn);
                       });
    }

    template <typename Fn>
    void convert (const BytestreamMidiView& m, Fn&& fn)
    {
        visit (*this, m, std::forward<Fn> (fn));
    }

    template <typename Fn>
    void convert (const View& v, Fn&& fn)
    {
        visit (*this, v, std::forward<Fn> (fn));
    }

    template <typename Fn>
    void convert (Iterator begin, Iterator end, Fn&& fn)
    {
        visit (*this, begin, end, std::forward<Fn> (fn));
    }

    PacketProtocol getProtocol() const noexcept { return mode; }

private:
    std::tuple<ToUMP1Converter, ToUMP2Converter> converters;
    const PacketProtocol mode {};
};

/**
        Allows conversion from bytestream- or Universal MIDI Packet-formatted
        messages to bytestream format.

        @tags{Audio}
    */
struct ToBytestreamConverter
{
    explicit ToBytestreamConverter (int storageSize)
        : translator (storageSize)
    {
    }

    template <typename Fn>
    void convert (const MidiMessage& m, Fn&& fn)
    {
        fn (m);
    }

    template <typename Fn>
    void convert (const View& v, double time, Fn&& fn)
    {
        Conversion::midi2ToMidi1DefaultTranslation (v, [&] (const View& midi1)
                                                    {
                                                        translator.dispatch (midi1, time, fn);
                                                    });
    }

    void reset() { translator.reset(); }

    Midi1ToBytestreamTranslator translator;
};
} // namespace juce::universal_midi_packets

#endif
