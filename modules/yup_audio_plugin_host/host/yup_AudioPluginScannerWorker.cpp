/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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


namespace yup
{

namespace
{

/** Scan requests and replies exchanged by the scanner process and the worker process. */
struct AudioPluginScanProtocol
{
    static constexpr int32 version = 1;

    struct Request
    {
        AudioPluginFormatType formatType = AudioPluginFormatType::unknown;
        String path;
    };

    struct Reply
    {
        bool ok = false;
        String errorMessage;
        std::vector<AudioPluginDescription> descriptions;
    };

    static MemoryBlock writeRequest (const Request& request)
    {
        MemoryOutputStream stream;
        stream.writeInt (version);
        stream.writeInt (static_cast<int> (request.formatType));
        stream.writeString (request.path);
        return stream.getMemoryBlock();
    }

    static std::optional<Request> readRequest (const MemoryBlock& block)
    {
        MemoryInputStream stream (block, false);

        if (stream.readInt() != version)
            return std::nullopt;

        Request request;
        request.formatType = static_cast<AudioPluginFormatType> (stream.readInt());
        request.path = stream.readString();
        return request;
    }

    static MemoryBlock writeReply (const Reply& reply)
    {
        MemoryOutputStream stream;
        stream.writeInt (version);
        stream.writeBool (reply.ok);
        stream.writeString (reply.errorMessage);
        stream.writeInt (static_cast<int> (reply.descriptions.size()));

        for (const auto& description : reply.descriptions)
            AudioPluginBridgeProtocol::writeDescription (stream, description);

        return stream.getMemoryBlock();
    }

    static std::optional<Reply> readReply (const MemoryBlock& block)
    {
        MemoryInputStream stream (block, false);

        if (stream.readInt() != version)
            return std::nullopt;

        Reply reply;
        reply.ok = stream.readBool();
        reply.errorMessage = stream.readString();

        const auto numDescriptions = stream.readInt();
        if (numDescriptions < 0)
            return std::nullopt;

        for (int i = 0; i < numDescriptions && ! stream.isExhausted(); ++i)
            reply.descriptions.push_back (AudioPluginBridgeProtocol::readDescription (stream));

        if (static_cast<int> (reply.descriptions.size()) != numDescriptions)
            return std::nullopt;

        return reply;
    }
};

} // namespace

//==============================================================================

AudioPluginScannerWorker::AudioPluginScannerWorker() = default;

AudioPluginScannerWorker::~AudioPluginScannerWorker() = default;

void AudioPluginScannerWorker::addFormat (std::unique_ptr<AudioPluginFormat> format)
{
    jassert (format != nullptr);
    if (format == nullptr)
        return;

    for (auto& existing : formats)
    {
        if (existing->getFormatType() == format->getFormatType())
        {
            existing = std::move (format);
            return;
        }
    }

    formats.push_back (std::move (format));
}

void AudioPluginScannerWorker::setHostContext (const AudioPluginHostContext& newContext)
{
    context = newContext;
}

bool AudioPluginScannerWorker::initialiseFromCommandLine (const String& commandLine, const String& commandLineUniqueID)
{
    if (! ChildProcessWorker::initialiseFromCommandLine (commandLine, commandLineUniqueID))
        return false;

#if YUP_MAC
    Process::setDockIconVisible (false);
#endif

    return true;
}

void AudioPluginScannerWorker::handleMessageFromCoordinator (const MemoryBlock& message)
{
    MessageManager::callAsync ([this, message]
    {
        scanOnMessageThread (message);
    });
}

void AudioPluginScannerWorker::handleConnectionLost()
{
    // The scanner is gone or gave up on a hanging scan: the message thread may be stuck in a plugin
    Process::terminate();
}

void AudioPluginScannerWorker::scanOnMessageThread (const MemoryBlock& message)
{
    AudioPluginScanProtocol::Reply reply;

    if (const auto request = AudioPluginScanProtocol::readRequest (message))
    {
        const auto format = std::find_if (formats.begin(), formats.end(), [&] (const auto& f)
        {
            return f->getFormatType() == request->formatType;
        });

        if (format == formats.end())
        {
            reply.errorMessage = "Format not supported by the scanner worker";
        }
        else
        {
            auto result = (*format)->scanFileFully (File (request->path), context);
            reply.ok = result.wasOk();

            if (result.wasOk())
                reply.descriptions = std::move (result).getValue();
            else
                reply.errorMessage = result.getErrorMessage();
        }
    }
    else
    {
        reply.errorMessage = "Malformed scan request";
    }

    sendMessageToCoordinator (AudioPluginScanProtocol::writeReply (reply));
}

} // namespace yup
