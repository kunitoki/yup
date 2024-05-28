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
/** Main application class for the YUPApplication, extending JUCE application functionality.

    This class defines the lifecycle and behavior of the application using the JUCE framework, handling
    events such as application start and quit, as well as managing application instances.
 */
class JUCE_API YUPApplication : public JUCEApplicationBase
{
public:
    //==============================================================================
    /** Constructor for the YUPApplication.

        Initializes the application, potentially setting up necessary state or resources.
     */
    YUPApplication();

    /** Destructor for the YUPApplication.

        Cleans up any resources or state before the application is closed.
     */
    ~YUPApplication() override;

    //==============================================================================
    /** Determines if multiple instances of the application are allowed.

        @return True if more than one instance is allowed, false otherwise.
     */
    bool moreThanOneInstanceAllowed() override;

    /** Called when another instance of the application has been started.

        This method can handle arguments passed from the new instance, allowing for inter-instance communication.

        @param commandLine The command line string from the starting instance.
     */
    void anotherInstanceStarted (const String& commandLine) override;

    //==============================================================================
    /** Called when the system requests the application to quit.

        This method can perform cleanup operations before the application exits. If the quit request needs to be
        denied (to ask user for unsaved changes, etc.), this method can be overridden to handle such scenarios.
     */
    void systemRequestedQuit() override;

    //==============================================================================
    /** Called when the application is suspended.

        This could occur when the application is minimized or not the focus on the device, allowing for resource
        conservation operations to take place.
     */
    void suspended() override;

    /** Called when the application is resumed from a suspended state.

        This method can be used to re-initialize resources or state that was released or altered upon suspension.
     */
    void resumed() override;

    //==============================================================================
    /** Called when an unhandled exception occurs.

        This method provides a way to handle exceptions that are not caught elsewhere in the application,
        possibly allowing the application to recover or to log additional diagnostic information.

        @param e Pointer to the exception object.
        @param sourceFilename Name of the file where the exception occurred.
        @param lineNumber Line number in the source file where the exception occurred.
     */
    void unhandledException (const std::exception*,
                             const String& sourceFilename,
                             int lineNumber) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YUPApplication)
};

/** These are called automatically by the YUPApplication class but must be called by plugins. */
void staticInitialisation();
void staticFinalisation();

} // namespace yup
