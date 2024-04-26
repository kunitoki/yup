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

namespace juce
{

//==============================================================================
/**
    Launches and monitors a child process.

    This class lets you launch an executable, and read its output. You can also
    use it to check whether the child process has finished.

    @tags{Core}
*/
class JUCE_API  DocumentWindow
{
public:
    //==============================================================================
    /** Creates a process object.
        To actually launch the process, use start().
    */
    DocumentWindow();

    /** Destructor.
        Note that deleting this object won't terminate the child process.
    */
    ~DocumentWindow();

    void setWindowTitle (const String& windowTitle);

    void setSize (int width, int height);
    std::tuple<int, int> getSize() const;

    void setVisible (bool shouldBeVisible);

    void close();
    bool shouldClose() const;

    void* nativeHandle() const;

    virtual void mouseDown(int button, int mods, double x, double y);
    virtual void mouseMove(int button, int mods, double x, double y);
    virtual void mouseDrag(int button, int mods, double x, double y);
    virtual void mouseUp(int button, int mods, double x, double y);

    virtual void keyDown(int key, int scancode, int mods, double x, double y);
    virtual void keyUp(int key, int scancode, int mods, double x, double y);

    /** Attempts to launch a child process command.

        The command should be the name of the executable file, followed by any arguments
        that are required.
        If the process has already been launched, this will launch it again. If a problem
        occurs, the method will return false.
        The streamFlags is a combinations of values to indicate which of the child's output
        streams should be read and returned by readProcessOutput().
    */
    //bool start (const String& command, int streamFlags = wantStdOut | wantStdErr);

    /** Attempts to launch a child process command.

        The first argument should be the name of the executable file, followed by any other
        arguments that are needed.
        If the process has already been launched, this will launch it again. If a problem
        occurs, the method will return false.
        The streamFlags is a combinations of values to indicate which of the child's output
        streams should be read and returned by readProcessOutput().
    */
    //bool start (const StringArray& arguments, int streamFlags = wantStdOut | wantStdErr);

    /** Returns true if the child process is alive. */
    //bool isRunning() const;

    /** Attempts to read some output from the child process.
        This will attempt to read up to the given number of bytes of data from the
        process. It returns the number of bytes that were actually read.
    */
    //int readProcessOutput (void* destBuffer, int numBytesToRead);

    /** Blocks until the process has finished, and then returns its complete output
        as a string.
    */
    //String readAllProcessOutput();

    /** Blocks until the process is no longer running. */
    //bool waitForProcessToFinish (int timeoutMs) const;

    /** If the process has finished, this returns its exit code. */
    //uint32 getExitCode() const;

    /** Attempts to kill the child process.
        Returns true if it succeeded. Trying to read from the process after calling this may
        result in undefined behaviour.
    */
    //bool kill();

private:
    //==============================================================================
    struct HeavyweightWindow;
    std::unique_ptr<HeavyweightWindow> heavyweightWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DocumentWindow)
};

} // namespace juce
