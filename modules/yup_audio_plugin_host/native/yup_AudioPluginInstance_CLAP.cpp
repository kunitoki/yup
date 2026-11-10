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

//==============================================================================
struct CLAPModule
{
    CLAPModuleHandle handle = nullptr;
    const clap_plugin_entry_t* entry = nullptr;

#if YUP_MAC
    template <typename CFType>
    struct CFObjectDeleter
    {
        void operator() (CFType object) const noexcept
        {
            if (object != nullptr)
                CFRelease (object);
        }
    };

    template <typename CFType>
    using CFUniquePtr = std::unique_ptr<std::remove_pointer_t<CFType>, CFObjectDeleter<CFType>>;

    static CFUniquePtr<CFBundleRef> createBundle (const File& file)
    {
        const auto path = file.getFullPathName();
        CFUniquePtr<CFURLRef> url (CFURLCreateFromFileSystemRepresentation (kCFAllocatorDefault,
                                                                            reinterpret_cast<const UInt8*> (path.toRawUTF8()),
                                                                            static_cast<CFIndex> (path.getNumBytesAsUTF8()),
                                                                            true));

        if (url == nullptr)
            return {};

        return CFUniquePtr<CFBundleRef> (CFBundleCreate (kCFAllocatorDefault, url.get()));
    }

    static File getBundleExecutableFile (CFBundleRef bundleToUse, const File& bundleFile)
    {
        const auto macOSFolder = bundleFile.getChildFile ("Contents/MacOS");

        if (bundleToUse != nullptr)
        {
            auto* executableValue = CFBundleGetValueForInfoDictionaryKey (bundleToUse, kCFBundleExecutableKey);

            if (executableValue != nullptr && CFGetTypeID (executableValue) == CFStringGetTypeID())
                return macOSFolder.getChildFile (String::fromCFString (static_cast<CFStringRef> (executableValue)));
        }

        return macOSFolder.getChildFile (bundleFile.getFileNameWithoutExtension());
    }
#endif

    static std::unique_ptr<CLAPModule> load (const File& file)
    {
        auto m = std::make_unique<CLAPModule>();
        auto libraryFile = file;

#if YUP_MAC
        if (file.isDirectory())
        {
            auto bundle = createBundle (file);

            if (bundle == nullptr)
                return nullptr;

            libraryFile = getBundleExecutableFile (bundle.get(), file);
        }
#endif

        m->handle = clapLoadModule (libraryFile.getFullPathName().toRawUTF8());

        if (m->handle == nullptr)
            return nullptr;

        // The destructor unloads the library, and only deinitialises an entry whose init succeeded
        const auto* entry = reinterpret_cast<const clap_plugin_entry_t*> (clapGetAddress (m->handle, "clap_entry"));

        if (entry == nullptr || entry->init == nullptr || ! entry->init (file.getFullPathName().toRawUTF8()))
            return nullptr;

        m->entry = entry;
        return m;
    }

    ~CLAPModule()
    {
        if (entry != nullptr && entry->deinit != nullptr)
            entry->deinit();

        if (handle != nullptr)
            clapUnloadModule (handle);
    }
};

//==============================================================================
template <typename Extension>
const Extension* getCLAPExtension (const clap_plugin_t* plugin, const char* extensionId)
{
    if (plugin == nullptr || plugin->get_extension == nullptr)
        return nullptr;

    return static_cast<const Extension*> (plugin->get_extension (plugin, extensionId));
}

// CLAP thread-check reports audio threads by this flag, set while the host drives processing
thread_local bool isCLAPAudioThread = false;

struct ScopedCLAPAudioThread
{
    ScopedCLAPAudioThread() noexcept
        : previous (isCLAPAudioThread)
    {
        isCLAPAudioThread = true;
    }

    ~ScopedCLAPAudioThread() noexcept
    {
        isCLAPAudioThread = previous;
    }

    const bool previous;
};

//==============================================================================
// Runs clap_plugin_thread_pool tasks in parallel, the requesting audio thread takes part too.
class CLAPThreadPool
{
public:
    ~CLAPThreadPool()
    {
        stop();
    }

    void start (const clap_plugin_t* pluginToUse, int numWorkers)
    {
        stop();

        plugin = pluginToUse;
        extension = getCLAPExtension<clap_plugin_thread_pool_t> (plugin, CLAP_EXT_THREAD_POOL);

        if (extension == nullptr || extension->exec == nullptr)
            return;

        for (int i = 0; i < numWorkers; ++i)
        {
            auto& worker = workers.emplace_back (std::make_unique<Worker> (*this));
            worker->startThread (Thread::Priority::highest);
        }
    }

    void stop()
    {
        for (auto& worker : workers)
        {
            worker->signalThreadShouldExit();
            worker->wake.signal();
        }

        for (auto& worker : workers)
            worker->stopThread (-1);

        workers.clear();
        extension = nullptr;
    }

    bool execute (uint32_t numTasks)
    {
        if (extension == nullptr)
            return false;

        totalTasks.store (numTasks, std::memory_order_relaxed);
        nextTask.store (0, std::memory_order_relaxed);
        pendingWorkers.store (static_cast<int> (workers.size()), std::memory_order_release);

        for (auto& worker : workers)
            worker->wake.signal();

        runTasks();

        // Returning only once every worker checked in guarantees none touches the next request
        while (pendingWorkers.load (std::memory_order_acquire) > 0)
            std::this_thread::yield();

        return true;
    }

private:
    struct Worker final : public Thread
    {
        explicit Worker (CLAPThreadPool& ownerToUse)
            : Thread ("CLAP Thread Pool")
            , owner (ownerToUse)
        {
        }

        void run() override
        {
            isCLAPAudioThread = true;

            while (! threadShouldExit())
            {
                wake.wait (-1);

                if (threadShouldExit())
                    break;

                owner.runTasks();
                owner.pendingWorkers.fetch_sub (1, std::memory_order_acq_rel);
            }
        }

        CLAPThreadPool& owner;
        WaitableEvent wake;
    };

    void runTasks()
    {
        for (;;)
        {
            const auto task = nextTask.fetch_add (1, std::memory_order_acq_rel);
            if (task >= totalTasks.load (std::memory_order_relaxed))
                break;

            extension->exec (plugin, task);
        }
    }

    const clap_plugin_t* plugin = nullptr;
    const clap_plugin_thread_pool_t* extension = nullptr;
    std::vector<std::unique_ptr<Worker>> workers;
    std::atomic<uint32_t> nextTask { 0 };
    std::atomic<uint32_t> totalTasks { 0 };
    std::atomic<int> pendingWorkers { 0 };
};

//==============================================================================
// The clap_host_t supplied to each plugin instance. Requests that may arrive on any thread
// only raise flags, the owning instance serves them on the message thread.
struct YUPCLAPHost
{
    explicit YUPCLAPHost (const AudioPluginHostContext& ctx)
        : hostName (ctx.hostName)
        , hostVendor (ctx.hostVendor)
        , hostVersion (ctx.hostVersion)
    {
        host.clap_version = CLAP_VERSION;
        host.host_data = this;
        host.name = hostName.toRawUTF8();
        host.vendor = hostVendor.toRawUTF8();
        host.url = "";
        host.version = hostVersion.toRawUTF8();

        host.get_extension = [] (const clap_host_t* h, const char* extensionId) -> const void*
        {
            return from (h).getExtension (extensionId);
        };
        host.request_restart = [] (const clap_host_t* h)
        {
            from (h).restartRequested.store (true);
        };
        host.request_process = [] (const clap_host_t*) {};
        host.request_callback = [] (const clap_host_t* h)
        {
            from (h).callbackRequested.store (true);
        };

        threadCheck.is_main_thread = [] (const clap_host_t*)
        {
            return MessageManager::existsAndIsCurrentThread();
        };
        threadCheck.is_audio_thread = [] (const clap_host_t*)
        {
            return isCLAPAudioThread;
        };

        threadPoolExtension.request_exec = [] (const clap_host_t* h, uint32_t numTasks)
        {
            return isCLAPAudioThread && from (h).threadPool.execute (numTasks);
        };

        timerSupport.register_timer = [] (const clap_host_t* h, uint32_t periodMs, clap_id* timerId)
        {
            return from (h).registerTimer (periodMs, timerId);
        };
        timerSupport.unregister_timer = [] (const clap_host_t* h, clap_id timerId)
        {
            return from (h).timers.erase (timerId) > 0;
        };

#if YUP_LINUX
        posixFdSupport.register_fd = [] (const clap_host_t* h, int fd, clap_posix_fd_flags_t flags)
        {
            return from (h).registerFd (fd, flags);
        };
        posixFdSupport.modify_fd = [] (const clap_host_t* h, int fd, clap_posix_fd_flags_t flags)
        {
            auto& self = from (h);
            return self.unregisterFd (fd) && self.registerFd (fd, flags);
        };
        posixFdSupport.unregister_fd = [] (const clap_host_t* h, int fd)
        {
            return from (h).unregisterFd (fd);
        };
#endif

        params.rescan = [] (const clap_host_t* h, clap_param_rescan_flags flags)
        {
            from (h).paramsRescanFlags.fetch_or (flags);
        };
        params.clear = [] (const clap_host_t*, clap_id, clap_param_clear_flags) {};
        params.request_flush = [] (const clap_host_t* h)
        {
            from (h).flushRequested.store (true);
        };

        state.mark_dirty = [] (const clap_host_t* h)
        {
            from (h).stateDirty.store (true);
        };

        log.log = [] (const clap_host_t*, clap_log_severity severity, const char* message)
        {
            ignoreUnused (severity, message);
            YUP_MODULE_DBG (PLUGIN_HOST_CLAP, "plugin log [" << static_cast<int> (severity) << "]: " << (message != nullptr ? message : ""));
        };

        tail.changed = [] (const clap_host_t* h)
        {
            from (h).tailChanged.store (true);
        };

        latency.changed = [] (const clap_host_t* h)
        {
            from (h).latencyChanged.store (true);
        };

        audioPorts.is_rescan_flag_supported = [] (const clap_host_t*, uint32_t flag)
        {
            // Changing the port list would change the bus structure, which hosted instances keep fixed
            return (flag & CLAP_AUDIO_PORTS_RESCAN_LIST) == 0;
        };
        audioPorts.rescan = [] (const clap_host_t* h, uint32_t flags)
        {
            from (h).audioPortsRescanFlags.fetch_or (flags);
        };

        notePorts.supported_dialects = [] (const clap_host_t*) -> uint32_t
        {
            return CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
        };
        notePorts.rescan = [] (const clap_host_t*, uint32_t) {};

        trackInfoExtension.get = [] (const clap_host_t* h, clap_track_info_t* info)
        {
            return info != nullptr && from (h).fillTrackInfo (*info);
        };

        gui.resize_hints_changed = [] (const clap_host_t*) {};
        gui.request_resize = [] (const clap_host_t* h, uint32_t width, uint32_t height) -> bool
        {
            auto& self = from (h);
            return MessageManager::existsAndIsCurrentThread() && self.guiResizeRequested != nullptr && self.guiResizeRequested (width, height);
        };
        gui.request_show = [] (const clap_host_t* h) -> bool
        {
            auto& self = from (h);
            return MessageManager::existsAndIsCurrentThread() && self.guiShowRequested != nullptr && self.guiShowRequested();
        };
        gui.request_hide = [] (const clap_host_t* h) -> bool
        {
            auto& self = from (h);
            return MessageManager::existsAndIsCurrentThread() && self.guiHideRequested != nullptr && self.guiHideRequested();
        };
        gui.closed = [] (const clap_host_t* h, bool wasDestroyed)
        {
            auto& self = from (h);
            if (MessageManager::existsAndIsCurrentThread() && self.guiClosed != nullptr)
                self.guiClosed (wasDestroyed);
        };
    }

    ~YUPCLAPHost()
    {
        threadPool.stop();
        timers.clear();

#if YUP_LINUX
        for (const auto& [fd, registration] : fds)
            PluginFdDispatcher::remove (fd, registration.second);
#endif
    }

    clap_host_t host {};
    const clap_plugin_t* plugin = nullptr;

    std::atomic<bool> callbackRequested { false };
    std::atomic<bool> restartRequested { false };
    std::atomic<bool> flushRequested { false };
    std::atomic<bool> latencyChanged { false };
    std::atomic<bool> tailChanged { false };
    std::atomic<bool> stateDirty { false };
    std::atomic<uint32_t> paramsRescanFlags { 0 };
    std::atomic<uint32_t> audioPortsRescanFlags { 0 };

    AudioProcessorBase::HostInfo trackInfo;
    CLAPThreadPool threadPool;

    std::function<bool (uint32_t, uint32_t)> guiResizeRequested;
    std::function<bool()> guiShowRequested;
    std::function<bool()> guiHideRequested;
    std::function<void (bool)> guiClosed;

private:
    struct PluginTimer final : public Timer
    {
        PluginTimer (YUPCLAPHost& ownerToUse, clap_id idToUse)
            : owner (ownerToUse)
            , id (idToUse)
        {
        }

        void timerCallback() override
        {
            if (auto* extension = getCLAPExtension<clap_plugin_timer_support_t> (owner.plugin, CLAP_EXT_TIMER_SUPPORT); extension != nullptr && extension->on_timer != nullptr)
                extension->on_timer (owner.plugin, id);
        }

        YUPCLAPHost& owner;
        const clap_id id;
    };

    static YUPCLAPHost& from (const clap_host_t* h) noexcept
    {
        return *static_cast<YUPCLAPHost*> (h->host_data);
    }

    const void* getExtension (const char* extensionId) const
    {
        const std::pair<const char*, const void*> extensions[] = {
            { CLAP_EXT_THREAD_CHECK, &threadCheck },
            { CLAP_EXT_THREAD_POOL, &threadPoolExtension },
            { CLAP_EXT_TIMER_SUPPORT, &timerSupport },
#if YUP_LINUX
            { CLAP_EXT_POSIX_FD_SUPPORT, &posixFdSupport },
#endif
            { CLAP_EXT_PARAMS, &params },
            { CLAP_EXT_STATE, &state },
            { CLAP_EXT_LOG, &log },
            { CLAP_EXT_TAIL, &tail },
            { CLAP_EXT_LATENCY, &latency },
            { CLAP_EXT_AUDIO_PORTS, &audioPorts },
            { CLAP_EXT_NOTE_PORTS, &notePorts },
            { CLAP_EXT_TRACK_INFO, &trackInfoExtension },
            { CLAP_EXT_GUI, &gui },
        };

        for (const auto& [id, extension] : extensions)
        {
            if (std::strcmp (extensionId, id) == 0)
                return extension;
        }

        return nullptr;
    }

    bool registerTimer (uint32_t periodMs, clap_id* timerId)
    {
        if (timerId == nullptr || ! MessageManager::existsAndIsCurrentThread())
            return false;

        const auto id = nextTimerId++;
        auto& timer = timers[id];
        timer = std::make_unique<PluginTimer> (*this, id);
        timer->startTimer (static_cast<int> (jlimit<uint32_t> (1, static_cast<uint32_t> (std::numeric_limits<int>::max()), periodMs)));

        *timerId = id;
        return true;
    }

    bool fillTrackInfo (clap_track_info_t& info) const
    {
        info = {};

        if (trackInfo.trackName.isNotEmpty())
        {
            info.flags |= CLAP_TRACK_INFO_HAS_TRACK_NAME;
            trackInfo.trackName.copyToUTF8 (info.name, sizeof (info.name));
        }

        if (trackInfo.trackColor.has_value())
        {
            const auto argb = *trackInfo.trackColor;
            info.flags |= CLAP_TRACK_INFO_HAS_TRACK_COLOR;
            info.color = { static_cast<uint8_t> (argb >> 24), static_cast<uint8_t> (argb >> 16), static_cast<uint8_t> (argb >> 8), static_cast<uint8_t> (argb) };
        }

        return info.flags != 0;
    }

#if YUP_LINUX
    bool registerFd (int fd, clap_posix_fd_flags_t flags)
    {
        if (fds.count (fd) > 0)
            return false;

        short eventMask = 0;
        eventMask |= (flags & CLAP_POSIX_FD_READ) != 0 ? POLLIN : 0;
        eventMask |= (flags & CLAP_POSIX_FD_WRITE) != 0 ? POLLOUT : 0;
        eventMask |= (flags & CLAP_POSIX_FD_ERROR) != 0 ? POLLERR : 0;

        const auto token = PluginFdDispatcher::add (fd, [this, flags] (int readyFd)
        {
            if (auto* extension = getCLAPExtension<clap_plugin_posix_fd_support_t> (plugin, CLAP_EXT_POSIX_FD_SUPPORT); extension != nullptr && extension->on_fd != nullptr)
                extension->on_fd (plugin, readyFd, flags);
        },
                                                    eventMask);

        fds[fd] = { flags, token };

        return true;
    }

    bool unregisterFd (int fd)
    {
        const auto it = fds.find (fd);
        if (it == fds.end())
            return false;

        PluginFdDispatcher::remove (fd, it->second.second);
        fds.erase (it);
        return true;
    }

    std::map<int, std::pair<clap_posix_fd_flags_t, int>> fds;
    clap_host_posix_fd_support_t posixFdSupport {};
#endif

    clap_host_thread_check_t threadCheck {};
    clap_host_thread_pool_t threadPoolExtension {};
    clap_host_timer_support_t timerSupport {};
    clap_host_params_t params {};
    clap_host_state_t state {};
    clap_host_log_t log {};
    clap_host_tail_t tail {};
    clap_host_latency_t latency {};
    clap_host_audio_ports_t audioPorts {};
    clap_host_note_ports_t notePorts {};
    clap_host_track_info_t trackInfoExtension {};
    clap_host_gui_t gui {};

    std::map<clap_id, std::unique_ptr<PluginTimer>> timers;
    clap_id nextTimerId = 0;

    String hostName;
    String hostVendor;
    String hostVersion;
};

//==============================================================================
#if YUP_MAC
void* getCLAPParentViewFromNativeHandle (void* nativeHandle)
{
    if (nativeHandle == nullptr)
        return nullptr;

    id nativeObject = (__bridge id) nativeHandle;
    if ([nativeObject isKindOfClass:[NSWindow class]])
        return (__bridge void*) [(NSWindow*) nativeObject contentView];

    if ([nativeObject isKindOfClass:[NSView class]])
        return nativeHandle;

    return nullptr;
}
#endif

const char* getCLAPWindowApi()
{
#if YUP_MAC
    return CLAP_WINDOW_API_COCOA;
#elif YUP_WINDOWS
    return CLAP_WINDOW_API_WIN32;
#elif YUP_LINUX
    return CLAP_WINDOW_API_X11;
#else
    return nullptr;
#endif
}

bool initialiseCLAPWindow (clap_window_t& window, void* nativeHandle)
{
    if (nativeHandle == nullptr)
        return false;

    window.api = getCLAPWindowApi();
    if (window.api == nullptr)
        return false;

#if YUP_MAC
    window.cocoa = getCLAPParentViewFromNativeHandle (nativeHandle);
    return window.cocoa != nullptr;
#elif YUP_WINDOWS
    window.win32 = nativeHandle;
    return true;
#elif YUP_LINUX
    window.x11 = static_cast<clap_xwnd> (reinterpret_cast<std::uintptr_t> (nativeHandle));
    return window.x11 != 0;
#else
    ignoreUnused (window);
    return false;
#endif
}

bool canCreateCLAPEditor (const clap_plugin_t* plugin)
{
    if (plugin == nullptr)
        return false;

    const auto* gui = reinterpret_cast<const clap_plugin_gui_t*> (
        plugin->get_extension (plugin, CLAP_EXT_GUI));

    const auto* windowApi = getCLAPWindowApi();
    return gui != nullptr
        && windowApi != nullptr
        && gui->is_api_supported != nullptr
        && gui->is_api_supported (plugin, windowApi, false);
}

class CLAPEditor final : public AudioProcessorEditor
{
public:
    static std::unique_ptr<CLAPEditor> create (const clap_plugin_t* plugin, YUPCLAPHost& host)
    {
        if (! canCreateCLAPEditor (plugin))
            return nullptr;

        const auto* gui = reinterpret_cast<const clap_plugin_gui_t*> (
            plugin->get_extension (plugin, CLAP_EXT_GUI));

        if (gui == nullptr || gui->create == nullptr || ! gui->create (plugin, getCLAPWindowApi(), false))
            return nullptr;

        return std::unique_ptr<CLAPEditor> (new CLAPEditor (plugin, gui, host));
    }

    ~CLAPEditor() override
    {
        detachPlugView();

        host.guiResizeRequested = nullptr;
        host.guiShowRequested = nullptr;
        host.guiHideRequested = nullptr;
        host.guiClosed = nullptr;

        if (gui != nullptr && gui->destroy != nullptr)
            gui->destroy (clapPlugin);
    }

    bool isResizable() const override
    {
        return gui != nullptr && gui->can_resize != nullptr && gui->can_resize (clapPlugin);
    }

    bool shouldPreserveAspectRatio() const override
    {
        return getAspectRatioHint().has_value();
    }

    Size<int> getPreferredAspectRatioSize() const override
    {
        return getAspectRatioHint().value_or (preferredSize);
    }

    Size<int> getPreferredSize() const override { return preferredSize; }

    void paint (Graphics& g) override
    {
        g.setFillColor (Color (0xff101417));
        g.fillAll();
    }

    void resized() override
    {
        resizePlugViewToBounds();
    }

    void attachedToNative() override
    {
        attachPlugView();
    }

    void detachedFromNative() override
    {
        detachPlugView();
    }

private:
    CLAPEditor (const clap_plugin_t* plugin, const clap_plugin_gui_t* guiExtension, YUPCLAPHost& hostToUse)
        : clapPlugin (plugin)
        , gui (guiExtension)
        , host (hostToUse)
    {
        uint32_t width = 0;
        uint32_t height = 0;

        if (gui != nullptr
            && gui->get_size != nullptr
            && gui->get_size (clapPlugin, &width, &height)
            && width > 0
            && height > 0)
        {
            preferredSize = {
                jmax (320, static_cast<int> (width)),
                jmax (240, static_cast<int> (height))
            };
        }

        setSize (preferredSize.to<float>());

        host.guiResizeRequested = [this] (uint32_t widthToUse, uint32_t heightToUse)
        {
            return handleResizeRequest (widthToUse, heightToUse);
        };
        host.guiShowRequested = [this]
        {
            return handleShowRequest();
        };
        host.guiHideRequested = [this]
        {
            return handleHideRequest();
        };
        host.guiClosed = [this] (bool)
        {
            shown = false;
            attached = false;
        };
    }

    bool handleResizeRequest (uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
            return false;

        preferredSize = {
            jmax (1, static_cast<int> (width)),
            jmax (1, static_cast<int> (height))
        };

        setHostedEditorSize (*this, preferredSize);

        return true;
    }

    bool handleShowRequest()
    {
        if (auto* topLevel = getTopLevelComponent())
        {
            topLevel->setVisible (true);
            return true;
        }

        return false;
    }

    bool handleHideRequest()
    {
        if (auto* topLevel = getTopLevelComponent())
        {
            topLevel->setVisible (false);
            return true;
        }

        return false;
    }

    void attachPlugView()
    {
        if (gui == nullptr || clapPlugin == nullptr || attached)
            return;

        auto* nativeComponent = getNativeComponent();
        if (nativeComponent == nullptr)
            return;

        clap_window_t parentWindow {};
        if (! initialiseCLAPWindow (parentWindow, nativeComponent->getNativeHandle()))
            return;

        if (gui->set_parent == nullptr || ! gui->set_parent (clapPlugin, &parentWindow))
            return;

        attached = true;
        resizePlugViewToBounds();

        if (! shown && gui->show != nullptr)
            shown = gui->show (clapPlugin);
    }

    void detachPlugView()
    {
        if (gui == nullptr || clapPlugin == nullptr)
            return;

        if (shown && gui->hide != nullptr)
            gui->hide (clapPlugin);

        shown = false;
        attached = false;
    }

    std::optional<Size<int>> getAspectRatioHint() const
    {
        clap_gui_resize_hints_t hints {};

        if (gui == nullptr || gui->get_resize_hints == nullptr || ! gui->get_resize_hints (clapPlugin, &hints))
            return std::nullopt;

        if (! hints.preserve_aspect_ratio || hints.aspect_ratio_width == 0 || hints.aspect_ratio_height == 0)
            return std::nullopt;

        return Size<int> { static_cast<int> (hints.aspect_ratio_width), static_cast<int> (hints.aspect_ratio_height) };
    }

    void resizePlugViewToBounds()
    {
        if (gui == nullptr || clapPlugin == nullptr || ! attached || gui->set_size == nullptr)
            return;

        const auto bounds = getBoundsRelativeToTopLevelComponent();
        uint32_t width = static_cast<uint32_t> (jmax (1.0f, bounds.getWidth()));
        uint32_t height = static_cast<uint32_t> (jmax (1.0f, bounds.getHeight()));

        if (isResizable() && gui->adjust_size != nullptr && gui->adjust_size (clapPlugin, &width, &height))
            followHostedEditorSize (*this, { static_cast<int> (width), static_cast<int> (height) });

        if (width > 0 && height > 0)
            gui->set_size (clapPlugin, width, height);
    }

    const clap_plugin_t* clapPlugin = nullptr;
    const clap_plugin_gui_t* gui = nullptr;
    YUPCLAPHost& host;
    Size<int> preferredSize { 640, 480 };
    bool attached = false;
    bool shown = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CLAPEditor)
};

//==============================================================================
// Fixed capacity, sorted input event list: storage is reserved once so filling it never allocates.
class CLAPInputEventList
{
public:
    CLAPInputEventList()
    {
        list.ctx = this;
        list.size = [] (const clap_input_events_t* events) -> uint32_t
        {
            return static_cast<uint32_t> (static_cast<const CLAPInputEventList*> (events->ctx)->events.size());
        };
        list.get = [] (const clap_input_events_t* events, uint32_t index) -> const clap_event_header_t*
        {
            const auto& storage = static_cast<const CLAPInputEventList*> (events->ctx)->events;
            return index < storage.size() ? &storage[static_cast<std::size_t> (index)].header : nullptr;
        };
    }

    void reserve (int maxEvents) { events.reserve (static_cast<std::size_t> (jmax (0, maxEvents))); }

    void clear() noexcept { events.clear(); }

    bool isEmpty() const noexcept { return events.empty(); }

    void addParameterValue (int sampleOffset, clap_id id, void* cookie, double value) noexcept
    {
        Event event {};
        event.param = {};
        initialiseHeader (event.param.header, sizeof (event.param), CLAP_EVENT_PARAM_VALUE, sampleOffset);
        event.param.param_id = id;
        event.param.cookie = cookie;
        event.param.note_id = -1;
        event.param.port_index = -1;
        event.param.channel = -1;
        event.param.key = -1;
        event.param.value = value;
        push (event);
    }

    void addNoteExpression (const NoteExpression& expression) noexcept
    {
        static_assert (CLAP_NOTE_EXPRESSION_PRESSURE == static_cast<int> (NoteExpression::Type::pressure));

        Event event {};
        event.expression = {};
        initialiseHeader (event.expression.header, sizeof (event.expression), CLAP_EVENT_NOTE_EXPRESSION, expression.sampleOffset);
        event.expression.expression_id = static_cast<clap_note_expression> (expression.type);
        event.expression.note_id = -1;
        event.expression.port_index = 0;
        event.expression.channel = expression.channel >= 1 ? static_cast<int16_t> (expression.channel - 1) : -1;
        event.expression.key = expression.key;
        event.expression.value = static_cast<double> (expression.value);
        push (event);
    }

    void addMidiMessage (const MidiMessageMetadata& metadata) noexcept
    {
        const auto message = metadata.getMessage();
        Event event {};

        if (message.isNoteOn() || message.isNoteOff())
        {
            event.note = {};
            initialiseHeader (event.note.header, sizeof (event.note), message.isNoteOn() ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF, metadata.samplePosition);
            event.note.note_id = -1;
            event.note.port_index = 0;
            event.note.channel = static_cast<int16_t> (message.getChannel() - 1);
            event.note.key = static_cast<int16_t> (message.getNoteNumber());
            event.note.velocity = static_cast<double> (message.getFloatVelocity());
        }
        else if (message.isSysEx())
        {
            event.sysex = {};
            initialiseHeader (event.sysex.header, sizeof (event.sysex), CLAP_EVENT_MIDI_SYSEX, metadata.samplePosition);
            event.sysex.port_index = 0;
            event.sysex.buffer = message.getSysExData();
            event.sysex.size = static_cast<uint32_t> (message.getSysExDataSize());
        }
        else if (metadata.numBytes > 0 && metadata.numBytes <= 3)
        {
            event.midi = {};
            initialiseHeader (event.midi.header, sizeof (event.midi), CLAP_EVENT_MIDI, metadata.samplePosition);
            event.midi.port_index = 0;
            std::memcpy (event.midi.data, metadata.data, static_cast<std::size_t> (metadata.numBytes));
        }
        else
        {
            return;
        }

        push (event);
    }

    clap_input_events_t list {};

private:
    union Event
    {
        clap_event_header_t header;
        clap_event_param_value_t param;
        clap_event_note_t note;
        clap_event_note_expression_t expression;
        clap_event_midi_t midi;
        clap_event_midi_sysex_t sysex;
    };

    static void initialiseHeader (clap_event_header_t& header, std::size_t size, uint16_t type, int sampleOffset) noexcept
    {
        header.size = static_cast<uint32_t> (size);
        header.time = static_cast<uint32_t> (jmax (0, sampleOffset));
        header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        header.type = type;
        header.flags = 0;
    }

    void push (const Event& event) noexcept
    {
        if (events.size() >= events.capacity())
        {
            jassertfalse; // More events than reserved for a single block, the excess is dropped
            return;
        }

        events.push_back (event);
    }

    std::vector<Event> events;
};

bool hasCLAPNoteInput (const clap_plugin_t* plugin)
{
    auto* notePorts = getCLAPExtension<clap_plugin_note_ports_t> (plugin, CLAP_EXT_NOTE_PORTS);
    if (notePorts == nullptr || notePorts->count (plugin, true) == 0)
        return false;

    clap_note_port_info_t info {};
    return notePorts->get (plugin, 0, true, &info) && (info.supported_dialects & CLAP_NOTE_DIALECT_CLAP) != 0;
}

bool allAudioPortsSupport64Bits (const clap_plugin_t* plugin)
{
    auto* audioPorts = getCLAPExtension<clap_plugin_audio_ports_t> (plugin, CLAP_EXT_AUDIO_PORTS);
    if (audioPorts == nullptr)
        return false;

    bool anyPort = false;

    for (const bool isInput : { true, false })
    {
        for (uint32_t i = 0; i < audioPorts->count (plugin, isInput); ++i)
        {
            clap_audio_port_info_t info {};
            if (! audioPorts->get (plugin, i, isInput, &info) || (info.flags & CLAP_AUDIO_PORT_SUPPORTS_64BITS) == 0)
                return false;

            anyPort = true;
        }
    }

    return anyPort;
}

std::optional<clap_event_transport_t> createCLAPTransport (AudioPlayHead* playHead, double sampleRate)
{
    if (playHead == nullptr)
        return std::nullopt;

    const auto optPosition = playHead->getPosition();
    if (! optPosition.has_value())
        return std::nullopt;

    const auto& position = optPosition.value();

    clap_event_transport_t transport {};
    transport.header.size = sizeof (transport);
    transport.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    transport.header.type = CLAP_EVENT_TRANSPORT;

    if (auto timeInSeconds = position.getTimeInSeconds())
    {
        transport.flags |= CLAP_TRANSPORT_HAS_SECONDS_TIMELINE;
        transport.song_pos_seconds = static_cast<decltype (transport.song_pos_seconds)> (*timeInSeconds * CLAP_SECTIME_FACTOR);
    }
    else if (auto timeInSamples = position.getTimeInSamples(); timeInSamples && sampleRate > 0.0)
    {
        transport.flags |= CLAP_TRANSPORT_HAS_SECONDS_TIMELINE;
        transport.song_pos_seconds = static_cast<decltype (transport.song_pos_seconds)> ((*timeInSamples / sampleRate) * CLAP_SECTIME_FACTOR);
    }

    if (auto ppq = position.getPpqPosition())
    {
        transport.flags |= CLAP_TRANSPORT_HAS_BEATS_TIMELINE;
        transport.song_pos_beats = static_cast<decltype (transport.song_pos_beats)> (*ppq * CLAP_BEATTIME_FACTOR);
    }

    if (auto tempo = position.getBpm())
    {
        transport.flags |= CLAP_TRANSPORT_HAS_TEMPO;
        transport.tempo = *tempo;
    }

    if (auto timeSignature = position.getTimeSignature())
    {
        transport.flags |= CLAP_TRANSPORT_HAS_TIME_SIGNATURE;
        transport.tsig_num = static_cast<uint16_t> (timeSignature->numerator);
        transport.tsig_denom = static_cast<uint16_t> (timeSignature->denominator);
    }

    if (auto loopPoints = position.getLoopPoints())
    {
        transport.loop_start_beats = static_cast<decltype (transport.loop_start_beats)> (loopPoints->ppqStart * CLAP_BEATTIME_FACTOR);
        transport.loop_end_beats = static_cast<decltype (transport.loop_end_beats)> (loopPoints->ppqEnd * CLAP_BEATTIME_FACTOR);
    }

    if (auto barStart = position.getPpqPositionOfLastBarStart())
        transport.bar_start = static_cast<decltype (transport.bar_start)> (*barStart * CLAP_BEATTIME_FACTOR);

    if (auto barCount = position.getBarCount())
        transport.bar_number = static_cast<int32_t> (*barCount);

    if (position.getIsPlaying())
        transport.flags |= CLAP_TRANSPORT_IS_PLAYING;

    if (position.getIsRecording())
        transport.flags |= CLAP_TRANSPORT_IS_RECORDING;

    if (position.getIsLooping())
        transport.flags |= CLAP_TRANSPORT_IS_LOOP_ACTIVE;

    return transport;
}

} // namespace

//==============================================================================

class CLAPInstance
    : public AudioPluginInstance
    , private Timer
{
public:
    CLAPInstance (const AudioPluginDescription& desc,
                  const AudioPluginHostContext& context,
                  std::unique_ptr<CLAPModule> module,
                  std::unique_ptr<YUPCLAPHost> clapHost,
                  const clap_plugin_t* plugin)
        : AudioPluginInstance (desc, buildBusLayout (plugin))
        , hostContext (context)
        , clapModule (std::move (module))
        , yupHost (std::move (clapHost))
        , clapPlugin (plugin)
        , paramsExtension (getCLAPExtension<clap_plugin_params_t> (plugin, CLAP_EXT_PARAMS))
        , outputEvents (*this)
    {
        buildParameterList();
        inputEvents.reserve (maxEventsPerBlock + static_cast<int> (clapParameterIds.size()));

        noteExpressionSupported = hasCLAPNoteInput (plugin);
        doublePrecisionSupported = allAudioPortsSupport64Bits (plugin);

        setNonRealtime (context.isNonRealtime);
        startTimerHz (30);
    }

    ~CLAPInstance() override
    {
        stopTimer();
        releaseResources();

        yupHost->plugin = nullptr;
        clapPlugin->destroy (clapPlugin);
    }

    //==============================================================================

    void prepareToPlay (const AudioSpec& spec) override
    {
        const ScopedLock lock (getProcessLock());

        releaseResources();
        lastSpec = spec;

        setProcessingPrecision (hostContext.preferDoublePrecision && supportsDoublePrecisionProcessing()
                                    ? ProcessingPrecision::doublePrecision
                                    : ProcessingPrecision::singlePrecision);

        updateRenderMode();
        prepareAudioPorts (spec.maxBlockSize);
        outputMidiBuffer.ensureSize (maxEventsPerBlock * 8);

        if (! clapPlugin->activate (clapPlugin, spec.sampleRate, 1, static_cast<uint32_t> (jmax (1, spec.maxBlockSize))))
        {
            YUP_MODULE_DBG (PLUGIN_HOST_CLAP, "activate failed: " << getName());
            return;
        }

        activated = true;
        processingStarted = false;
        steadyTime = 0;

        yupHost->threadPool.start (clapPlugin, jlimit (0, 8, SystemStats::getNumCpus() - 1));
        yupHost->latencyChanged.store (true);
    }

    void releaseResources() override
    {
        const ScopedLock lock (getProcessLock());

        if (! activated)
            return;

        if (processingStarted)
        {
            // Holding the process lock excludes the audio thread, so this thread stands in for it
            const ScopedCLAPAudioThread audioThread;
            clapPlugin->stop_processing (clapPlugin);
            processingStarted = false;
        }

        yupHost->threadPool.stop();
        clapPlugin->deactivate (clapPlugin);
        activated = false;
    }

    void processBlock (AudioProcessContext<float>& context) override
    {
        ScopedNoDenormals noDenormals;

        const ScopedTryLock lock (getProcessLock());
        if (! lock.isLocked() || isSuspended() || ! activated)
        {
            context.audio.clear();
            context.midi.clear();
            return;
        }

        if (isBypassed() && bypassParameterIndex < 0)
        {
            processBlockBypassed (context);
            return;
        }

        if (! isUsingDoublePrecision())
        {
            processWithPlugin (context);
            return;
        }

        auto& audioBuffer = context.audio;
        doublePrecisionBuffer.makeCopyOf (audioBuffer, true);

        AudioProcessContext<double> doubleContext { doublePrecisionBuffer, context.midi, context.params, context.playHead };
        doubleContext.noteExpressions = context.noteExpressions;
        processWithPlugin (doubleContext);

        const int numChannels = jmin (audioBuffer.getNumChannels(), doublePrecisionBuffer.getNumChannels());
        const int numSamples = jmin (audioBuffer.getNumSamples(), doublePrecisionBuffer.getNumSamples());

        for (int channel = 0; channel < numChannels; ++channel)
            FloatVectorOperations::convertDoubleToFloat (audioBuffer.getWritePointer (channel), doublePrecisionBuffer.getReadPointer (channel), numSamples);
    }

    void processBlock (AudioProcessContext<double>& context) override
    {
        ScopedNoDenormals noDenormals;

        const ScopedTryLock lock (getProcessLock());
        if (! lock.isLocked() || isSuspended() || ! activated || ! isUsingDoublePrecision())
        {
            jassert (! activated || isUsingDoublePrecision());
            context.audio.clear();
            context.midi.clear();
            return;
        }

        if (isBypassed() && bypassParameterIndex < 0)
        {
            processBlockBypassed (context);
            return;
        }

        processWithPlugin (context);
    }

    bool supportsDoublePrecisionProcessing() const override { return doublePrecisionSupported; }

    bool supportsNoteExpression() const override { return noteExpressionSupported; }

    //==============================================================================

    int getCurrentPreset() const noexcept override { return currentPreset; }

    void setCurrentPreset (int index) noexcept override { currentPreset = index; }

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    //==============================================================================

    Result loadStateFromMemory (const MemoryBlock& memoryBlock) override
    {
        auto* stateExtension = getCLAPExtension<clap_plugin_state_t> (clapPlugin, CLAP_EXT_STATE);
        if (stateExtension == nullptr)
            return Result::fail ("Plugin does not support CLAP state extension");

        if (memoryBlock.isEmpty())
            return Result::fail ("CLAP state is empty");

        MemoryInputStream input (memoryBlock, false);

        clap_istream_t stream {};
        stream.ctx = &input;
        stream.read = [] (const clap_istream_t* s, void* buffer, uint64_t size) -> int64_t
        {
            auto* source = static_cast<MemoryInputStream*> (s->ctx);
            const auto toRead = static_cast<int> (jmin (size, static_cast<uint64_t> (std::numeric_limits<int>::max())));
            return static_cast<int64_t> (source->read (buffer, toRead));
        };

        if (! stateExtension->load (clapPlugin, &stream))
            return Result::fail ("CLAP state load failed");

        refreshParameterValues();
        updateHostDisplay (ChangeDetails().withParameterValuesChanged (true));
        return Result::ok();
    }

    Result saveStateIntoMemory (MemoryBlock& memoryBlock) override
    {
        auto* stateExtension = getCLAPExtension<clap_plugin_state_t> (clapPlugin, CLAP_EXT_STATE);
        if (stateExtension == nullptr)
            return Result::fail ("Plugin does not support CLAP state extension");

        memoryBlock.reset();

        clap_ostream_t stream {};
        stream.ctx = &memoryBlock;
        stream.write = [] (const clap_ostream_t* s, const void* buffer, uint64_t size) -> int64_t
        {
            static_cast<MemoryBlock*> (s->ctx)->append (buffer, static_cast<std::size_t> (size));
            return static_cast<int64_t> (size);
        };

        return stateExtension->save (clapPlugin, &stream)
                 ? Result::ok()
                 : Result::fail ("CLAP state save failed");
    }

    //==============================================================================

    bool hasEditor() const override
    {
        return canCreateCLAPEditor (clapPlugin);
    }

    AudioProcessorEditor* createEditor() override
    {
        if (auto editor = CLAPEditor::create (clapPlugin, *yupHost))
            return editor.release();

        return nullptr;
    }

    int getLatencySamples() override
    {
        if (activated && MessageManager::existsAndIsCurrentThread())
            return queryPluginLatency();

        return AudioPluginInstance::getLatencySamples();
    }

    int getTailSamples() override
    {
        auto* tailExtension = getCLAPExtension<clap_plugin_tail_t> (clapPlugin, CLAP_EXT_TAIL);
        if (! activated || tailExtension == nullptr || tailExtension->get == nullptr)
            return 0;

        return static_cast<int> (jmin (tailExtension->get (clapPlugin), static_cast<uint32_t> (std::numeric_limits<int>::max())));
    }

    void hostInfoChanged (const HostInfo& newInfo) override
    {
        yupHost->trackInfo = newInfo;

        if (auto* trackInfo = getCLAPExtension<clap_plugin_track_info_t> (clapPlugin, CLAP_EXT_TRACK_INFO); trackInfo != nullptr && trackInfo->changed != nullptr)
            trackInfo->changed (clapPlugin);
    }

    //==============================================================================

    static std::unique_ptr<CLAPInstance> create (const AudioPluginDescription& desc,
                                                 const AudioPluginHostContext& context)
    {
        auto mod = CLAPModule::load (File (desc.fileOrBundlePath));
        if (mod == nullptr)
            return nullptr;

        const clap_plugin_factory_t* factory = reinterpret_cast<const clap_plugin_factory_t*> (
            mod->entry->get_factory (CLAP_PLUGIN_FACTORY_ID));

        if (factory == nullptr)
            return nullptr;

        auto yupHost = std::make_unique<YUPCLAPHost> (context);

        const uint32_t count = factory->get_plugin_count (factory);
        for (uint32_t i = 0; i < count; ++i)
        {
            const clap_plugin_descriptor_t* plugDesc = factory->get_plugin_descriptor (factory, i);
            if (plugDesc == nullptr)
                continue;

            if (String (plugDesc->id) != desc.identifier)
                continue;

            const clap_plugin_t* plugin = factory->create_plugin (factory,
                                                                  &yupHost->host,
                                                                  plugDesc->id);
            if (plugin == nullptr)
                continue;

            yupHost->plugin = plugin;

            if (! plugin->init (plugin))
            {
                yupHost->plugin = nullptr;
                plugin->destroy (plugin);
                continue;
            }

            return std::make_unique<CLAPInstance> (withPortCounts (desc, plugin), context, std::move (mod), std::move (yupHost), plugin);
        }

        return nullptr;
    }

private:
    //==============================================================================
    struct OutputEvents
    {
        explicit OutputEvents (CLAPInstance& ownerToUse)
            : owner (ownerToUse)
        {
            list.ctx = this;
            list.try_push = [] (const clap_output_events_t* events, const clap_event_header_t* event) -> bool
            {
                if (event == nullptr)
                    return false;

                static_cast<OutputEvents*> (events->ctx)->handle (*event);
                return true;
            };
        }

        void handle (const clap_event_header_t& event)
        {
            if (event.space_id != CLAP_CORE_EVENT_SPACE_ID)
                return;

            const int samplePosition = static_cast<int> (event.time);

            switch (event.type)
            {
                case CLAP_EVENT_PARAM_VALUE:
                {
                    const auto& param = reinterpret_cast<const clap_event_param_value_t&> (event);
                    owner.handlePluginParameterValue (param.param_id, param.value);
                    break;
                }

                case CLAP_EVENT_PARAM_GESTURE_BEGIN:
                case CLAP_EVENT_PARAM_GESTURE_END:
                {
                    const auto& gesture = reinterpret_cast<const clap_event_param_gesture_t&> (event);
                    owner.toHost.post (owner.findParameterIndex (gesture.param_id),
                                       event.type == CLAP_EVENT_PARAM_GESTURE_BEGIN ? AudioPluginParameterMailbox::gestureBegan
                                                                                    : AudioPluginParameterMailbox::gestureEnded);
                    break;
                }

                case CLAP_EVENT_NOTE_ON:
                case CLAP_EVENT_NOTE_OFF:
                case CLAP_EVENT_NOTE_CHOKE:
                {
                    const auto& note = reinterpret_cast<const clap_event_note_t&> (event);
                    if (midiOutput == nullptr || note.key < 0)
                        break;

                    const int channel = note.channel >= 0 ? note.channel + 1 : 1;
                    const auto velocity = static_cast<float> (note.velocity);

                    midiOutput->addEvent (event.type == CLAP_EVENT_NOTE_ON ? MidiMessage::noteOn (channel, note.key, velocity)
                                                                           : MidiMessage::noteOff (channel, note.key, velocity),
                                          samplePosition);
                    break;
                }

                case CLAP_EVENT_MIDI:
                {
                    const auto& midi = reinterpret_cast<const clap_event_midi_t&> (event);
                    if (midiOutput != nullptr)
                        midiOutput->addEvent (midi.data, MidiMessage::getMessageLengthFromFirstByte (midi.data[0]), samplePosition);
                    break;
                }

                case CLAP_EVENT_MIDI_SYSEX:
                {
                    const auto& sysex = reinterpret_cast<const clap_event_midi_sysex_t&> (event);
                    if (midiOutput != nullptr && sysex.buffer != nullptr)
                        midiOutput->addEvent (MidiMessage::createSysExMessage (sysex.buffer, static_cast<int> (sysex.size)), samplePosition);
                    break;
                }

                default:
                    break;
            }
        }

        CLAPInstance& owner;
        MidiBuffer* midiOutput = nullptr;
        clap_output_events_t list {};
    };

    static constexpr int maxEventsPerBlock = 2048;

    //==============================================================================

    template <typename FloatType>
    void processWithPlugin (AudioProcessContext<FloatType>& context)
    {
        const ScopedCLAPAudioThread audioThread;

        auto& audio = context.audio;
        const int numSamples = audio.getNumSamples();

        auto& scratch = getScratchBuffer<FloatType>();
        auto& channelPointers = getChannelPointers<FloatType>();

        if (numSamples > scratch.getNumSamples())
        {
            jassertfalse;
            audio.clear();
            context.midi.clear();
            return;
        }

        if (! processingStarted)
            processingStarted = clapPlugin->start_processing (clapPlugin);

        if (! processingStarted)
        {
            audio.clear();
            context.midi.clear();
            return;
        }

        int scratchChannel = 0;
        std::size_t pointerIndex = 0;

        // Inputs are always copied, plugins may not support in-place processing on any port
        auto mapPorts = [&] (std::vector<clap_audio_buffer_t>& ports, auto views, bool isInput)
        {
            int flatChannel = 0;

            for (std::size_t portIndex = 0; portIndex < ports.size(); ++portIndex)
            {
                auto& port = ports[portIndex];
                auto** firstChannel = channelPointers.data() + pointerIndex;

                for (uint32_t channel = 0; channel < port.channel_count; ++channel)
                {
                    const FloatType* source = nullptr;

                    if (! views.empty())
                    {
                        if (portIndex < views.size() && static_cast<int> (channel) < views[portIndex].getNumChannels())
                            source = views[portIndex].getChannels()[channel];
                    }
                    else if (flatChannel < audio.getNumChannels())
                    {
                        // Outputs go through getWritePointer so the buffer drops its cleared flag
                        source = isInput ? audio.getReadPointer (flatChannel++) : audio.getWritePointer (flatChannel++);
                    }

                    FloatType* pointer = nullptr;

                    if (! isInput && source != nullptr)
                    {
                        pointer = const_cast<FloatType*> (source);
                    }
                    else
                    {
                        pointer = scratch.getWritePointer (scratchChannel++);

                        if (isInput && source != nullptr)
                            FloatVectorOperations::copy (pointer, source, numSamples);
                        else if (isInput)
                            FloatVectorOperations::clear (pointer, numSamples);
                    }

                    channelPointers[pointerIndex++] = pointer;
                }

                if constexpr (std::is_same_v<FloatType, float>)
                    port.data32 = firstChannel;
                else
                    port.data64 = firstChannel;
            }
        };

        mapPorts (inputPorts, context.inputs, true);
        mapPorts (outputPorts, context.outputs, false);

        prepareInputEvents (context.params, context.midi, context.noteExpressions);

        const auto transport = createCLAPTransport (context.playHead, getSampleRate());

        clap_process_t process {};
        process.steady_time = steadyTime;
        process.frames_count = static_cast<uint32_t> (numSamples);
        process.transport = transport.has_value() ? &*transport : nullptr;
        process.audio_inputs = inputPorts.empty() ? nullptr : inputPorts.data();
        process.audio_inputs_count = static_cast<uint32_t> (inputPorts.size());
        process.audio_outputs = outputPorts.empty() ? nullptr : outputPorts.data();
        process.audio_outputs_count = static_cast<uint32_t> (outputPorts.size());
        process.in_events = &inputEvents.list;
        process.out_events = &outputEvents.list;

        outputMidiBuffer.clear();
        outputEvents.midiOutput = &outputMidiBuffer;

        const auto status = clapPlugin->process (clapPlugin, &process);

        outputEvents.midiOutput = nullptr;
        steadyTime += numSamples;

        if (status == CLAP_PROCESS_ERROR)
        {
            for (auto& port : outputPorts)
            {
                for (uint32_t channel = 0; channel < port.channel_count; ++channel)
                    FloatVectorOperations::clear (getPortChannel<FloatType> (port, channel), numSamples);
            }
        }

        context.midi.swapWith (outputMidiBuffer);
    }

    void prepareInputEvents (const ParameterChangeBuffer& changes, const MidiBuffer& midi, const NoteExpressionBuffer* expressions)
    {
        inputEvents.clear();
        addChangedParameterValues();

        const auto* change = changes.begin();
        auto midiIterator = midi.begin();
        const NoteExpression* expression = expressions != nullptr ? expressions->begin() : nullptr;
        const NoteExpression* expressionEnd = expressions != nullptr ? expressions->end() : nullptr;

        constexpr int none = std::numeric_limits<int>::max();

        // Merges the three time-sorted sources, notes go before expressions at the same time
        for (;;)
        {
            const int changeTime = change != changes.end() ? change->sampleOffset : none;
            const int midiTime = midiIterator != midi.end() ? (*midiIterator).samplePosition : none;
            const int expressionTime = expression != expressionEnd ? expression->sampleOffset : none;

            if (changeTime == none && midiTime == none && expressionTime == none)
                break;

            if (changeTime <= midiTime && changeTime <= expressionTime)
                addParameterChange (*change++);
            else if (midiTime <= expressionTime)
                inputEvents.addMidiMessage (*midiIterator++);
            else
                inputEvents.addNoteExpression (*expression++);
        }
    }

    // Values changed outside the plugin since they were last sent (host edits, bypass, rescans)
    void addChangedParameterValues()
    {
        const auto params = getParameters();

        for (std::size_t index = 0; index < clapParameterIds.size(); ++index)
        {
            const auto value = params[index]->getValue();

            if (value != lastSentValues[index].exchange (value, std::memory_order_relaxed))
                inputEvents.addParameterValue (0, clapParameterIds[index], clapParameterCookies[index], static_cast<double> (value));
        }
    }

    void addParameterChange (const ParameterChange& change)
    {
        if (! isPositiveAndBelow (change.parameterIndex, static_cast<int> (clapParameterIds.size())))
            return;

        const auto index = static_cast<std::size_t> (change.parameterIndex);
        auto& param = getParameters()[index];
        const auto value = param->convertToDenormalizedValue (change.normalizedValue);

        param->setValue (value);
        lastSentValues[index].store (value, std::memory_order_relaxed);
        inputEvents.addParameterValue (change.sampleOffset, clapParameterIds[index], clapParameterCookies[index], static_cast<double> (value));
    }

    void handlePluginParameterValue (clap_id id, double value)
    {
        const auto index = findParameterIndex (id);
        if (index < 0)
            return;

        const auto plainValue = static_cast<float> (value);
        getParameters()[static_cast<std::size_t> (index)]->setValue (plainValue);
        lastSentValues[static_cast<std::size_t> (index)].store (plainValue, std::memory_order_relaxed);
        toHost.set (index, value);
    }

    int findParameterIndex (clap_id id) const
    {
        const auto iter = parameterIndexById.find (id);
        return iter != parameterIndexById.end() ? iter->second : -1;
    }

    //==============================================================================

    void prepareAudioPorts (int maxBlockSize)
    {
        auto* portsExtension = getCLAPExtension<clap_plugin_audio_ports_t> (clapPlugin, CLAP_EXT_AUDIO_PORTS);

        auto fillPorts = [&] (bool isInput, std::vector<clap_audio_buffer_t>& ports)
        {
            ports.clear();
            int totalChannels = 0;

            const uint32_t count = portsExtension != nullptr ? portsExtension->count (clapPlugin, isInput) : 0;
            for (uint32_t i = 0; i < count; ++i)
            {
                clap_audio_port_info_t info {};
                if (! portsExtension->get (clapPlugin, i, isInput, &info))
                    info.channel_count = 0;

                auto& port = ports.emplace_back();
                port = {};
                port.channel_count = info.channel_count;
                totalChannels += static_cast<int> (info.channel_count);
            }

            return totalChannels;
        };

        const int totalChannels = fillPorts (true, inputPorts) + fillPorts (false, outputPorts);

        channelPointers32.assign (static_cast<std::size_t> (totalChannels), nullptr);
        channelPointers64.assign (static_cast<std::size_t> (totalChannels), nullptr);

        if (isUsingDoublePrecision())
        {
            scratchBuffer64.setSize (jmax (1, totalChannels), maxBlockSize, false, true, false);
            scratchBuffer32.setSize (0, 0);

            const auto& layout = getBusLayout();
            doublePrecisionBuffer.setSize (jmax (1, layout.getNumAudioInputChannels(), layout.getNumAudioOutputChannels()), maxBlockSize, false, true, false);
        }
        else
        {
            scratchBuffer32.setSize (jmax (1, totalChannels), maxBlockSize, false, true, false);
            scratchBuffer64.setSize (0, 0);
        }
    }

    template <typename FloatType>
    AudioBuffer<FloatType>& getScratchBuffer() noexcept
    {
        if constexpr (std::is_same_v<FloatType, float>)
            return scratchBuffer32;
        else
            return scratchBuffer64;
    }

    template <typename FloatType>
    std::vector<FloatType*>& getChannelPointers() noexcept
    {
        if constexpr (std::is_same_v<FloatType, float>)
            return channelPointers32;
        else
            return channelPointers64;
    }

    template <typename FloatType>
    static FloatType* getPortChannel (const clap_audio_buffer_t& port, uint32_t channel) noexcept
    {
        if constexpr (std::is_same_v<FloatType, float>)
            return port.data32[channel];
        else
            return port.data64[channel];
    }

    //==============================================================================

    void timerCallback() override
    {
        auto& host = *yupHost;

        if (host.callbackRequested.exchange (false))
            clapPlugin->on_main_thread (clapPlugin);

        if (host.restartRequested.exchange (false) && activated && lastSpec.has_value())
            prepareToPlay (*lastSpec);

        if (const auto flags = host.audioPortsRescanFlags.exchange (0); flags != 0)
            reconfigureAudioPorts (flags);

        if (const auto flags = host.paramsRescanFlags.exchange (0); flags != 0)
            rescanParameters (flags);

        flushParametersWhileInactive (host.flushRequested.exchange (false));

        toHost.flush ([this] (int index, double value, uint8 flags)
        {
            auto& param = getParameters()[static_cast<std::size_t> (index)];

            if ((flags & AudioPluginParameterMailbox::gestureBegan) != 0)
                param->beginChangeGesture();

            if ((flags & AudioPluginParameterMailbox::valueChanged) != 0)
            {
                param->setValueNotifyingHost (static_cast<float> (value));

                if (index == bypassParameterIndex)
                    setBypassed (param->getNormalizedValue() >= 0.5f);
            }

            if ((flags & AudioPluginParameterMailbox::gestureEnded) != 0)
                param->endChangeGesture();
        });

        if (host.latencyChanged.exchange (false) && activated)
            setLatencySamples (queryPluginLatency());

        ChangeDetails details;
        details = details.withTailChanged (host.tailChanged.exchange (false));
        details = details.withNonParameterStateChanged (host.stateDirty.exchange (false));

        if (details.tailChanged || details.nonParameterStateChanged)
            updateHostDisplay (details);
    }

    // While deactivated the plugin only learns about value changes through params.flush
    void flushParametersWhileInactive (bool flushRequested)
    {
        if (activated || paramsExtension == nullptr || paramsExtension->flush == nullptr)
            return;

        const ScopedLock lock (getProcessLock());

        if (activated)
            return;

        inputEvents.clear();
        addChangedParameterValues();

        if (inputEvents.isEmpty() && ! flushRequested)
            return;

        paramsExtension->flush (clapPlugin, &inputEvents.list, &outputEvents.list);
    }

    void rescanParameters (uint32_t flags)
    {
        if ((flags & CLAP_PARAM_RESCAN_ALL) != 0)
        {
            YUP_MODULE_DBG (PLUGIN_HOST_CLAP, "plugin changed its parameter list, keeping the previous one: " << getName());
            refreshParameterCookies();
        }

        ChangeDetails details;

        if ((flags & (CLAP_PARAM_RESCAN_VALUES | CLAP_PARAM_RESCAN_ALL)) != 0)
        {
            refreshParameterValues();
            details = details.withParameterValuesChanged (true);
        }

        if ((flags & (CLAP_PARAM_RESCAN_TEXT | CLAP_PARAM_RESCAN_INFO | CLAP_PARAM_RESCAN_ALL)) != 0)
            details = details.withParameterInfoChanged (true);

        updateHostDisplay (details);
    }

    // Cookies are only invalidated by CLAP_PARAM_RESCAN_ALL, which is only allowed while deactivated
    void refreshParameterCookies()
    {
        const uint32_t count = paramsExtension->count (clapPlugin);
        for (uint32_t i = 0; i < count; ++i)
        {
            clap_param_info_t info {};
            if (! paramsExtension->get_info (clapPlugin, i, &info))
                continue;

            if (const auto it = parameterIndexById.find (info.id); it != parameterIndexById.end())
                clapParameterCookies[static_cast<std::size_t> (it->second)] = info.cookie;
        }
    }

    void refreshParameterValues()
    {
        if (paramsExtension == nullptr || paramsExtension->get_value == nullptr)
            return;

        const auto params = getParameters();

        for (std::size_t index = 0; index < clapParameterIds.size(); ++index)
        {
            double value = 0.0;
            if (! paramsExtension->get_value (clapPlugin, clapParameterIds[index], &value))
                continue;

            params[index]->setValue (static_cast<float> (value));
            lastSentValues[index].store (static_cast<float> (value), std::memory_order_relaxed);
        }
    }

    void reconfigureAudioPorts (uint32_t rescanFlags)
    {
        const ScopedLock lock (getProcessLock());

        const bool namesOnly = (rescanFlags & ~static_cast<uint32_t> (CLAP_AUDIO_PORTS_RESCAN_NAMES)) == 0;
        const bool wasActivated = activated;

        if (! namesOnly)
            releaseResources();

        const auto newLayout = buildBusLayout (clapPlugin);

        if (! newLayout.hasSameBusStructure (getBusLayout()))
            YUP_MODULE_DBG (PLUGIN_HOST_CLAP, "plugin changed its port structure, keeping the previous layout: " << getName());
        else if (const auto result = setBusLayout (newLayout); result.failed())
            YUP_MODULE_DBG (PLUGIN_HOST_CLAP, "bus layout update failed: " << result.getErrorMessage());

        if (! namesOnly && wasActivated && lastSpec.has_value())
            prepareToPlay (*lastSpec);
    }

    void bypassStateChanged() override
    {
        if (bypassParameterIndex < 0)
            return;

        // The changed value reaches the plugin with the next process call or flush
        auto& param = getParameters()[static_cast<std::size_t> (bypassParameterIndex)];
        param->setNormalizedValue (isBypassed() ? 1.0f : 0.0f);
    }

    int queryPluginLatency() const
    {
        auto* latencyExtension = getCLAPExtension<clap_plugin_latency_t> (clapPlugin, CLAP_EXT_LATENCY);
        if (latencyExtension == nullptr || latencyExtension->get == nullptr)
            return 0;

        return static_cast<int> (jmin (latencyExtension->get (clapPlugin), static_cast<uint32_t> (std::numeric_limits<int>::max())));
    }

    //==============================================================================

    static AudioPluginDescription withPortCounts (AudioPluginDescription desc, const clap_plugin_t* plugin)

    {
        if (auto* portsExt = static_cast<const clap_plugin_audio_ports_t*> (plugin->get_extension (plugin, CLAP_EXT_AUDIO_PORTS)))
        {
            const auto countChannels = [&] (bool isInput)
            {
                int numChannels = 0;

                for (uint32_t i = 0; i < portsExt->count (plugin, isInput); ++i)
                {
                    clap_audio_port_info_t info {};
                    if (portsExt->get (plugin, i, isInput, &info))
                        numChannels += static_cast<int> (info.channel_count);
                }

                return numChannels;
            };

            desc.numInputChannels = countChannels (true);
            desc.numOutputChannels = countChannels (false);
        }

        if (auto* notePortsExt = static_cast<const clap_plugin_note_ports_t*> (plugin->get_extension (plugin, CLAP_EXT_NOTE_PORTS)))
        {
            desc.numMidiInputPorts = static_cast<int> (notePortsExt->count (plugin, true));
            desc.numMidiOutputPorts = static_cast<int> (notePortsExt->count (plugin, false));
        }

        return desc;
    }

    static AudioBusLayout buildBusLayout (const clap_plugin_t* plugin)
    {
        std::vector<AudioBus> inputs, outputs;

        auto* portsExt = reinterpret_cast<const clap_plugin_audio_ports_t*> (
            plugin->get_extension (plugin, CLAP_EXT_AUDIO_PORTS));

        if (portsExt != nullptr)
        {
            const uint32_t numInputs = portsExt->count (plugin, true);
            for (uint32_t i = 0; i < numInputs; ++i)
            {
                clap_audio_port_info_t info {};
                portsExt->get (plugin, i, true, &info);
                inputs.emplace_back (String (info.name), AudioBus::Type::Audio, AudioBus::Direction::Input, static_cast<int> (info.channel_count));
            }

            const uint32_t numOutputs = portsExt->count (plugin, false);
            for (uint32_t i = 0; i < numOutputs; ++i)
            {
                clap_audio_port_info_t info {};
                portsExt->get (plugin, i, false, &info);
                outputs.emplace_back (String (info.name), AudioBus::Type::Audio, AudioBus::Direction::Output, static_cast<int> (info.channel_count));
            }
        }

        auto* notePortsExt = reinterpret_cast<const clap_plugin_note_ports_t*> (
            plugin->get_extension (plugin, CLAP_EXT_NOTE_PORTS));

        if (notePortsExt != nullptr)
        {
            const uint32_t numInputs = notePortsExt->count (plugin, true);
            for (uint32_t i = 0; i < numInputs; ++i)
            {
                clap_note_port_info_t info {};
                if (notePortsExt->get (plugin, i, true, &info))
                    inputs.emplace_back (String (info.name), AudioBus::Type::Midi, AudioBus::Direction::Input, 1);
            }

            const uint32_t numOutputs = notePortsExt->count (plugin, false);
            for (uint32_t i = 0; i < numOutputs; ++i)
            {
                clap_note_port_info_t info {};
                if (notePortsExt->get (plugin, i, false, &info))
                    outputs.emplace_back (String (info.name), AudioBus::Type::Midi, AudioBus::Direction::Output, 1);
            }
        }

        return AudioBusLayout (std::move (inputs), std::move (outputs));
    }

    void buildParameterList()
    {
        if (paramsExtension == nullptr || paramsExtension->count == nullptr || paramsExtension->get_info == nullptr)
            return;

        const uint32_t count = paramsExtension->count (clapPlugin);
        for (uint32_t i = 0; i < count; ++i)
        {
            clap_param_info_t info {};
            if (! paramsExtension->get_info (clapPlugin, i, &info))
                continue;

            const auto id = info.id;
            const bool isStepped = (info.flags & CLAP_PARAM_IS_STEPPED) != 0;
            const bool isBypass = (info.flags & CLAP_PARAM_IS_BYPASS) != 0 && bypassParameterIndex < 0;
            const auto minValue = static_cast<float> (info.min_value);
            const auto maxValue = jmax (minValue, static_cast<float> (info.max_value));

            AudioParameterBuilder builder;
            builder.withID (String (static_cast<int64> (id)))
                .withName (String (info.name))
                .withRange (isStepped ? NormalisableRange<float> (minValue, maxValue, 1.0f) : NormalisableRange<float> (minValue, maxValue))
                .withDefault (static_cast<float> (info.default_value))
                .withAutomatable ((info.flags & CLAP_PARAM_IS_AUTOMATABLE) != 0)
                .withReadOnly ((info.flags & CLAP_PARAM_IS_READONLY) != 0)
                .withStepped (isStepped)
                .withEnum ((info.flags & CLAP_PARAM_IS_ENUM) != 0)
                .withModulatable ((info.flags & CLAP_PARAM_IS_MODULATABLE) != 0)
                .withPerNoteModulatable ((info.flags & (CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID | CLAP_PARAM_IS_MODULATABLE_PER_KEY)) != 0)
                .withBypass (isBypass)
                .withValueToString ([this, id] (float value)
            {
                return getParameterText (id, value);
            })
                .withStringToValue ([this, id] (const String& text)
            {
                return getParameterValueFromText (id, text);
            });

            if (id <= AudioParameter::maximumHostParameterID)
                builder.withHostID (id);

            if (const String modulePath (info.module); modulePath.isNotEmpty())
                builder.withModulePath (modulePath);

            if (isBypass)
                bypassParameterIndex = static_cast<int> (clapParameterIds.size());

            parameterIndexById[id] = static_cast<int> (clapParameterIds.size());
            clapParameterIds.push_back (id);
            clapParameterCookies.push_back (info.cookie);
            addParameter (builder.build());
        }

        lastSentValues = std::make_unique<std::atomic<float>[]> (clapParameterIds.size());
        toHost.resize (static_cast<int> (clapParameterIds.size()));

        refreshParameterValues();
    }

    String getParameterText (clap_id id, float value) const
    {
        char text[256] {};

        if (paramsExtension->value_to_text != nullptr && paramsExtension->value_to_text (clapPlugin, id, static_cast<double> (value), text, sizeof (text)))
            return String::fromUTF8 (text);

        return String (value, 3);
    }

    float getParameterValueFromText (clap_id id, const String& text) const
    {
        double value = 0.0;

        if (paramsExtension->text_to_value != nullptr && paramsExtension->text_to_value (clapPlugin, id, text.toRawUTF8(), &value))
            return static_cast<float> (value);

        return text.getFloatValue();
    }

    void updateRenderMode()
    {
        if (clapPlugin == nullptr)
            return;

        const auto mode = isNonRealtime() ? CLAP_RENDER_OFFLINE : CLAP_RENDER_REALTIME;
        if (currentRenderMode == mode)
            return;

        auto* renderExt = reinterpret_cast<const clap_plugin_render_t*> (
            clapPlugin->get_extension (clapPlugin, CLAP_EXT_RENDER));

        if (renderExt != nullptr && renderExt->set != nullptr)
        {
            if (mode == CLAP_RENDER_OFFLINE
                && renderExt->has_hard_realtime_requirement != nullptr
                && renderExt->has_hard_realtime_requirement (clapPlugin))
            {
                return;
            }

            if (renderExt->set (clapPlugin, mode))
                currentRenderMode = mode;
        }
    }

    void nonRealtimeStateChanged() override
    {
        updateRenderMode();
    }

    AudioPluginHostContext hostContext;
    std::unique_ptr<CLAPModule> clapModule;
    std::unique_ptr<YUPCLAPHost> yupHost;
    const clap_plugin_t* clapPlugin = nullptr;
    const clap_plugin_params_t* paramsExtension = nullptr;

    CLAPInputEventList inputEvents;
    OutputEvents outputEvents;
    MidiBuffer outputMidiBuffer;

    std::vector<clap_audio_buffer_t> inputPorts;
    std::vector<clap_audio_buffer_t> outputPorts;
    std::vector<float*> channelPointers32;
    std::vector<double*> channelPointers64;
    AudioBuffer<float> scratchBuffer32;
    AudioBuffer<double> scratchBuffer64;
    AudioBuffer<double> doublePrecisionBuffer;

    std::vector<clap_id> clapParameterIds;
    std::vector<void*> clapParameterCookies;
    std::unordered_map<clap_id, int> parameterIndexById;
    std::unique_ptr<std::atomic<float>[]> lastSentValues;
    AudioPluginParameterMailbox toHost;
    int bypassParameterIndex = -1;

    std::optional<AudioSpec> lastSpec;
    int64 steadyTime = 0;
    std::atomic<bool> activated { false };
    bool processingStarted = false;
    bool noteExpressionSupported = false;
    bool doublePrecisionSupported = false;
    clap_plugin_render_mode currentRenderMode = -1;
    int currentPreset = 0;
};

//==============================================================================

CLAPFormat::CLAPFormat() = default;
CLAPFormat::~CLAPFormat() = default;

AudioPluginFormatType CLAPFormat::getFormatType() const
{
    return AudioPluginFormatType::clap;
}

String CLAPFormat::getFormatName() const
{
    return "CLAP";
}

StringArray CLAPFormat::getFileExtensions() const
{
    return { ".clap" };
}

FileSearchPath CLAPFormat::getDefaultSearchPaths() const
{
    FileSearchPath paths;

#if YUP_MAC
    paths.add (File ("/Library/Audio/Plug-Ins/CLAP"));
    paths.add (File::getSpecialLocation (File::userHomeDirectory).getChildFile ("Library/Audio/Plug-Ins/CLAP"));
#elif YUP_WINDOWS
    if (const char* pf = getenv ("CommonProgramFiles"))
        paths.add (File (String (pf) + "\\CLAP"));
    if (const char* appdata = getenv ("APPDATA"))
        paths.add (File (String (appdata) + "\\CLAP"));
#elif YUP_LINUX
    paths.add (File ("/usr/lib/clap"));
    paths.add (File ("/usr/local/lib/clap"));
    paths.add (File::getSpecialLocation (File::userHomeDirectory).getChildFile (".clap"));
#endif

    return paths;
}

ResultValue<std::vector<AudioPluginDescription>> CLAPFormat::scanFile (const File& file)
{
    if (file.getFileExtension().toLowerCase() != ".clap")
        return makeResultValueFail ("Not a CLAP file");

    YUP_MODULE_DBG (PLUGIN_HOST_CLAP, "scanning: " << file.getFullPathName());

    auto mod = CLAPModule::load (file);
    if (mod == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_HOST_CLAP, "failed to load module: " << file.getFullPathName());
        return makeResultValueFail ("Failed to load CLAP module: " + file.getFullPathName());
    }

    const clap_plugin_factory_t* factory = reinterpret_cast<const clap_plugin_factory_t*> (
        mod->entry->get_factory (CLAP_PLUGIN_FACTORY_ID));

    if (factory == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_HOST_CLAP, "no plugin factory in: " << file.getFullPathName());
        return makeResultValueFail ("No plugin factory in: " + file.getFullPathName());
    }

    std::vector<AudioPluginDescription> results;
    const uint32_t count = factory->get_plugin_count (factory);

    for (uint32_t i = 0; i < count; ++i)
    {
        const clap_plugin_descriptor_t* plugDesc = factory->get_plugin_descriptor (factory, i);
        if (plugDesc == nullptr)
            continue;

        AudioPluginDescription desc;
        desc.formatType = AudioPluginFormatType::clap;
        desc.fileOrBundlePath = file.getFullPathName();
        desc.name = String (plugDesc->name);
        desc.vendor = String (plugDesc->vendor);
        desc.version = String (plugDesc->version);
        desc.identifier = String (plugDesc->id);

        bool isNoteEffect = false, isMono = false;

        if (plugDesc->features != nullptr)
        {
            for (int j = 0; plugDesc->features[j] != nullptr; ++j)
            {
                const String feature (plugDesc->features[j]);
                desc.isInstrument = desc.isInstrument || feature == CLAP_PLUGIN_FEATURE_INSTRUMENT;
                desc.isEffect = desc.isEffect || feature == CLAP_PLUGIN_FEATURE_AUDIO_EFFECT;
                isNoteEffect = isNoteEffect || feature == CLAP_PLUGIN_FEATURE_NOTE_EFFECT;
                isMono = isMono || feature == CLAP_PLUGIN_FEATURE_MONO;
            }
        }

        const int numAudioChannels = isMono ? 1 : 2;

        if (desc.isInstrument)
        {
            desc.numOutputChannels = numAudioChannels;
            desc.numMidiInputPorts = 1;
        }
        else if (isNoteEffect && ! desc.isEffect)
        {
            desc.numMidiInputPorts = 1;
            desc.numMidiOutputPorts = 1;
        }
        else
        {
            desc.numInputChannels = numAudioChannels;
            desc.numOutputChannels = numAudioChannels;
        }

        YUP_MODULE_DBG (PLUGIN_HOST_CLAP, "scan found: " << desc.name << " [" << desc.identifier << "]");
        results.push_back (std::move (desc));
    }

    YUP_MODULE_DBG (PLUGIN_HOST_CLAP, "scan complete: " << results.size() << " plugins in " << file.getFileName());

    if (results.empty())
        return makeResultValueFail ("No plugins found in: " + file.getFullPathName());

    return makeResultValueOk (std::move (results));
}

ResultValue<std::unique_ptr<AudioPluginInstance>> CLAPFormat::loadPlugin (
    const AudioPluginDescription& description,
    const AudioPluginHostContext& context)
{
    YUP_MODULE_DBG (PLUGIN_HOST_CLAP, "loading: " << description.name << " [" << description.identifier << "]");

    auto instance = CLAPInstance::create (description, context);

    if (instance == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_HOST_CLAP, "load failed: " << description.name);
        return makeResultValueFail ("Failed to load CLAP plugin: " + description.name);
    }

    YUP_MODULE_DBG (PLUGIN_HOST_CLAP, "loaded: " << description.name);
    return makeResultValueOk (std::move (instance));
}

} // namespace yup
