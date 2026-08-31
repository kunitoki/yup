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

#include <yup_events/yup_events.h>

using namespace yup;

namespace
{

constexpr size_t expectedTextFieldsCount (ToastTemplate::TemplateType type)
{
    switch (type)
    {
        case ToastTemplate::TemplateType::imageAndText01:
        case ToastTemplate::TemplateType::text01:
            return 1;

        case ToastTemplate::TemplateType::imageAndText02:
        case ToastTemplate::TemplateType::imageAndText03:
        case ToastTemplate::TemplateType::text02:
        case ToastTemplate::TemplateType::text03:
            return 2;

        case ToastTemplate::TemplateType::imageAndText04:
        case ToastTemplate::TemplateType::text04:
            return 3;
    }

    return 0;
}

} // namespace

//==============================================================================

class ToastTemplateTests : public ::testing::Test
{
protected:
    ToastTemplate toast;
};

TEST_F (ToastTemplateTests, TextFieldsCountMatchesTemplateType)
{
    constexpr ToastTemplate::TemplateType types[] = {
        ToastTemplate::TemplateType::imageAndText01,
        ToastTemplate::TemplateType::imageAndText02,
        ToastTemplate::TemplateType::imageAndText03,
        ToastTemplate::TemplateType::imageAndText04,
        ToastTemplate::TemplateType::text01,
        ToastTemplate::TemplateType::text02,
        ToastTemplate::TemplateType::text03,
        ToastTemplate::TemplateType::text04
    };

    for (const auto type : types)
    {
        ToastTemplate t (type);
        EXPECT_EQ (expectedTextFieldsCount (type), t.getTextFieldsCount());
    }
}

TEST_F (ToastTemplateTests, DefaultTemplateIsTwoLineText)
{
    EXPECT_EQ (ToastTemplate::TemplateType::text02, toast.getType());
    EXPECT_EQ (2, toast.getTextFieldsCount());
    EXPECT_FALSE (toast.hasImage());
}

TEST_F (ToastTemplateTests, SetAndGetTextFields)
{
    toast.setFirstLine ("First");
    toast.setSecondLine ("Second");
    toast.setThirdLine ("Third");

    EXPECT_EQ (String ("First"), toast.getTextField (ToastTemplate::TextField::firstLine));
    EXPECT_EQ (String ("Second"), toast.getTextField (ToastTemplate::TextField::secondLine));

    // The default template only has two fields, so the third line is ignored.
    EXPECT_TRUE (toast.getTextField (ToastTemplate::TextField::thirdLine).isEmpty());
}

TEST_F (ToastTemplateTests, OutOfRangeTextFieldIsIgnored)
{
    ToastTemplate singleLine (ToastTemplate::TemplateType::text01);
    singleLine.setFirstLine ("Only");

    EXPECT_EQ (1, singleLine.getTextFieldsCount());
    EXPECT_EQ (String ("Only"), singleLine.getTextField (ToastTemplate::TextField::firstLine));

    // Setting a field beyond the template's capacity is a no-op, not a crash.
    singleLine.setSecondLine ("Ignored");
    EXPECT_TRUE (singleLine.getTextField (ToastTemplate::TextField::secondLine).isEmpty());
}

TEST_F (ToastTemplateTests, CopyConstructorCopiesTemplate)
{
    toast.setFirstLine ("First");
    toast.setSecondLine ("Second");
    toast.setAttributionText ("Attribution");
    toast.addAction ("Action");

    const ToastTemplate copy (toast);

    EXPECT_EQ (toast.getTextField (ToastTemplate::TextField::firstLine), copy.getTextField (ToastTemplate::TextField::firstLine));
    EXPECT_EQ (toast.getTextField (ToastTemplate::TextField::secondLine), copy.getTextField (ToastTemplate::TextField::secondLine));
    EXPECT_EQ (toast.getAttributionText(), copy.getAttributionText());
    EXPECT_EQ (toast.getActionsCount(), copy.getActionsCount());
    EXPECT_EQ (toast.getActionLabel (0), copy.getActionLabel (0));
    EXPECT_EQ (toast.getType(), copy.getType());
}

TEST_F (ToastTemplateTests, AttributionText)
{
    EXPECT_TRUE (toast.getAttributionText().isEmpty());

    toast.setAttributionText ("via YUP");
    EXPECT_EQ (String ("via YUP"), toast.getAttributionText());
}

TEST_F (ToastTemplateTests, ImagePathAndCropHint)
{
    EXPECT_FALSE (toast.hasImage());
    EXPECT_TRUE (toast.getImagePath().getFullPathName().isEmpty());

    toast.setImagePath (File ("/tmp/image.png"), ToastTemplate::CropHint::circle);

    EXPECT_EQ (File ("/tmp/image.png"), toast.getImagePath());
    EXPECT_EQ (ToastTemplate::CropHint::circle, toast.getCropHint());
}

TEST_F (ToastTemplateTests, HeroImageAndInlineFlag)
{
    // Hero images only apply to templates that include an image slot, so a
    // text-only template never reports one.
    EXPECT_FALSE (toast.hasHeroImage());

    ToastTemplate withImage (ToastTemplate::TemplateType::imageAndText02);
    EXPECT_FALSE (withImage.hasHeroImage());

    withImage.setHeroImagePath (File ("/tmp/hero.png"), true);

    EXPECT_TRUE (withImage.hasHeroImage());
    EXPECT_TRUE (withImage.isInlineHeroImage());
    EXPECT_EQ (File ("/tmp/hero.png"), withImage.getHeroImagePath());
}

TEST_F (ToastTemplateTests, ImageTemplatesReportHasImage)
{
    ToastTemplate withImage (ToastTemplate::TemplateType::imageAndText02);
    EXPECT_TRUE (withImage.hasImage());

    ToastTemplate textOnly (ToastTemplate::TemplateType::text02);
    EXPECT_FALSE (textOnly.hasImage());
}

TEST_F (ToastTemplateTests, AudioSystemFileAndRawPathAreMutuallyExclusive)
{
    EXPECT_FALSE (toast.getAudioSystemFile().has_value());
    EXPECT_TRUE (toast.getAudioPath().isEmpty());

    toast.setAudioPath (ToastTemplate::AudioSystemFile::mail);
    EXPECT_TRUE (toast.getAudioSystemFile().has_value());
    EXPECT_EQ (ToastTemplate::AudioSystemFile::mail, *toast.getAudioSystemFile());
    EXPECT_TRUE (toast.getAudioPath().isEmpty());

    toast.setAudioPath ("custom/sound.wav");
    EXPECT_FALSE (toast.getAudioSystemFile().has_value());
    EXPECT_EQ (String ("custom/sound.wav"), toast.getAudioPath());
}

TEST_F (ToastTemplateTests, AudioOptionDurationScenarioAndExpiration)
{
    EXPECT_EQ (ToastTemplate::AudioOption::default_, toast.getAudioOption());
    EXPECT_EQ (ToastTemplate::Duration::system, toast.getDuration());
    EXPECT_EQ (ToastTemplate::Scenario::default_, toast.getScenario());
    EXPECT_EQ (0, toast.getExpiration());

    toast.setAudioOption (ToastTemplate::AudioOption::silent);
    toast.setDuration (ToastTemplate::Duration::long_);
    toast.setScenario (ToastTemplate::Scenario::reminder);
    toast.setExpiration (5000);

    EXPECT_EQ (ToastTemplate::AudioOption::silent, toast.getAudioOption());
    EXPECT_EQ (ToastTemplate::Duration::long_, toast.getDuration());
    EXPECT_EQ (ToastTemplate::Scenario::reminder, toast.getScenario());
    EXPECT_EQ (5000, toast.getExpiration());
}

TEST_F (ToastTemplateTests, Actions)
{
    EXPECT_EQ (0, toast.getActionsCount());

    toast.addAction ("Accept");
    toast.addAction ("Decline");

    EXPECT_EQ (2, toast.getActionsCount());
    EXPECT_EQ (String ("Accept"), toast.getActionLabel (0));
    EXPECT_EQ (String ("Decline"), toast.getActionLabel (1));
}

//==============================================================================

class ToastNotificationTests : public ::testing::Test
{
protected:
    void TearDown() override
    {
        ToastNotification::deleteInstance();
    }
};

TEST_F (ToastNotificationTests, SingletonIsStable)
{
    EXPECT_NE (nullptr, ToastNotification::getInstance());
    EXPECT_EQ (ToastNotification::getInstance(), ToastNotification::getInstance());
}

TEST_F (ToastNotificationTests, NotInitializedByDefault)
{
    EXPECT_FALSE (ToastNotification::getInstance()->isInitialized());
}

TEST_F (ToastNotificationTests, ShowToastBeforeInitializeFails)
{
    auto result = ToastNotification::getInstance()->showToast (ToastTemplate());

    EXPECT_TRUE (result.failed());
    EXPECT_EQ (ToastNotification::getErrorDescription (ToastNotification::Error::notInitialized), result.getErrorMessage());
}

TEST_F (ToastNotificationTests, ShowToastCompletionNotInvokedOnSyncFailure)
{
    // When showToast() fails synchronously (backend not initialized), the
    // completion callback is not invoked - the return value carries the error.
    bool completionInvoked = false;

    auto result = ToastNotification::getInstance()->showToast (ToastTemplate(),
                                                               [&] (const ResultValue<int64>&)
    {
        completionInvoked = true;
    });

    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (completionInvoked);
}

TEST_F (ToastNotificationTests, HideToastBeforeInitializeReturnsFalse)
{
    EXPECT_FALSE (ToastNotification::getInstance()->hideToast (42));
}

TEST_F (ToastNotificationTests, ClearIsSafeBeforeInitialize)
{
    EXPECT_NO_THROW (ToastNotification::getInstance()->clear());
}

TEST_F (ToastNotificationTests, SetAppNameAppUserModelIdAndFallbackImage)
{
    auto& instance = *ToastNotification::getInstance();

    instance.setAppName ("MyTestApp");
    EXPECT_EQ (String ("MyTestApp"), instance.getAppName());

    instance.setAppUserModelId ("MyCompany.MyApp");
    EXPECT_EQ (String ("MyCompany.MyApp"), instance.getAppUserModelId());

    // setFallbackImage() has no public getter; just check it doesn't crash.
    EXPECT_NO_THROW (instance.setFallbackImage (File ("/tmp/fallback.png")));
    EXPECT_NO_THROW (instance.setFallbackImage (std::nullopt));
}

TEST_F (ToastNotificationTests, SetPermissionStateChangedCallbackIsAccepted)
{
    // Every backend either stores this callback or ignores it; none of them
    // touch the system permission APIs, so this is safe on every platform.
    EXPECT_NO_THROW (ToastNotification::getInstance()->setPermissionStateChangedCallback (
        [] (ToastNotification::PermissionState) {}));
}

#if YUP_LINUX || YUP_BSD

TEST_F (ToastNotificationTests, GetPermissionStateReportsGrantedOnLinux)
{
    // Linux/BSD has no user-facing notification permission, so the backend
    // reports "granted" synchronously without touching the system.
    bool called = false;
    ToastNotification::PermissionState reported = ToastNotification::PermissionState::notDetermined;

    ToastNotification::getPermissionState ([&] (ToastNotification::PermissionState state)
    {
        called = true;
        reported = state;
    });

    EXPECT_TRUE (called);
    EXPECT_EQ (ToastNotification::PermissionState::granted, reported);
}

TEST_F (ToastNotificationTests, RequestPermissionReportsGrantedOnLinux)
{
    bool called = false;
    ToastNotification::PermissionState reported = ToastNotification::PermissionState::notDetermined;

    ToastNotification::requestPermission ([&] (ToastNotification::PermissionState state)
    {
        called = true;
        reported = state;
    });

    EXPECT_TRUE (called);
    EXPECT_EQ (ToastNotification::PermissionState::granted, reported);
}

#endif // YUP_LINUX || YUP_BSD

TEST_F (ToastNotificationTests, ConfigureAumiBuildsExpectedId)
{
    EXPECT_EQ (String ("Company.Product"),
               ToastNotification::configureAUMI ("Company", "Product"));

    EXPECT_EQ (String ("Company.Product.SubProduct"),
               ToastNotification::configureAUMI ("Company", "Product", "SubProduct"));

    EXPECT_EQ (String ("Company.Product.SubProduct.1.2.3"),
               ToastNotification::configureAUMI ("Company", "Product", "SubProduct", "1.2.3"));

    // The version is only appended when a sub-product is present.
    EXPECT_EQ (String ("Company.Product"),
               ToastNotification::configureAUMI ("Company", "Product", "", "1.2.3"));
}

TEST_F (ToastNotificationTests, ErrorDescriptionsAreAvailable)
{
    constexpr ToastNotification::Error errors[] = {
        ToastNotification::Error::noError,
        ToastNotification::Error::notInitialized,
        ToastNotification::Error::systemNotSupported,
        ToastNotification::Error::shellLinkNotCreated,
        ToastNotification::Error::invalidAppUserModelID,
        ToastNotification::Error::invalidParameters,
        ToastNotification::Error::invalidHandler,
        ToastNotification::Error::notDisplayed,
        ToastNotification::Error::permissionDenied,
        ToastNotification::Error::unknownError
    };

    for (const auto error : errors)
        EXPECT_FALSE (ToastNotification::getErrorDescription (error).isEmpty());

    EXPECT_EQ (String ("The user denied or revoked notification permission"),
               ToastNotification::getErrorDescription (ToastNotification::Error::permissionDenied));
}
