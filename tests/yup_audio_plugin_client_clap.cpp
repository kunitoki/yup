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

#include <gtest/gtest.h>

#include <algorithm>

// =============================================================================
#define YUP_AUDIO_PLUGIN_ENABLE_CLAP 1
#define YupPlugin_Id "test.clap.plugin"
#define YupPlugin_Name "Test CLAP Plugin"
#define YupPlugin_Vendor "TestVendor"
#define YupPlugin_Version "1.0.0"
#define YupPlugin_Description "Test CLAP wrapper for yup"
#define YupPlugin_URL "https://test.example"
#define YupPlugin_Email "test@example.com"
#define YupPlugin_IsSynth 0
#define YupPlugin_IsMono 0

// =============================================================================
#include <yup_audio_processors/yup_audio_processors.h>
#include "yup_audio_plugin_client/yup_TestPluginProcessor.h"

#define YUP_AUDIO_PLUGIN_CREATE_FUNCTION createPluginProcessorCLAP
#include "yup_audio_plugin_client/clap/yup_audio_plugin_client_CLAP.cpp"

extern "C" yup::AudioProcessor* createPluginProcessorCLAP()
{
    return new TestPluginProcessor (testPluginBusLayoutWithSidechain());
}

// =============================================================================
// Tests
// =============================================================================

using namespace yup;

namespace
{

// Minimal clap_host_t — same zero-extension pattern used during CLAP scanning
clap_host_t makeMinimalHost()
{
    clap_host_t host {};
    host.clap_version = CLAP_VERSION;
    host.host_data = nullptr;
    host.name = "TestHost";
    host.vendor = "TestVendor";
    host.url = "";
    host.version = "0.0.0";
    host.get_extension = [] (const clap_host_t*, const char*) -> const void*
    {
        return nullptr;
    };
    host.request_restart = [] (const clap_host_t*) {};
    host.request_process = [] (const clap_host_t*) {};
    host.request_callback = [] (const clap_host_t*) {};
    return host;
}

} // namespace

//------------------------------------------------------------------------------
// Factory tests
//------------------------------------------------------------------------------

TEST (CLAPWrapperTest, EntryPointProvidesFactory)
{
    // clap_entry is the extern "C" symbol exported by the wrapper
    const auto* factory = static_cast<const clap_plugin_factory_t*> (
        clap_entry.get_factory (CLAP_PLUGIN_FACTORY_ID));
    ASSERT_NE (nullptr, factory);
}

TEST (CLAPWrapperTest, FactoryReturnsNullForWrongID)
{
    const auto* result = clap_entry.get_factory ("wrong.factory.id");
    EXPECT_EQ (nullptr, result);
}

TEST (CLAPWrapperTest, FactoryReportsOnePlugin)
{
    const auto* factory = static_cast<const clap_plugin_factory_t*> (
        clap_entry.get_factory (CLAP_PLUGIN_FACTORY_ID));
    ASSERT_NE (nullptr, factory);

    EXPECT_EQ (1u, factory->get_plugin_count (factory));
    EXPECT_NE (nullptr, factory->get_plugin_descriptor (factory, 0));
    EXPECT_EQ (nullptr, factory->get_plugin_descriptor (factory, 1));
}

TEST (CLAPWrapperTest, DescriptorMatchesMetadata)
{
    const auto* factory = static_cast<const clap_plugin_factory_t*> (
        clap_entry.get_factory (CLAP_PLUGIN_FACTORY_ID));
    ASSERT_NE (nullptr, factory);

    const auto* desc = factory->get_plugin_descriptor (factory, 0);
    ASSERT_NE (nullptr, desc);
    EXPECT_STREQ ("test.clap.plugin", desc->id);
    EXPECT_STREQ ("Test CLAP Plugin", desc->name);
    EXPECT_STREQ ("TestVendor", desc->vendor);
}

TEST (CLAPWrapperTest, CreatePluginReturnsNullForWrongID)
{
    auto host = makeMinimalHost();
    const auto* factory = static_cast<const clap_plugin_factory_t*> (
        clap_entry.get_factory (CLAP_PLUGIN_FACTORY_ID));
    ASSERT_NE (nullptr, factory);

    const auto* plugin = factory->create_plugin (factory, &host, "wrong.id");
    EXPECT_EQ (nullptr, plugin);
}

TEST (CLAPWrapperTest, CreatePluginSucceeds)
{
    auto host = makeMinimalHost();
    const auto* factory = static_cast<const clap_plugin_factory_t*> (
        clap_entry.get_factory (CLAP_PLUGIN_FACTORY_ID));
    ASSERT_NE (nullptr, factory);

    const auto* plugin = factory->create_plugin (factory, &host, "test.clap.plugin");
    ASSERT_NE (nullptr, plugin);

    // Clean up
    plugin->destroy (plugin);
}

//------------------------------------------------------------------------------
// Lifecycle tests
//------------------------------------------------------------------------------

class CLAPLifecycleTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        host = makeMinimalHost();
        const auto* factory = static_cast<const clap_plugin_factory_t*> (
            clap_entry.get_factory (CLAP_PLUGIN_FACTORY_ID));
        plugin = factory->create_plugin (factory, &host, "test.clap.plugin");
        ASSERT_NE (nullptr, plugin);
    }

    void TearDown() override
    {
        if (plugin != nullptr)
            plugin->destroy (plugin);
    }

    clap_host_t host;
    const clap_plugin_t* plugin = nullptr;
};

TEST_F (CLAPLifecycleTests, InitSucceeds)
{
    EXPECT_TRUE (plugin->init (plugin));
}

TEST_F (CLAPLifecycleTests, ActivateAndDeactivate)
{
    ASSERT_TRUE (plugin->init (plugin));
    EXPECT_TRUE (plugin->activate (plugin, 44100.0, 512, 1024));
    plugin->deactivate (plugin);
}

//------------------------------------------------------------------------------
// Parameter extension tests
//------------------------------------------------------------------------------

class CLAPParamsTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        host = makeMinimalHost();
        const auto* factory = static_cast<const clap_plugin_factory_t*> (
            clap_entry.get_factory (CLAP_PLUGIN_FACTORY_ID));
        plugin = factory->create_plugin (factory, &host, "test.clap.plugin");
        ASSERT_NE (nullptr, plugin);
        ASSERT_TRUE (plugin->init (plugin));

        paramsExt = static_cast<const clap_plugin_params_t*> (
            plugin->get_extension (plugin, CLAP_EXT_PARAMS));
    }

    void TearDown() override
    {
        if (plugin != nullptr)
            plugin->destroy (plugin);
    }

    clap_host_t host;
    const clap_plugin_t* plugin = nullptr;
    const clap_plugin_params_t* paramsExt = nullptr;
};

TEST_F (CLAPParamsTests, ExtensionIsAvailable)
{
    ASSERT_NE (nullptr, paramsExt);
}

TEST_F (CLAPParamsTests, CountIncludesBypassParameter)
{
    ASSERT_NE (nullptr, paramsExt);
    // 4 processor parameters + 1 synthetic bypass = 5
    EXPECT_EQ (5u, paramsExt->count (plugin));
}

TEST_F (CLAPParamsTests, GetInfoForFloatParameter)
{
    ASSERT_NE (nullptr, paramsExt);

    clap_param_info_t info {};
    EXPECT_TRUE (paramsExt->get_info (plugin, 0, &info));

    EXPECT_EQ (100u, info.id); // host ID
    EXPECT_EQ (static_cast<uint32_t> (CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE),
               info.flags & (CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE));
}

TEST_F (CLAPParamsTests, GetInfoForSteppedEnumParameter)
{
    ASSERT_NE (nullptr, paramsExt);

    clap_param_info_t info {};
    EXPECT_TRUE (paramsExt->get_info (plugin, 1, &info));

    EXPECT_EQ (200u, info.id);
    EXPECT_NE (0u, info.flags & CLAP_PARAM_IS_STEPPED);
    EXPECT_EQ (4u, info.max_value - info.min_value); // 0 to 4
}

TEST_F (CLAPParamsTests, GetInfoForReadOnlyParameter)
{
    ASSERT_NE (nullptr, paramsExt);

    clap_param_info_t info {};
    EXPECT_TRUE (paramsExt->get_info (plugin, 2, &info));

    EXPECT_EQ (300u, info.id);
    EXPECT_EQ (0u, info.flags & CLAP_PARAM_IS_AUTOMATABLE);
}

TEST_F (CLAPParamsTests, GetValueAndValueToText)
{
    ASSERT_NE (nullptr, paramsExt);

    double value = 0.0;
    EXPECT_TRUE (paramsExt->get_value (plugin, 100, &value));

    char buffer[64] {};
    EXPECT_TRUE (paramsExt->value_to_text (plugin, 100, value, buffer, sizeof (buffer)));
    EXPECT_GT (std::strlen (buffer), 0u);
}

TEST_F (CLAPParamsTests, TextToValueRoundTrip)
{
    ASSERT_NE (nullptr, paramsExt);

    char buffer[64] {};
    ASSERT_TRUE (paramsExt->value_to_text (plugin, 100, 0.5, buffer, sizeof (buffer)));

    double parsed = 0.0;
    EXPECT_TRUE (paramsExt->text_to_value (plugin, 100, buffer, &parsed));
    EXPECT_NEAR (0.5, parsed, 0.01);
}

TEST_F (CLAPParamsTests, BypassParameterIsLast)
{
    ASSERT_NE (nullptr, paramsExt);

    const uint32_t count = paramsExt->count (plugin);
    ASSERT_GE (count, 1u);

    clap_param_info_t info {};
    EXPECT_TRUE (paramsExt->get_info (plugin, count - 1, &info));

    EXPECT_NE (0u, info.flags & CLAP_PARAM_IS_BYPASS);
}

//------------------------------------------------------------------------------
// Bypass parameter handling tests
//------------------------------------------------------------------------------

namespace
{

// Minimal CLAP input event list holding a single event (or none when null)
struct SingleInputEventList final : clap_input_events_t
{
    explicit SingleInputEventList (const clap_event_header_t* eventToDeliver)
        : event (eventToDeliver)
    {
        this->ctx = this;
        this->size = [] (const clap_input_events_t* list) -> uint32_t
        {
            return static_cast<const SingleInputEventList*> (list)->event != nullptr ? 1u : 0u;
        };
        this->get = [] (const clap_input_events_t* list, uint32_t index) -> const clap_event_header_t*
        {
            return index == 0 ? static_cast<const SingleInputEventList*> (list)->event : nullptr;
        };
    }

    const clap_event_header_t* event = nullptr;
};

// Minimal memory-backed CLAP output stream
struct MemoryOStream final : clap_ostream_t
{
    MemoryOStream()
    {
        this->ctx = this;
        this->write = [] (const clap_ostream_t* stream, const void* buffer, uint64_t size) -> int64_t
        {
            auto* self = static_cast<MemoryOStream*> (stream->ctx);
            self->data.append (buffer, static_cast<size_t> (size));
            return static_cast<int64_t> (size);
        };
    }

    yup::MemoryBlock data;
};

// Minimal memory-backed CLAP input stream
struct MemoryIStream final : clap_istream_t
{
    explicit MemoryIStream (const yup::MemoryBlock& source)
        : data (source)
    {
        this->ctx = this;
        this->read = [] (const clap_istream_t* stream, void* buffer, uint64_t size) -> int64_t
        {
            auto* self = static_cast<MemoryIStream*> (stream->ctx);
            const auto remaining = self->data.getSize() - self->position;
            const auto bytesToRead = std::min<uint64_t> (size, remaining);

            if (bytesToRead > 0)
                std::memcpy (buffer, static_cast<const char*> (self->data.getData()) + self->position, static_cast<size_t> (bytesToRead));

            self->position += static_cast<size_t> (bytesToRead);
            return static_cast<int64_t> (bytesToRead);
        };
    }

    yup::MemoryBlock data;
    size_t position = 0;
};

} // namespace

class CLAPBypassTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        host = makeMinimalHost();
        createPlugin();
    }

    void TearDown() override
    {
        if (plugin != nullptr)
            plugin->destroy (plugin);
    }

    void createPlugin()
    {
        const auto* factory = static_cast<const clap_plugin_factory_t*> (
            clap_entry.get_factory (CLAP_PLUGIN_FACTORY_ID));
        plugin = factory->create_plugin (factory, &host, "test.clap.plugin");
        ASSERT_NE (nullptr, plugin);
        ASSERT_TRUE (plugin->init (plugin));

        processor = static_cast<TestPluginProcessor*> (getWrapper (plugin)->getProcessor());
        ASSERT_NE (nullptr, processor);

        paramsExt = static_cast<const clap_plugin_params_t*> (
            plugin->get_extension (plugin, CLAP_EXT_PARAMS));
        ASSERT_NE (nullptr, paramsExt);

        stateExt = static_cast<const clap_plugin_state_t*> (
            plugin->get_extension (plugin, CLAP_EXT_STATE));
        ASSERT_NE (nullptr, stateExt);

        // The wrapper-owned bypass parameter is always the last one
        const auto count = paramsExt->count (plugin);
        ASSERT_GE (count, 1u);

        clap_param_info_t info {};
        ASSERT_TRUE (paramsExt->get_info (plugin, count - 1, &info));
        bypassParameterID = info.id;
    }

    // Runs the plugin's process function with valid audio buffers so the wrapper
    // reaches processAudioBlock. Optionally delivers a bypass parameter change.
    void runProcess (const clap_event_header_t* event = nullptr)
    {
        SingleInputEventList inputEvents (event);

        float mainInputData[2][64] = {};
        float sidechainInputData[1][64] = {};
        float mainOutputData[2][64] = {};

        float* mainInputChannels[2] = { mainInputData[0], mainInputData[1] };
        float* sidechainInputChannels[1] = { sidechainInputData[0] };
        float* mainOutputChannels[2] = { mainOutputData[0], mainOutputData[1] };

        clap_audio_buffer_t mainInput {};
        mainInput.data32 = mainInputChannels;
        mainInput.channel_count = 2;

        clap_audio_buffer_t sidechainInput {};
        sidechainInput.data32 = sidechainInputChannels;
        sidechainInput.channel_count = 1;

        clap_audio_buffer_t mainOutput {};
        mainOutput.data32 = mainOutputChannels;
        mainOutput.channel_count = 2;

        clap_audio_buffer_t inputs[] = { mainInput, sidechainInput };
        clap_audio_buffer_t outputs[] = { mainOutput };

        clap_process_t process {};
        process.frames_count = 64;
        process.in_events = &inputEvents;
        process.audio_inputs = inputs;
        process.audio_inputs_count = 2;
        process.audio_outputs = outputs;
        process.audio_outputs_count = 1;

        EXPECT_EQ (CLAP_PROCESS_CONTINUE, plugin->process (plugin, &process));
    }

    void sendBypassValue (double value)
    {
        clap_event_param_value_t paramEvent {};
        paramEvent.header.size = sizeof (paramEvent);
        paramEvent.header.time = 0;
        paramEvent.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        paramEvent.header.type = CLAP_EVENT_PARAM_VALUE;
        paramEvent.header.flags = 0;
        paramEvent.param_id = bypassParameterID;
        paramEvent.cookie = nullptr;
        paramEvent.note_id = -1;
        paramEvent.port_index = -1;
        paramEvent.channel = -1;
        paramEvent.key = -1;
        paramEvent.value = value;

        runProcess (reinterpret_cast<const clap_event_header_t*> (&paramEvent));
    }

    clap_host_t host;
    const clap_plugin_t* plugin = nullptr;
    const clap_plugin_params_t* paramsExt = nullptr;
    const clap_plugin_state_t* stateExt = nullptr;
    TestPluginProcessor* processor = nullptr;
    clap_id bypassParameterID = CLAP_INVALID_ID;
};

TEST_F (CLAPBypassTests, DefaultsToNotBypassed)
{
    double value = 1.0;
    ASSERT_TRUE (paramsExt->get_value (plugin, bypassParameterID, &value));
    EXPECT_DOUBLE_EQ (0.0, value);
}

TEST_F (CLAPBypassTests, ValueToTextAndTextToValue)
{
    char buffer[64] {};
    ASSERT_TRUE (paramsExt->value_to_text (plugin, bypassParameterID, 1.0, buffer, sizeof (buffer)));
    EXPECT_STREQ ("On", buffer);

    ASSERT_TRUE (paramsExt->value_to_text (plugin, bypassParameterID, 0.0, buffer, sizeof (buffer)));
    EXPECT_STREQ ("Off", buffer);

    double value = 0.0;
    ASSERT_TRUE (paramsExt->text_to_value (plugin, bypassParameterID, "On", &value));
    EXPECT_DOUBLE_EQ (1.0, value);

    ASSERT_TRUE (paramsExt->text_to_value (plugin, bypassParameterID, "Off", &value));
    EXPECT_DOUBLE_EQ (0.0, value);
}

TEST_F (CLAPBypassTests, BypassEventRoutesToBypassedPath)
{
    sendBypassValue (1.0);

    EXPECT_EQ (1, processor->bypassCallCount);
    EXPECT_EQ (0, processor->processCallCount);

    double value = 0.0;
    ASSERT_TRUE (paramsExt->get_value (plugin, bypassParameterID, &value));
    EXPECT_DOUBLE_EQ (1.0, value);
}

TEST_F (CLAPBypassTests, NonBypassEventRoutesToProcessPath)
{
    sendBypassValue (0.0);

    EXPECT_EQ (0, processor->bypassCallCount);
    EXPECT_EQ (1, processor->processCallCount);

    double value = 1.0;
    ASSERT_TRUE (paramsExt->get_value (plugin, bypassParameterID, &value));
    EXPECT_DOUBLE_EQ (0.0, value);
}

TEST_F (CLAPBypassTests, StateRoundTripsBypassState)
{
    sendBypassValue (1.0);
    EXPECT_EQ (1, processor->bypassCallCount);

    MemoryOStream out;
    ASSERT_TRUE (stateExt->save (plugin, &out));
    ASSERT_GT (out.data.getSize(), 0u);

    // Recreate the plugin and restore the saved state
    plugin->destroy (plugin);
    plugin = nullptr;

    createPlugin();

    MemoryIStream in (out.data);
    ASSERT_TRUE (stateExt->load (plugin, &in));

    double value = 0.0;
    ASSERT_TRUE (paramsExt->get_value (plugin, bypassParameterID, &value));
    EXPECT_DOUBLE_EQ (1.0, value);

    // A fresh process call without bypass events must keep routing to the
    // bypass path because the loaded state restored the wrapper's bypass flag
    runProcess();
    EXPECT_EQ (1, processor->bypassCallCount);
    EXPECT_EQ (0, processor->processCallCount);
}

//------------------------------------------------------------------------------
// State extension tests
//------------------------------------------------------------------------------

class CLAPStateTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        host = makeMinimalHost();
        const auto* factory = static_cast<const clap_plugin_factory_t*> (
            clap_entry.get_factory (CLAP_PLUGIN_FACTORY_ID));
        plugin = factory->create_plugin (factory, &host, "test.clap.plugin");
        ASSERT_NE (nullptr, plugin);
        ASSERT_TRUE (plugin->init (plugin));

        stateExt = static_cast<const clap_plugin_state_t*> (
            plugin->get_extension (plugin, CLAP_EXT_STATE));
    }

    void TearDown() override
    {
        if (plugin != nullptr)
            plugin->destroy (plugin);
    }

    clap_host_t host;
    const clap_plugin_t* plugin = nullptr;
    const clap_plugin_state_t* stateExt = nullptr;
};

TEST_F (CLAPStateTests, ExtensionIsAvailable)
{
    ASSERT_NE (nullptr, stateExt);
}

//------------------------------------------------------------------------------
// Audio ports extension tests
//------------------------------------------------------------------------------

class CLAPAudioPortsTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        host = makeMinimalHost();
        const auto* factory = static_cast<const clap_plugin_factory_t*> (
            clap_entry.get_factory (CLAP_PLUGIN_FACTORY_ID));
        plugin = factory->create_plugin (factory, &host, "test.clap.plugin");
        ASSERT_NE (nullptr, plugin);
        ASSERT_TRUE (plugin->init (plugin));

        portsExt = static_cast<const clap_plugin_audio_ports_t*> (
            plugin->get_extension (plugin, CLAP_EXT_AUDIO_PORTS));
    }

    void TearDown() override
    {
        if (plugin != nullptr)
            plugin->destroy (plugin);
    }

    clap_host_t host;
    const clap_plugin_t* plugin = nullptr;
    const clap_plugin_audio_ports_t* portsExt = nullptr;
};

TEST_F (CLAPAudioPortsTests, ExtensionIsAvailable)
{
    ASSERT_NE (nullptr, portsExt);
}

TEST_F (CLAPAudioPortsTests, InputCountMatchesLayout)
{
    ASSERT_NE (nullptr, portsExt);
    // 2 input buses (Main Input + Sidechain Input) → 2 ports
    EXPECT_EQ (2u, portsExt->count (plugin, true));
}

TEST_F (CLAPAudioPortsTests, OutputCountMatchesLayout)
{
    ASSERT_NE (nullptr, portsExt);
    // 1 output bus with 2 channels → 1 port
    EXPECT_EQ (1u, portsExt->count (plugin, false));
}

//------------------------------------------------------------------------------
// Note ports extension tests
//------------------------------------------------------------------------------

class CLAPNotePortsTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        host = makeMinimalHost();
        const auto* factory = static_cast<const clap_plugin_factory_t*> (
            clap_entry.get_factory (CLAP_PLUGIN_FACTORY_ID));
        plugin = factory->create_plugin (factory, &host, "test.clap.plugin");
        ASSERT_NE (nullptr, plugin);
        ASSERT_TRUE (plugin->init (plugin));

        notePortsExt = static_cast<const clap_plugin_note_ports_t*> (
            plugin->get_extension (plugin, CLAP_EXT_NOTE_PORTS));
    }

    void TearDown() override
    {
        if (plugin != nullptr)
            plugin->destroy (plugin);
    }

    clap_host_t host;
    const clap_plugin_t* plugin = nullptr;
    const clap_plugin_note_ports_t* notePortsExt = nullptr;
};

TEST_F (CLAPNotePortsTests, ExtensionIsAvailable)
{
    ASSERT_NE (nullptr, notePortsExt);
}

TEST_F (CLAPNotePortsTests, ReportsZeroPortsForNonSynth)
{
    ASSERT_NE (nullptr, notePortsExt);
    EXPECT_EQ (0u, notePortsExt->count (plugin, true));
    EXPECT_EQ (0u, notePortsExt->count (plugin, false));
}

//------------------------------------------------------------------------------
// Latency and tail tests
//------------------------------------------------------------------------------

class CLAPLatencyTailTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        host = makeMinimalHost();
        const auto* factory = static_cast<const clap_plugin_factory_t*> (
            clap_entry.get_factory (CLAP_PLUGIN_FACTORY_ID));
        plugin = factory->create_plugin (factory, &host, "test.clap.plugin");
        ASSERT_NE (nullptr, plugin);
        ASSERT_TRUE (plugin->init (plugin));

        latencyExt = static_cast<const clap_plugin_latency_t*> (
            plugin->get_extension (plugin, CLAP_EXT_LATENCY));
        tailExt = static_cast<const clap_plugin_tail_t*> (
            plugin->get_extension (plugin, CLAP_EXT_TAIL));
    }

    void TearDown() override
    {
        if (plugin != nullptr)
            plugin->destroy (plugin);
    }

    clap_host_t host;
    const clap_plugin_t* plugin = nullptr;
    const clap_plugin_latency_t* latencyExt = nullptr;
    const clap_plugin_tail_t* tailExt = nullptr;
};

TEST_F (CLAPLatencyTailTests, ExtensionsAreAvailable)
{
    ASSERT_NE (nullptr, latencyExt);
    ASSERT_NE (nullptr, tailExt);
}

TEST_F (CLAPLatencyTailTests, LatencyIsZero)
{
    ASSERT_NE (nullptr, latencyExt);
    EXPECT_EQ (0u, latencyExt->get (plugin));
}

TEST_F (CLAPLatencyTailTests, TailIsZero)
{
    ASSERT_NE (nullptr, tailExt);
    EXPECT_EQ (0u, tailExt->get (plugin));
}

//------------------------------------------------------------------------------
// Sidechain audio port tests (AudioBus::Role → CLAP_AUDIO_PORT_IS_MAIN)
//------------------------------------------------------------------------------

TEST_F (CLAPAudioPortsTests, MainAudioPortHasIsMainFlag)
{
    ASSERT_NE (nullptr, portsExt);

    const auto numInputs = portsExt->count (plugin, true);
    ASSERT_GE (numInputs, 1u);

    clap_audio_port_info_t info {};
    ASSERT_TRUE (portsExt->get (plugin, 0, true, &info));

    EXPECT_NE (0u, info.flags & CLAP_AUDIO_PORT_IS_MAIN);
    EXPECT_EQ (2u, info.channel_count); // Main Input = 2 channels
}

TEST_F (CLAPAudioPortsTests, AuxiliaryAudioPortLacksIsMainFlag)
{
    ASSERT_NE (nullptr, portsExt);

    const auto numInputs = portsExt->count (plugin, true);
    ASSERT_GE (numInputs, 2u);

    clap_audio_port_info_t info {};
    ASSERT_TRUE (portsExt->get (plugin, 1, true, &info));

    EXPECT_EQ (0u, info.flags & CLAP_AUDIO_PORT_IS_MAIN);
    EXPECT_EQ (1u, info.channel_count); // Sidechain Input = 1 channel
}

TEST_F (CLAPAudioPortsTests, OutputPortHasIsMainFlag)
{
    ASSERT_NE (nullptr, portsExt);

    const auto numOutputs = portsExt->count (plugin, false);
    ASSERT_GE (numOutputs, 1u);

    clap_audio_port_info_t info {};
    ASSERT_TRUE (portsExt->get (plugin, 0, false, &info));

    EXPECT_NE (0u, info.flags & CLAP_AUDIO_PORT_IS_MAIN);
    EXPECT_EQ (2u, info.channel_count); // Main Output = 2 channels
}

TEST_F (CLAPAudioPortsTests, SidechainPortNameIsExposed)
{
    ASSERT_NE (nullptr, portsExt);

    const auto numInputs = portsExt->count (plugin, true);
    ASSERT_GE (numInputs, 2u);

    clap_audio_port_info_t info {};
    ASSERT_TRUE (portsExt->get (plugin, 1, true, &info));

    // The port name should be non-empty
    EXPECT_NE (0, info.name[0]);
}
