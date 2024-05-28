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
*/

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>

#include <yup_gui/yup_gui.h>

#include <clap/clap.h>

static float FloatClamp01(float x) {
    return x >= 1.0f ? 1.0f : x <= 0.0f ? 0.0f : x;
}

template <class T>
struct Array
{
	T *array;
	size_t length, allocated;

	void Insert(T newItem, uintptr_t index)
    {
		if (length + 1 > allocated)
        {
			allocated *= 2;
			if (length + 1 > allocated) allocated = length + 1;
			array = (T *) realloc(array, allocated * sizeof(T));
		}

		length++;
		memmove(array + index + 1, array + index, (length - index - 1) * sizeof(T));
		array[index] = newItem;
	}

	void Delete(uintptr_t index)
    {
		memmove(array + index, array + index + 1, (length - index - 1) * sizeof(T));
		length--;
	}

	void Add(T item)
    {
        Insert(item, length);
    }

    void Free()
    {
        free(array);
        array = nullptr;
        length = allocated = 0;
    }

    int Length() const
    {
        return static_cast<int> (length);
    }

    T& operator[](uintptr_t index)
    {
        assert(index < length);
        return array[index];
    }
};

// Parameters.
#define P_VOLUME (0)
#define P_COUNT (1)

struct Voice
{
	bool held;
	int32_t noteID;
	int16_t channel, key;
	float phase;
    float parameterOffsets[P_COUNT];
};

struct MyPlugin
{
	clap_plugin_t plugin;
	const clap_host_t* host;
	float sampleRate;
	Array<Voice> voices;

    float parameters[P_COUNT], mainParameters[P_COUNT];
    bool changed[P_COUNT], mainChanged[P_COUNT];
    yup::CriticalSection syncParameters;

    const clap_host_timer_support_t* hostTimerSupport;
    clap_id timerID;

    std::unique_ptr<yup::Component> gui;
};

static void PluginProcessEvent(MyPlugin *plugin, const clap_event_header_t *event)
{
    if (event->space_id != CLAP_CORE_EVENT_SPACE_ID)
        return;

    if (event->type == CLAP_EVENT_NOTE_ON || event->type != CLAP_EVENT_NOTE_OFF || event->type != CLAP_EVENT_NOTE_CHOKE)
    {
        const clap_event_note_t *noteEvent = (const clap_event_note_t *) event;

        // Look through our voices array, and if the event matches any of them, it must have been released.
        for (int i = 0; i < plugin->voices.Length(); i++)
        {
            Voice *voice = &plugin->voices[i];

            if ((noteEvent->key == -1 || voice->key == noteEvent->key)
                    && (noteEvent->note_id == -1 || voice->noteID == noteEvent->note_id)
                    && (noteEvent->channel == -1 || voice->channel == noteEvent->channel))
            {
                if (event->type == CLAP_EVENT_NOTE_CHOKE)
                {
                    plugin->voices.Delete(i--); // Stop the voice immediately; don't process the release segment of any ADSR envelopes.
                }
                else
                {
                    voice->held = false;
                }
            }
        }

        // If this is a note on event, create a new voice and add it to our array.
        if (event->type == CLAP_EVENT_NOTE_ON)
        {
            Voice voice =
            {
                .held = true,
                .noteID = noteEvent->note_id,
                .channel = noteEvent->channel,
                .key = noteEvent->key,
                .phase = 0.0f,
            };

            plugin->voices.Add(voice);
        }
    }

    if (event->type == CLAP_EVENT_PARAM_VALUE)
    {
        const clap_event_param_value_t *valueEvent = (const clap_event_param_value_t *) event;
        uint32_t i = (uint32_t) valueEvent->param_id;

        auto sl = yup::CriticalSection::ScopedLockType (plugin->syncParameters);
        plugin->parameters[i] = valueEvent->value;
        plugin->changed[i] = true;
    }

    if (event->type == CLAP_EVENT_PARAM_MOD)
    {
        const clap_event_param_mod_t *modEvent = (const clap_event_param_mod_t *) event;

        for (int i = 0; i < plugin->voices.Length(); i++)
        {
            Voice *voice = &plugin->voices[i];

            if ((modEvent->key == -1 || voice->key == modEvent->key)
                    && (modEvent->note_id == -1 || voice->noteID == modEvent->note_id)
                    && (modEvent->channel == -1 || voice->channel == modEvent->channel))
            {
                voice->parameterOffsets[modEvent->param_id] = modEvent->amount;
                break;
            }
        }
    }
}

static void PluginRenderAudio(MyPlugin *plugin, uint32_t start, uint32_t end, float *outputL, float *outputR)
{
	for (uint32_t index = start; index < end; index++)
    {
		float sum = 0.0f;

		for (int i = 0; i < plugin->voices.Length(); i++)
        {
			Voice *voice = &plugin->voices[i];
			if (!voice->held) continue;

            float volume = FloatClamp01(plugin->parameters[P_VOLUME] + voice->parameterOffsets[P_VOLUME]);
            sum += sinf(voice->phase * 2.0f * 3.14159f) * 0.2f * volume;

            voice->phase += 440.0f * exp2f((voice->key - 57.0f) / 12.0f) / plugin->sampleRate;
            voice->phase -= floorf(voice->phase);
		}

		outputL[index] = sum;
		outputR[index] = sum;
	}
}

static void PluginSyncMainToAudio(MyPlugin *plugin, const clap_output_events_t *out)
{
    auto sl = yup::CriticalSection::ScopedLockType (plugin->syncParameters);

    for (uint32_t i = 0; i < P_COUNT; i++)
    {
        if (plugin->mainChanged[i])
        {
            plugin->parameters[i] = plugin->mainParameters[i];
            plugin->mainChanged[i] = false;

            clap_event_param_value_t event = {};
            event.header.size = sizeof(event);
            event.header.time = 0;
            event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            event.header.type = CLAP_EVENT_PARAM_VALUE;
            event.header.flags = 0;
            event.param_id = i;
            event.cookie = NULL;
            event.note_id = -1;
            event.port_index = -1;
            event.channel = -1;
            event.key = -1;
            event.value = plugin->parameters[i];
            out->try_push(out, &event.header);
        }
    }
}

static bool PluginSyncAudioToMain(MyPlugin *plugin)
{
    bool anyChanged = false;
    auto sl = yup::CriticalSection::ScopedLockType (plugin->syncParameters);

    for (uint32_t i = 0; i < P_COUNT; i++)
    {
        if (plugin->changed[i])
        {
            plugin->mainParameters[i] = plugin->parameters[i];
            plugin->changed[i] = false;
            anyChanged = true;
        }
    }

    return anyChanged;
}

// GUI size.
#if JUCE_MAC
#define GUI_API CLAP_WINDOW_API_COCOA
#elif JUCE_WINDOWS
#define GUI_API CLAP_WINDOW_API_WIN32
#elif JUCE_LINUX
#define GUI_API CLAP_WINDOW_API_X11
#endif

#define GUI_WIDTH (600)
#define GUI_HEIGHT (400)

//==============================================================================

static const clap_plugin_descriptor_t pluginDescriptor =
{
	.clap_version = CLAP_VERSION_INIT,
	.id = "kunitoki.YupCLAP",
	.name = "YupCLAP",
	.vendor = "kunitoki",
	.url = "https://github.com/kunitoki/yup",
	.manual_url = "https://github.com/kunitoki/yup",
	.support_url = "https://github.com/kunitoki/yup",
	.version = "1.0.0",
	.description = "The best audio plugin ever.",

	.features = (const char *[])
    {
		CLAP_PLUGIN_FEATURE_INSTRUMENT,
		CLAP_PLUGIN_FEATURE_SYNTHESIZER,
		CLAP_PLUGIN_FEATURE_STEREO,
		NULL,
	},
};

//==============================================================================

static const clap_plugin_params_t extensionParams =
{
    .count = [] (const clap_plugin_t *plugin) -> uint32_t
    {
        return P_COUNT;
    },

    .get_info = [] (const clap_plugin_t *_plugin, uint32_t index, clap_param_info_t *information) -> bool
    {
        if (index == P_VOLUME)
        {
            memset(information, 0, sizeof(clap_param_info_t));
            information->id = index;
            // These flags enable polyphonic modulation.
            information->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID;
            information->min_value = 0.0f;
            information->max_value = 1.0f;
            information->default_value = 0.5f;
            strcpy(information->name, "Volume");
            return true;
        }
        else
        {
            return false;
        }
    },

    .get_value = [] (const clap_plugin_t *_plugin, clap_id id, double *value) -> bool
    {
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;
        uint32_t i = (uint32_t) id;

        if (i >= P_COUNT) return false;

        // get_value is called on the main thread, but should return the value of the parameter according to the audio thread,
        // since the value on the audio thread is the one that host communicates with us via CLAP_EVENT_PARAM_VALUE events.
        // Since we're accessing the opposite thread's arrays, we must acquire the syncParameters mutex.
        // And although we need to check the mainChanged array, we mustn't actually modify the parameters array,
        // since that can only be done on the audio thread. Don't worry -- it'll pick up the changes eventually.
        auto sl = yup::CriticalSection::ScopedLockType (plugin->syncParameters);
        *value = plugin->mainChanged[i] ? plugin->mainParameters[i] : plugin->parameters[i];

        return true;
    },

    .value_to_text = [] (const clap_plugin_t *_plugin, clap_id id, double value, char *display, uint32_t size)
    {
        uint32_t i = (uint32_t) id;
        if (i >= P_COUNT) return false;
        snprintf(display, size, "%f", value);
        return true;
    },

    .text_to_value = [] (const clap_plugin_t *_plugin, clap_id param_id, const char *display, double *value)
    {
        // TODO Implement this.
        return false;
    },

    .flush = [] (const clap_plugin_t *_plugin, const clap_input_events_t *in, const clap_output_events_t *out)
    {
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;
        const uint32_t eventCount = in->size(in);

        // For parameters that have been modified by the main thread, send CLAP_EVENT_PARAM_VALUE events to the host.
        PluginSyncMainToAudio(plugin, out);

        // Process events sent to our plugin from the host.
        for (uint32_t eventIndex = 0; eventIndex < eventCount; eventIndex++)
        {
            PluginProcessEvent(plugin, in->get(in, eventIndex));
        }
    },
};

//==============================================================================

static const clap_plugin_note_ports_t extensionNotePorts =
{
	.count = [] (const clap_plugin_t *plugin, bool isInput) -> uint32_t
    {
		return isInput ? 1 : 0;
	},

	.get = [] (const clap_plugin_t *plugin, uint32_t index, bool isInput, clap_note_port_info_t *info) -> bool
    {
		if (!isInput || index) return false;
		info->id = 0;
		info->supported_dialects = CLAP_NOTE_DIALECT_CLAP; // TODO Also support the MIDI dialect.
		info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
		snprintf(info->name, sizeof(info->name), "%s", "Note Port");
		return true;
	},
};

//==============================================================================

static const clap_plugin_audio_ports_t extensionAudioPorts =
{
	.count = [] (const clap_plugin_t *plugin, bool isInput) -> uint32_t
    {
		return isInput ? 0 : 1;
	},

	.get = [] (const clap_plugin_t *plugin, uint32_t index, bool isInput, clap_audio_port_info_t *info) -> bool
    {
		if (isInput || index) return false;
		info->id = 0;
		info->channel_count = 2;
		info->flags = CLAP_AUDIO_PORT_IS_MAIN;
		info->port_type = CLAP_PORT_STEREO;
		info->in_place_pair = CLAP_INVALID_ID;
		snprintf(info->name, sizeof(info->name), "%s", "Audio Output");
		return true;
	},
};

//==============================================================================

static const clap_plugin_state_t extensionState =
{
    .save = [] (const clap_plugin_t *_plugin, const clap_ostream_t *stream) -> bool
    {
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        // Synchronize any changes from the audio thread (that is, parameter values sent to us by the host)
        // before we save the state of the plugin.
        PluginSyncAudioToMain(plugin);

        return sizeof(float) * P_COUNT == stream->write(stream, plugin->mainParameters, sizeof(float) * P_COUNT);
    },

    .load = [] (const clap_plugin_t *_plugin, const clap_istream_t *stream) -> bool
    {
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        // Since we're modifying a parameter array, we need to acquire the syncParameters mutex.
        auto sl = yup::CriticalSection::ScopedLockType (plugin->syncParameters);

        bool success = sizeof(float) * P_COUNT == stream->read(stream, plugin->mainParameters, sizeof(float) * P_COUNT);
        // Make sure that the audio thread will pick up upon the modified parameters next time pluginClass.process is called.
        for (uint32_t i = 0; i < P_COUNT; i++)
            plugin->mainChanged[i] = true;

        return success;
    },
};

//==============================================================================

static const clap_plugin_gui_t extensionGUI =
{
    .is_api_supported = [] (const clap_plugin_t *_plugin, const char *api, bool isFloating) -> bool
    {
        return 0 == strcmp(api, GUI_API) && !isFloating;
    },

    .get_preferred_api = [] (const clap_plugin_t *_plugin, const char **api, bool *isFloating) -> bool
    {
        *api = GUI_API;
        *isFloating = false;
        return true;
    },

    .create = [] (const clap_plugin_t *_plugin, const char *api, bool isFloating) -> bool
    {
        DBG ("clap_plugin_gui_t::create");

        if (!extensionGUI.is_api_supported(_plugin, api, isFloating)) return false;

        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        plugin->gui = std::make_unique<yup::Component>();
        plugin->gui->setVisible (false);
        plugin->gui->setSize ({ GUI_WIDTH, GUI_HEIGHT });
        return true;
    },

    .destroy = [] (const clap_plugin_t *_plugin)
    {
        DBG ("clap_plugin_gui_t::destroy");

        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        plugin->gui.reset();
    },

    .set_scale = [] (const clap_plugin_t *plugin, double scale) -> bool
    {
        DBG ("clap_plugin_gui_t::set_scale " << scale);

        return false;
    },

    .get_size = [] (const clap_plugin_t *_plugin, uint32_t *width, uint32_t *height) -> bool
    {
        DBG ("clap_plugin_gui_t::get_size");

        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        *width = plugin->gui->getWidth();
        *height = plugin->gui->getHeight();
        return true;
    },

    .can_resize = [] (const clap_plugin_t *plugin) -> bool
    {
        return true;
    },

    .get_resize_hints = [] (const clap_plugin_t *plugin, clap_gui_resize_hints_t *hints) -> bool
    {
        hints->can_resize_horizontally = true;
        hints->can_resize_vertically = true;
        hints->preserve_aspect_ratio = false;
        hints->aspect_ratio_width = 0;
        hints->aspect_ratio_height = 0;

        return true;
    },

    .adjust_size = [] (const clap_plugin_t *_plugin, uint32_t *width, uint32_t *height) -> bool
    {
        return extensionGUI.get_size(_plugin, width, height);
    },

    .set_size = [] (const clap_plugin_t *_plugin, uint32_t width, uint32_t height) -> bool
    {
        DBG ("clap_plugin_gui_t::set_size " << (int32_t)width << "," << (int32_t)height);

        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        plugin->gui->setSize ({ static_cast<float> (width), static_cast<float> (height) });
        return true;
    },

    .set_parent = [] (const clap_plugin_t *_plugin, const clap_window_t *window) -> bool
    {
        DBG ("clap_plugin_gui_t::set_parent");

        assert(0 == strcmp(window->api, GUI_API));

        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        plugin->gui->addToDesktop(yup::ComponentNative::defaultFlags & ~yup::ComponentNative::decoratedWindow, window->cocoa);

        return true;
    },

    .set_transient = [] (const clap_plugin_t *plugin, const clap_window_t *window) -> bool
    {
        DBG ("clap_plugin_gui_t::set_transient");
        return false;
    },

    .suggest_title = [] (const clap_plugin_t *plugin, const char *title)
    {
        DBG ("clap_plugin_gui_t::suggest_title " << title);
    },

    .show = [] (const clap_plugin_t *_plugin) -> bool
    {
        DBG ("clap_plugin_gui_t::show");

        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        plugin->gui->setVisible(true);
        return true;
    },

    .hide = [] (const clap_plugin_t *_plugin) -> bool
    {
        DBG ("clap_plugin_gui_t::hide");

        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        plugin->gui->setVisible(false);
        return true;
    },
};

//==============================================================================

static const clap_plugin_timer_support_t extensionTimerSupport =
{
    .on_timer = [] (const clap_plugin_t *_plugin, clap_id timerID)
    {
        MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        if (plugin->gui && PluginSyncAudioToMain(plugin))
            ;

        yup::MessageManager::getInstance()->runDispatchLoopUntil(10);
    },
};

//==============================================================================

static const clap_plugin_t pluginClass =
{
	.desc = &pluginDescriptor,
	.plugin_data = nullptr,

	.init = [] (const clap_plugin *_plugin) -> bool
    {
        DBG ("clap_plugin_t::init");

		MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

        plugin->hostTimerSupport = (const clap_host_timer_support_t *) plugin->host->get_extension(plugin->host, CLAP_EXT_TIMER_SUPPORT);

        if (plugin->hostTimerSupport && plugin->hostTimerSupport->register_timer) {
            plugin->hostTimerSupport->register_timer(plugin->host, 20 /* every 20 milliseconds */, &plugin->timerID);
        }

        for (uint32_t i = 0; i < P_COUNT; i++)
        {
            clap_param_info_t information = {};
            extensionParams.get_info(_plugin, i, &information);
            plugin->mainParameters[i] = plugin->parameters[i] = information.default_value;
        }

		return true;
	},

	.destroy = [] (const clap_plugin *_plugin)
    {
        DBG ("clap_plugin_t::destroy");

		MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

		plugin->voices.Free();

        if (plugin->hostTimerSupport && plugin->hostTimerSupport->register_timer) {
            plugin->hostTimerSupport->unregister_timer(plugin->host, plugin->timerID);
        }

		free(plugin);
	},

	.activate = [] (const clap_plugin *_plugin, double sampleRate, uint32_t minimumFramesCount, uint32_t maximumFramesCount) -> bool
    {
        DBG ("clap_plugin_t::activate");

		MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;
		plugin->sampleRate = sampleRate;
		return true;
	},

	.deactivate = [] (const clap_plugin *_plugin)
    {
        DBG ("clap_plugin_t::deactivate");
	},

	.start_processing = [] (const clap_plugin *_plugin) -> bool
    {
        DBG ("clap_plugin_t::start_processing");
		return true;
	},

	.stop_processing = [] (const clap_plugin *_plugin)
    {
        DBG ("clap_plugin_t::stop_processing");
	},

	.reset = [] (const clap_plugin *_plugin)
    {
        DBG ("clap_plugin_t::reset");

		MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;
		plugin->voices.Free();
	},

	.process = [] (const clap_plugin *_plugin, const clap_process_t *process) -> clap_process_status
    {
		MyPlugin *plugin = (MyPlugin *) _plugin->plugin_data;

		assert(process->audio_outputs_count == 1);
		assert(process->audio_inputs_count == 0);

        PluginSyncMainToAudio(plugin, process->out_events);

		const uint32_t frameCount = process->frames_count;
		const uint32_t inputEventCount = process->in_events->size(process->in_events);
		uint32_t eventIndex = 0;
		uint32_t nextEventFrame = inputEventCount ? 0 : frameCount;

		for (uint32_t i = 0; i < frameCount; )
        {
			while (eventIndex < inputEventCount && nextEventFrame == i)
            {
				const clap_event_header_t *event = process->in_events->get(process->in_events, eventIndex);

				if (event->time != i)
                {
					nextEventFrame = event->time;
					break;
				}

				PluginProcessEvent(plugin, event);
				eventIndex++;

				if (eventIndex == inputEventCount)
                {
					nextEventFrame = frameCount;
					break;
				}
			}

			PluginRenderAudio(plugin, i, nextEventFrame, process->audio_outputs[0].data32[0], process->audio_outputs[0].data32[1]);
			i = nextEventFrame;
		}

		for (int i = 0; i < plugin->voices.Length(); i++)
        {
			Voice *voice = &plugin->voices[i];

			if (!voice->held)
            {
				clap_event_note_t event = {};
				event.header.size = sizeof(event);
				event.header.time = 0;
				event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
				event.header.type = CLAP_EVENT_NOTE_END;
				event.header.flags = 0;
				event.key = voice->key;
				event.note_id = voice->noteID;
				event.channel = voice->channel;
				event.port_index = 0;
				process->out_events->try_push(process->out_events, &event.header);

				plugin->voices.Delete(i--);
			}
		}

		return CLAP_PROCESS_CONTINUE;
	},

	.get_extension = [] (const clap_plugin *plugin, const char *id) -> const void*
    {
        DBG ("clap_plugin_t::get_extension " << id);

		if (0 == strcmp(id, CLAP_EXT_NOTE_PORTS     )) return &extensionNotePorts;
		if (0 == strcmp(id, CLAP_EXT_AUDIO_PORTS    )) return &extensionAudioPorts;
        if (0 == strcmp(id, CLAP_EXT_PARAMS         )) return &extensionParams;
        if (0 == strcmp(id, CLAP_EXT_STATE          )) return &extensionState;
        if (0 == strcmp(id, CLAP_EXT_GUI            )) return &extensionGUI;
        if (0 == strcmp(id, CLAP_EXT_TIMER_SUPPORT  )) return &extensionTimerSupport;
        return nullptr;
	},

	.on_main_thread = [] (const clap_plugin *_plugin)
    {
        DBG ("clap_plugin_t::on_main_thread");
	},
};

//==============================================================================

static const clap_plugin_factory_t pluginFactory =
{
	.get_plugin_count = [](const clap_plugin_factory *factory) -> uint32_t
    {
        DBG ("clap_plugin_factory_t::get_plugin_count");

		return 1;
	},

	.get_plugin_descriptor = [](const clap_plugin_factory *factory, uint32_t index) -> const clap_plugin_descriptor_t*
    {
        DBG ("clap_plugin_factory_t::get_plugin_descriptor " << (int32_t)index);

		return index == 0 ? &pluginDescriptor : nullptr;
	},

	.create_plugin = [] (const clap_plugin_factory *factory, const clap_host_t *host, const char *pluginID) -> const clap_plugin_t*
    {
        DBG ("clap_plugin_factory_t::create_plugin");

		if (!clap_version_is_compatible(host->clap_version) || strcmp(pluginID, pluginDescriptor.id))
        {
			return nullptr;
		}

		// Allocate the plugin structure, and fill in the plugin information from the pluginClass variable.
		MyPlugin *plugin = (MyPlugin *) calloc(1, sizeof(MyPlugin));
		plugin->host = host;
		plugin->plugin = pluginClass;
		plugin->plugin.plugin_data = plugin;
		return &plugin->plugin;
	},
};

//==============================================================================

extern "C" const clap_plugin_entry_t clap_entry =
{
	.clap_version = CLAP_VERSION_INIT,

	.init = [](const char *path) -> bool
    {
        DBG ("clap_plugin_entry_t::init");

        yup::initialiseJuce_GUI();
        yup::staticInitialisation();

		return true;
	},

	.deinit = []
    {
        DBG ("clap_plugin_entry_t::deinit");

        yup::staticFinalisation();
        yup::shutdownJuce_GUI();
    },

	.get_factory = [](const char *factoryID) -> const void*
    {
        DBG ("clap_plugin_entry_t::get_factory " << factoryID);
		return strcmp(factoryID, CLAP_PLUGIN_FACTORY_ID) ? nullptr : &pluginFactory;
	},
};
