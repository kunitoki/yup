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

namespace yup
{

//==============================================================================

class JUCE_API JUCEApplication : public JUCEApplicationBase
{
public:
    JUCEApplication();
    ~JUCEApplication() override;

    bool moreThanOneInstanceAllowed() override;

    void anotherInstanceStarted (const String& commandLine) override;

    void systemRequestedQuit() override;

    void suspended() override;

    void resumed() override;

    void unhandledException (const std::exception*,
                             const String& sourceFilename,
                             int lineNumber) override;

private:
    static void staticInitialisation();
    static void staticFinalisation();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JUCEApplication)
};

} // namespace yup
