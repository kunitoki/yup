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

namespace yup
{

//==============================================================================
/**
    Holds a list of command-line arguments, and provides useful methods for searching
    and operating on them.

    You can create an ArgumentList manually, or give it some argv/argc values from a
    main() function to parse.

    Options start with one or two dashes. An option takes its values either assigned
    (only long options: "--file=a.txt") or as the arguments that follow it ("--file a.txt",
    "--files a.txt b.txt", "-f a.txt"). Values run until the next option; negative numbers
    ("--gain -6") are values, not options.

    A lone "--" ends the options: the arguments after it are never options or option values,
    see getTrailingArguments().

    An option can be given more than once ("--include a --include b"): the single value
    getters use its first occurrence, getValuesForOption() collects the values of all of them.

    The option passed to the lookup methods can also be a list of alternatives separated by
    pipes, e.g. "--file|-f".

    @code
    // myapp --input song.wav --gain -6 --tags rock live --verbose
    ArgumentList args (argc, argv);

    auto input = args.tryGetExistingFileForOption ("--input|-i"); // ResultValue<File>
    auto gain = args.getDoubleValueForOption ("--gain", 0.0);     // -6.0
    auto tags = args.getValuesForOption ("--tags");               // { "rock", "live" }
    auto verbose = args.getBoolValueForOption ("--verbose", false); // true
    @endcode

    @see ConsoleApplication

    @tags{Core}
*/
struct YUP_API ArgumentList
{
    /** Creates an argument list for a given executable. */
    ArgumentList (String executable, StringArray arguments);

    /** Parses a standard argv/argc pair to create an argument list. */
    ArgumentList (int argc, char* argv[]);

    /** Tokenises a string containing all the arguments to create an argument list. */
    ArgumentList (const String& executable, const String& arguments);

    ArgumentList (const ArgumentList&) = default;
    ArgumentList& operator= (const ArgumentList&) = default;

    //==============================================================================
    /**
        One of the arguments in an ArgumentList.

        @tags{Core}
    */
    struct Argument
    {
        /** The original text of this argument. */
        String text;

        /** Resolves this argument as an absolute File, using the current working
            directory as a base for resolving relative paths, and stripping quotes, etc.
        */
        File resolveAsFile() const;

        /** Resolves this argument as an absolute File like resolveAsFile(), failing if the file doesn't exist.
            @see resolveAsExistingFile
        */
        ResultValue<File> tryResolveAsExistingFile() const;

        /** Resolves this argument as an absolute File like resolveAsFile(), failing if the folder doesn't exist.
            @see resolveAsExistingFolder
        */
        ResultValue<File> tryResolveAsExistingFolder() const;

        /** Like tryResolveAsExistingFile(), but calls ConsoleApplication::fail() if the file doesn't exist. */
        File resolveAsExistingFile() const;

        /** Like tryResolveAsExistingFolder(), but calls ConsoleApplication::fail() if the folder doesn't exist. */
        File resolveAsExistingFolder() const;

        /** Returns true if this argument starts with a double dash. */
        bool isLongOption() const;

        /** Returns true if this argument starts with a single dash, and isn't a negative number. */
        bool isShortOption() const;

        /** Returns true if this argument starts with a double dash, followed by the given string. */
        bool isLongOption (const String& optionRoot) const;

        /** Returns the value assigned to a long option, e.g. "bar" for "--foo=bar".
            Returns nullopt if this isn't a long option, or if it has no '=' (an empty string for "--foo=").
        */
        std::optional<String> getLongOptionValue() const;

        /** Returns true if this argument is exactly a single dash followed by the given character, e.g. "-h".
            Short options are never grouped, so "-abc" or "-I/home" don't match 'a' or 'h'.
        */
        bool isShortOption (char shortOptionCharacter) const;

        /** Returns true if this argument starts with one or more dashes, and isn't a negative number. */
        bool isOption() const;

        /** Compares this argument against a string.
            The string may be a pipe-separated list of options, e.g. "--help|-h"
        */
        bool operator== (StringRef stringToCompare) const;
    };

    //==============================================================================
    /** Returns the number of arguments in the list. */
    int size() const;

    /** Returns one of the arguments, or an empty one if the index is out of range. */
    Argument operator[] (int index) const;

    /** Iterates the arguments. */
    auto begin() const noexcept { return arguments.begin(); }

    /** Iterates the arguments. */
    auto end() const noexcept { return arguments.end(); }

    /** Calls ConsoleApplication::fail() unless there are at least the given number of arguments. */
    void checkMinNumArguments (int expectedMinNumberOfArgs) const;

    //==============================================================================
    /** Returns true if the given option appears before any "--".
        @see removeOptionIfFound
    */
    bool containsOption (StringRef option) const;

    /** Removes the first occurrence of the given option (but not its values), returning true if it was found.
        @see containsOption, removeValuesForOption
    */
    bool removeOptionIfFound (StringRef option);

    /** Returns the index of the first occurrence of the given option before any "--", or -1 if there isn't one. */
    int indexOfOption (StringRef option) const;

    /** Calls ConsoleApplication::fail() unless the given option is found. */
    void failIfOptionIsMissing (StringRef option) const;

    /** Returns the arguments after a lone "--", which are never options or option values. */
    StringArray getTrailingArguments() const;

    //==============================================================================
    /** Returns the first value of the first occurrence of the given option.

        That's the assigned value for "--file=foo.txt", or the argument that follows the option
        for "--file foo.txt" and "-f foo.txt", unless that's another option.
        Returns nullopt if the option isn't found or has no value.
    */
    std::optional<String> getValueForOption (StringRef option) const;

    /** Returns all the values of all the occurrences of the given option, in order.

        For example "--include a b --include=c" gives { "a", "b", "c" } for "--include".
    */
    StringArray getValuesForOption (StringRef option) const;

    /** Like getValueForOption(), but also removes the option and that value from the list. */
    std::optional<String> removeValueForOption (StringRef option);

    /** Like getValuesForOption(), but also removes all the occurrences of the option and their values from the list. */
    StringArray removeValuesForOption (StringRef option);

    /** Returns the value of the given option as an integer, or defaultValue if it's missing or not an integer. */
    int getIntValueForOption (StringRef option, int defaultValue) const;

    /** Returns the value of the given option as a double, or defaultValue if it's missing or not a number. */
    double getDoubleValueForOption (StringRef option, double defaultValue) const;

    /** Returns defaultValue if the given option is missing, false if its value is "false", "no", "off" or "0"
        (case insensitive), and true otherwise, e.g. for a plain "--verbose".
    */
    bool getBoolValueForOption (StringRef option, bool defaultValue) const;

    //==============================================================================
    /** Resolves the value of the given option as a file, see getValueForOption().
        Fails if the option is missing or has no value.
    */
    ResultValue<File> tryGetFileForOption (StringRef option) const;

    /** Like tryGetFileForOption(), also failing if the file doesn't exist. */
    ResultValue<File> tryGetExistingFileForOption (StringRef option) const;

    /** Like tryGetFileForOption(), also failing if the folder doesn't exist. */
    ResultValue<File> tryGetExistingFolderForOption (StringRef option) const;

    /** Like tryGetFileForOption(), but calls ConsoleApplication::fail() on failure. */
    File getFileForOption (StringRef option) const;

    /** Like getFileForOption(), also removing the option and its value from the list. */
    File getFileForOptionAndRemove (StringRef option);

    /** Like tryGetExistingFileForOption(), but calls ConsoleApplication::fail() on failure. */
    File getExistingFileForOption (StringRef option) const;

    /** Like getExistingFileForOption(), also removing the option and its value from the list. */
    File getExistingFileForOptionAndRemove (StringRef option);

    /** Like tryGetExistingFolderForOption(), but calls ConsoleApplication::fail() on failure. */
    File getExistingFolderForOption (StringRef option) const;

    /** Like getExistingFolderForOption(), also removing the option and its value from the list. */
    File getExistingFolderForOptionAndRemove (StringRef option);

    //==============================================================================
    /** The name or path of the executable that was invoked, as it was specified on the command-line. */
    String executableName;

    /** The list of arguments (not including the name of the executable that was invoked). */
    Array<Argument> arguments;
};

//==============================================================================
/**
    Represents a the set of commands that a console app can perform, and provides
    helper functions for performing them.

    When using these helper classes to implement a console app, you probably want to
    do something along these lines:

    @code
    int main (int argc, char* argv[])
    {
        ConsoleApplication app;

        app.addHelpCommand ("--help|-h", "Usage:", true);
        app.addVersionCommand ("--version|-v", "MyApp version 1.2.3");

        app.addCommand ({ "--foo",
                          "--foo filename",
                          "Performs a foo operation on the given file",
                          [] (const auto& args) { doFoo (args); }});

        return app.findAndRunCommand (argc, argv);
    }
    @endcode

    @see ArgumentList

    @tags{Core}
*/
struct YUP_API ConsoleApplication
{
    //==============================================================================
    /**
        Represents a command that can be executed if its command-line arguments are matched.

        @see ConsoleApplication::addCommand(), ConsoleApplication::findAndRunCommand()

        @tags{Core}
    */
    struct Command
    {
        /** The option string that must appear in the argument list for this command to be invoked.
            This can also be a list of different versions separated by pipes, e.g. "--help|-h"
        */
        String commandOption;

        /** A description of the command-line arguments needed for this command, which will be
            printed as part of the help text.
        */
        String argumentDescription;

        /** A short (one line) description of this command, which can be printed by
            ConsoleApplication::printCommandList().
        */
        String shortDescription;

        /** A longer description of this command, for use in extended help. */
        String longDescription;

        /** The actual command that should be invoked to perform this action. */
        std::function<void (const ArgumentList&)> command;
    };

    //==============================================================================
    /** Adds a command to the list. */
    void addCommand (Command);

    /** Adds a command to the list, and marks it as one which is invoked if no other
        command matches.
    */
    void addDefaultCommand (Command);

    /** Adds a command that will print the given text in response to the "--version" option. */
    void addVersionCommand (String versionArgument, String versionText);

    /** Adds a help command to the list.
        This command will print the user-supplied message that's passed in here as an
        argument, followed by a list of all the registered commands.
    */
    void addHelpCommand (String helpArgument, String helpMessage, bool makeDefaultCommand);

    /** Prints out the list of commands and their short descriptions in a format that's
        suitable for use as help.
    */
    void printCommandList (const ArgumentList&) const;

    /** Prints out a longer description of a particular command, based on its
        longDescription member.
    */
    void printCommandDetails (const ArgumentList&, const Command&) const;

    //==============================================================================
    /** Throws a failure exception to cause a command-line app to terminate.
        This is intended to be called from code in a Command, so that the
        exception will be automatically caught and turned into a printed error message
        and a return code which will be returned from main().
        @see ConsoleApplication::invokeCatchingFailures()
    */
    [[noreturn]] static void fail (String errorMessage, int returnCode = 1);

    /** Invokes a function, catching any fail() calls that it might trigger, and handling
        them by printing their error message and returning their error code.
        @see ConsoleApplication::fail()
    */
    static int invokeCatchingFailures (std::function<int()>&& functionToCall);

    //==============================================================================
    /** Looks for the first command in the list which matches the given arguments, and
        tries to invoke it.

        If no command is found, and if there is no default command to run, it fails with
        a suitable error message.
        If the command calls the fail() function, this will throw an exception that gets
        automatically caught and handled, and this method will return the error code that
        was passed into the fail() call.

        If optionMustBeFirstArg is true, then only the first argument will be looked at
        when searching the available commands - this lets you do 'git' style commands where
        the executable name is followed by a verb.
    */
    int findAndRunCommand (const ArgumentList&,
                           bool optionMustBeFirstArg = false) const;

    /** Creates an ArgumentList object from the argc and argv variablrs, and invokes
        findAndRunCommand() using it.
    */
    int findAndRunCommand (int argc, char* argv[]) const;

    /** Looks for the first command in the list which matches the given arguments.
        If none is found, this returns either the default command (if one is set)
        or nullptr.
        If optionMustBeFirstArg is true, then only the first argument will be looked at
        when searching the available commands - this lets you do 'git' style commands where
        the executable name is followed by a verb.
    */
    const Command* findCommand (const ArgumentList&, bool optionMustBeFirstArg) const;

    /** Gives read-only access to the list of registered commands. */
    const std::vector<Command>& getCommands() const;

private:
    //==============================================================================
    std::vector<Command> commands;
    int commandIfNoOthersRecognised = -1;
};

} // namespace yup
