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

/*
    The Apple backend uses the UserNotifications framework (macOS 10.14+,
    iOS 10+). Notifications are shown as banners even while the app is in the
    foreground, activation and action callbacks are forwarded to the template
    callbacks, and expiration is implemented by removing the delivered
    notification after the timeout, since UNUserNotificationCenter has no
    native expiration concept.

    If the application installs its own UNUserNotificationCenter delegate, the
    backend delegate is not installed, activation callbacks are not delivered,
    and the retained callback entries are reclaimed when the notification is
    hidden or cleared.
*/

#if YUP_MAC || YUP_IOS

namespace yup::detail
{

namespace
{

//==============================================================================
// A copy of the event callbacks of a ToastTemplate, retained until the
// notification is gone.
struct ToastCallbacks
{
    std::function<void()> onActivated;
    std::function<void (int)> onActivatedWithAction;
    std::function<void (ToastTemplate::DismissalReason)> onDismissed;
    std::function<void()> onFailed;
};

//==============================================================================
// The per-process state of the toast backend.
struct AppleToastState
{
    CriticalSection lock;
    std::map<int64, String> idToIdentifier;
    std::map<String, ToastCallbacks> identifierToCallbacks;
    yup::Atomic<int64> nextId { 1 };
    bool initialized { false };
    std::function<void (ToastNotification::PermissionState)> permissionStateChangedCallback;
};

AppleToastState& getToastState()
{
    static AppleToastState state;
    return state;
}

ToastNotification::PermissionState mapAuthorizationStatus (UNAuthorizationStatus status)
{
    switch (status)
    {
        case UNAuthorizationStatusNotDetermined:
            return ToastNotification::PermissionState::notDetermined;
        case UNAuthorizationStatusDenied:
            return ToastNotification::PermissionState::denied;
        case UNAuthorizationStatusAuthorized:
        case UNAuthorizationStatusProvisional:
            return ToastNotification::PermissionState::granted;
        default:
            // Includes UNAuthorizationStatusEphemeral (macOS 12+ / iOS 14+).
            return ToastNotification::PermissionState::granted;
    }
}

} // namespace

} // yup::detail

//==============================================================================
// The delegate object that receives notification presentation and activation
// callbacks from the system. It is retained for the lifetime of the process.
@interface ToastNotificationCenterDelegate : NSObject <UNUserNotificationCenterDelegate>
@end

@implementation ToastNotificationCenterDelegate

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
       willPresentNotification:(UNNotification*)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completionHandler
{
    yup::ignoreUnused (center, notification);

    UNNotificationPresentationOptions options = UNNotificationPresentationOptionSound;

#if defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && __IPHONE_OS_VERSION_MIN_REQUIRED < 140000
    // iOS < 14 has no Banner/List presentation options.
    options |= UNNotificationPresentationOptionAlert;
#else
    options |= UNNotificationPresentationOptionBanner | UNNotificationPresentationOptionList;
#endif

    completionHandler (options);
}

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
    didReceiveNotificationResponse:(UNNotificationResponse*)response
           withCompletionHandler:(void (^)(void))completionHandler
{
    yup::ignoreUnused (center);

    auto& state = yup::detail::getToastState();

    const yup::String identifier = yup::nsStringToYup (response.notification.request.identifier);

    yup::detail::ToastCallbacks callbacks;
    bool foundCallbacks = false;

    {
        yup::ScopedLock lock (state.lock);

        const auto callbacksIter = state.identifierToCallbacks.find (identifier);
        if (callbacksIter != state.identifierToCallbacks.end())
        {
            callbacks = callbacksIter->second;
            foundCallbacks = true;
        }

        state.identifierToCallbacks.erase (identifier);

        for (auto it = state.idToIdentifier.begin(); it != state.idToIdentifier.end(); ++it)
        {
            if (it->second == identifier)
            {
                state.idToIdentifier.erase (it);
                break;
            }
        }
    }

    if (! foundCallbacks)
    {
        completionHandler();
        return;
    }

    if ([response.actionIdentifier isEqualToString:UNNotificationDefaultActionIdentifier])
    {
        if (callbacks.onActivated)
            callbacks.onActivated();
    }
    else if ([response.actionIdentifier isEqualToString:UNNotificationDismissActionIdentifier])
    {
        if (callbacks.onDismissed)
            callbacks.onDismissed (yup::ToastTemplate::DismissalReason::userCanceled);
    }
    else
    {
        NSString* const prefix = @"yup_action_";

        if ([response.actionIdentifier hasPrefix:prefix])
        {
            const int actionIndex = [[response.actionIdentifier substringFromIndex:prefix.length] intValue];

            if (callbacks.onActivatedWithAction)
                callbacks.onActivatedWithAction (actionIndex);
        }
    }

    completionHandler();
}

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
    didChangeSettings:(UNNotificationSettings*)settings API_AVAILABLE(macos(12.0), ios(15.0))
{
    yup::ignoreUnused (center);

    auto& state = yup::detail::getToastState();

    std::function<void (yup::ToastNotification::PermissionState)> callback;

    {
        yup::ScopedLock lock (state.lock);
        callback = state.permissionStateChangedCallback;
    }

    if (callback)
        callback (yup::detail::mapAuthorizationStatus (settings.authorizationStatus));
}

@end

namespace yup
{

namespace detail
{

namespace
{

static ToastNotificationCenterDelegate* toastNotificationDelegate = nil;

// The set of notification categories registered by this backend, kept so that
// registering a new category doesn't drop the previously registered ones.
static NSMutableSet<UNNotificationCategory*>* sRegisteredCategories = nil;

//==============================================================================
void runOnMainQueue (dispatch_block_t block)
{
    if (NSThread.isMainThread)
        block();
    else
        dispatch_async (dispatch_get_main_queue(), block);
}

UNNotificationSound* createSound (const ToastTemplate& toast)
{
    if (toast.getAudioOption() == ToastTemplate::AudioOption::silent)
        return nil;

    if (toast.getAudioSystemFile().has_value())
        return [UNNotificationSound defaultSound];

    if (! toast.getAudioPath().isEmpty())
        return [UNNotificationSound soundNamed:yupStringToNS (toast.getAudioPath())];

    return [UNNotificationSound defaultSound];
}

API_AVAILABLE(macos(12.0), ios(15.0))
UNNotificationInterruptionLevel createInterruptionLevel (const ToastTemplate& toast)
{
    switch (toast.getScenario())
    {
        case ToastTemplate::Scenario::default_:
            return UNNotificationInterruptionLevelActive;
        case ToastTemplate::Scenario::alarm:
        case ToastTemplate::Scenario::incomingCall:
        case ToastTemplate::Scenario::reminder:
            return UNNotificationInterruptionLevelTimeSensitive;
    }

    return UNNotificationInterruptionLevelActive;
}

NSString* createCategoryIdentifier (const ToastTemplate& toast)
{
    return [NSString stringWithFormat:@"yup_toast_actions_%d", static_cast<int> (toast.getActionsCount())];
}

void registerCategoryIfNeeded (UNUserNotificationCenter* center, const ToastTemplate& toast)
{
    if (toast.getActionsCount() == 0)
        return;

    if (sRegisteredCategories == nil)
        sRegisteredCategories = [NSMutableSet set];

    NSString* const identifier = createCategoryIdentifier (toast);

    for (UNNotificationCategory* existingCategory in [sRegisteredCategories copy])
    {
        if ([existingCategory.identifier isEqualToString:identifier])
            [sRegisteredCategories removeObject:existingCategory];
    }

    NSMutableArray<UNNotificationAction*>* actions = [NSMutableArray arrayWithCapacity:toast.getActionsCount()];

    for (size_t i = 0; i < toast.getActionsCount(); ++i)
    {
        NSString* actionIdentifier = [NSString stringWithFormat:@"yup_action_%zu", i];
        NSString* title = yupStringToNS (toast.getActionLabel (i));

        [actions addObject:[UNNotificationAction actionWithIdentifier:actionIdentifier
                                                                title:title
                                                              options:UNNotificationActionOptionForeground]];
    }

    UNNotificationCategory* category = [UNNotificationCategory categoryWithIdentifier:identifier
                                                                              actions:actions
                                                                    intentIdentifiers:@[]
                                                                              options:UNNotificationCategoryOptionNone];

    [sRegisteredCategories addObject:category];
    [center setNotificationCategories:sRegisteredCategories];
}

} // namespace

//==============================================================================
Result toastNotificationInitialize (const ToastNotificationSettings&)
{
    auto& state = getToastState();

    if (state.initialized)
        return Result::ok();

    if (toastNotificationDelegate == nil)
    {
        toastNotificationDelegate = [[ToastNotificationCenterDelegate alloc] init];

        runOnMainQueue (^
        {
            UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];

            // Don't clobber a delegate the application may have set itself.
            if (center.delegate == nil)
                center.delegate = toastNotificationDelegate;

            [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionBadge | UNAuthorizationOptionSound)
                                  completionHandler:^(BOOL result, NSError* error) {
                if (result)
                    yup::Logger::outputDebugString ("Got it!");
                else
                    yup::Logger::outputDebugString ("Broken! " + nsStringToYup ([error localizedDescription]));
            }];
        });
    }

    state.initialized = true;
    return Result::ok();
}

//==============================================================================
ResultValue<int64> toastNotificationShow (const ToastTemplate& toast, const ToastNotificationSettings&,
                                          std::function<void (const ResultValue<int64>&)> completion)
{
    auto& state = getToastState();

    if (! state.initialized)
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::notInitialized));

    const int64 id = (state.nextId += 1) - 1;
    NSString* requestIdentifier = [[NSUUID UUID] UUIDString];
    const String yupIdentifier = nsStringToYup (requestIdentifier);

    // The blocks below run asynchronously, after the caller's template may have
    // been destroyed, so they must capture their own copy.
    const ToastTemplate toastCopy (toast);

    // Removes the pending entry from the bookkeeping maps, used when the
    // notification is never submitted (for example when permission is denied).
    const auto removePendingNotification = ^
    {
        ScopedLock lock (state.lock);
        state.identifierToCallbacks.erase (yupIdentifier);

        for (auto it = state.idToIdentifier.begin(); it != state.idToIdentifier.end(); ++it)
        {
            if (it->second == yupIdentifier)
            {
                state.idToIdentifier.erase (it);
                break;
            }
        }
    };

    // Builds the notification and submits it to the center, reporting the
    // outcome through completion.
    const auto submitNotification = ^ (UNUserNotificationCenter* center)
    {
        UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];

        content.title = yupStringToNS (toastCopy.getTextField (ToastTemplate::TextField::firstLine));
        content.body = yupStringToNS (toastCopy.getTextField (ToastTemplate::TextField::secondLine));

        if (content.body.length == 0)
            content.body = yupStringToNS (toastCopy.getAttributionText());

        if (@available (macOS 12.0, iOS 15.0, *))
            content.interruptionLevel = createInterruptionLevel (toastCopy);

        content.sound = createSound (toastCopy);
        content.categoryIdentifier = createCategoryIdentifier (toastCopy);

        if (toastCopy.hasHeroImage() || ! toastCopy.getImagePath().getFullPathName().isEmpty())
        {
            const File sourceFile (toastCopy.hasHeroImage() ? toastCopy.getHeroImagePath() : toastCopy.getImagePath());

            const String extension = sourceFile.getFileExtension().isEmpty() ? ".img" : sourceFile.getFileExtension();
            const File tempCopy = File::createTempFile (extension);

            if (sourceFile.copyFileTo (tempCopy))
            {
                NSError* attachmentError = nil;
                UNNotificationAttachment* const attachment = [UNNotificationAttachment attachmentWithIdentifier:@"yup_image"
                                                                                                          URL:createNSURLFromFile (tempCopy)
                                                                                                      options:nil
                                                                                                        error:&attachmentError];

                if (attachment != nil)
                {
                    content.attachments = @[ attachment ];
                }
                else
                {
                    tempCopy.deleteFile();
                }
            }
        }

        registerCategoryIfNeeded (center, toastCopy);

        UNNotificationRequest* const request = [UNNotificationRequest requestWithIdentifier:requestIdentifier
                                                                                    content:content
                                                                                    trigger:nil];

        [center addNotificationRequest:request
                 withCompletionHandler:^(NSError* error)
        {
            if (error != nil)
            {
                ToastCallbacks callbacks;

                {
                    ScopedLock lock (state.lock);
                    const auto iter = state.identifierToCallbacks.find (yupIdentifier);
                    if (iter != state.identifierToCallbacks.end())
                        callbacks = iter->second;

                    state.identifierToCallbacks.erase (yupIdentifier);

                    for (auto it = state.idToIdentifier.begin(); it != state.idToIdentifier.end(); ++it)
                    {
                        if (it->second == yupIdentifier)
                        {
                            state.idToIdentifier.erase (it);
                            break;
                        }
                    }
                }

                if (callbacks.onFailed)
                    callbacks.onFailed();

                if (completion)
                    completion (makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::notDisplayed)));
            }
            else
            {
                if (completion)
                    completion (makeResultValueOk (id));

                if (toastCopy.getExpiration() > 0)
                {
                    // UNUserNotificationCenter has no expiration concept, so remove
                    // the delivered notification after the requested timeout.
                    dispatch_after (dispatch_time (DISPATCH_TIME_NOW, (int64_t) toastCopy.getExpiration() * NSEC_PER_MSEC),
                                    dispatch_get_main_queue(), ^
                    {
                        UNUserNotificationCenter* const mainCenter = [UNUserNotificationCenter currentNotificationCenter];
                        [mainCenter removeDeliveredNotificationsWithIdentifiers:@[ requestIdentifier ]];
                    });
                }
            }
        }];
    };

    {
        ScopedLock lock (state.lock);
        state.idToIdentifier.emplace (id, yupIdentifier);

        ToastCallbacks callbacks;
        callbacks.onActivated = toastCopy.onActivated;
        callbacks.onActivatedWithAction = toastCopy.onActivatedWithAction;
        callbacks.onDismissed = toastCopy.onDismissed;
        callbacks.onFailed = toastCopy.onFailed;

        state.identifierToCallbacks.emplace (yupIdentifier, std::move (callbacks));
    }

    runOnMainQueue (^
    {
        UNUserNotificationCenter* const center = [UNUserNotificationCenter currentNotificationCenter];
        __weak UNUserNotificationCenter* const weakCenter = center;

        [weakCenter getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings* settings)
        {
            switch (mapAuthorizationStatus (settings.authorizationStatus))
            {
                case ToastNotification::PermissionState::granted:
                    submitNotification (weakCenter);
                    break;

                case ToastNotification::PermissionState::denied:
                    removePendingNotification();
                    if (completion)
                        completion (makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::permissionDenied)));
                    break;

                case ToastNotification::PermissionState::notDetermined:
                    [weakCenter requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionBadge | UNAuthorizationOptionSound)
                                             completionHandler:^(BOOL granted, NSError*)
                    {
                        if (! granted)
                        {
                            removePendingNotification();

                            if (completion)
                                completion (makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::permissionDenied)));

                            return;
                        }

                        submitNotification (weakCenter);
                    }];
                    break;
            }
        }];
    });

    return makeResultValueOk (id);
}

//==============================================================================
bool toastNotificationHide (int64 id)
{
    auto& state = getToastState();

    String identifier;

    {
        ScopedLock lock (state.lock);
        const auto iter = state.idToIdentifier.find (id);
        if (iter == state.idToIdentifier.end())
            return false;

        identifier = iter->second;
        state.idToIdentifier.erase (iter);
        state.identifierToCallbacks.erase (identifier);
    }

    NSString* const nsIdentifier = yupStringToNS (identifier);

    runOnMainQueue (^
    {
        UNUserNotificationCenter* const center = [UNUserNotificationCenter currentNotificationCenter];
        [center removePendingNotificationRequestsWithIdentifiers:@[ nsIdentifier ]];
        [center removeDeliveredNotificationsWithIdentifiers:@[ nsIdentifier ]];
    });

    return true;
}

//==============================================================================
void toastNotificationClear()
{
    auto& state = getToastState();

    {
        ScopedLock lock (state.lock);
        state.idToIdentifier.clear();
        state.identifierToCallbacks.clear();
    }

    runOnMainQueue (^
    {
        UNUserNotificationCenter* const center = [UNUserNotificationCenter currentNotificationCenter];
        [center removeAllPendingNotificationRequests];
        [center removeAllDeliveredNotifications];
    });
}

//==============================================================================
void toastNotificationGetPermissionState (std::function<void (ToastNotification::PermissionState)> callback)
{
    if (! callback)
        return;

    runOnMainQueue (^
    {
        UNUserNotificationCenter* const center = [UNUserNotificationCenter currentNotificationCenter];

        [center getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings* settings)
        {
            callback (mapAuthorizationStatus (settings.authorizationStatus));
        }];
    });
}

//==============================================================================
void toastNotificationRequestPermission (std::function<void (ToastNotification::PermissionState)> callback)
{
    if (! callback)
        return;

    runOnMainQueue (^
    {
        UNUserNotificationCenter* const center = [UNUserNotificationCenter currentNotificationCenter];

        [center getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings* settings)
        {
            const auto permission = mapAuthorizationStatus (settings.authorizationStatus);

            if (permission != ToastNotification::PermissionState::notDetermined)
            {
                if (callback)
                    callback (permission);

                return;
            }

            [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionBadge | UNAuthorizationOptionSound)
                                  completionHandler:^(BOOL, NSError*)
            {
                [center getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings* newSettings)
                {
                    if (callback)
                        callback (mapAuthorizationStatus (newSettings.authorizationStatus));
                }];
            }];
        }];
    });
}

//==============================================================================
void toastNotificationSetPermissionStateChangedCallback (std::function<void (ToastNotification::PermissionState)> callback)
{
    auto& state = getToastState();

    ScopedLock lock (state.lock);
    state.permissionStateChangedCallback = std::move (callback);
}

} // namespace detail
} // namespace yup

#endif // YUP_MAC || YUP_IOS
