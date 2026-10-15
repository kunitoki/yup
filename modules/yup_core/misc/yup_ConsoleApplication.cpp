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

static File resolveFilename (const String& name)
{
    return File::getCurrentWorkingDirectory().getChildFile (name.unquoted());
}

static ResultValue<File> checkFileExists (const File& f)
{
    if (! f.exists())
        return makeResultValueFail ("Could not find file: " + f.getFullPathName());

    return makeResultValueOk (f);
}

static ResultValue<File> checkFolderExists (const File& f)
{
    if (! f.isDirectory())
        return makeResultValueFail ("Could not find folder: " + f.getFullPathName());

    return makeResultValueOk (f);
}

static File getFileOrFail (const ResultValue<File>& result)
{
    if (result.failed())
        ConsoleApplication::fail (result.getErrorMessage());

    return result.getReference();
}

static bool isNumberFormat (const String& s, bool allowFraction)
{
    auto digits = s.trim();

    if (digits.startsWithChar ('-') || digits.startsWithChar ('+'))
        digits = digits.substring (1);

    if (! allowFraction)
        return digits.isNotEmpty() && digits.containsOnly ("0123456789");

    return digits.containsOnly ("0123456789.eE+-")
        && (CharacterFunctions::isDigit (digits[0]) || (digits[0] == '.' && CharacterFunctions::isDigit (digits[1])));
}

static bool isNegativeNumberFormat (StringRef s) { return s[0] == '-' && isNumberFormat (String (s), true); }

static bool isShortOptionFormat (StringRef s) { return s[0] == '-' && s[1] != '-' && ! isNegativeNumberFormat (s); }

static bool isLongOptionFormat (StringRef s) { return s[0] == '-' && s[1] == '-' && s[2] != '-'; }

static bool isOptionFormat (StringRef s) { return s[0] == '-' && ! isNegativeNumberFormat (s); }

//==============================================================================
File ArgumentList::Argument::resolveAsFile() const
{
    return resolveFilename (text);
}

ResultValue<File> ArgumentList::Argument::tryResolveAsExistingFile() const
{
    return checkFileExists (resolveAsFile());
}

ResultValue<File> ArgumentList::Argument::tryResolveAsExistingFolder() const
{
    return checkFolderExists (resolveAsFile());
}

File ArgumentList::Argument::resolveAsExistingFile() const
{
    return getFileOrFail (tryResolveAsExistingFile());
}

File ArgumentList::Argument::resolveAsExistingFolder() const
{
    return getFileOrFail (tryResolveAsExistingFolder());
}

bool ArgumentList::Argument::isLongOption() const { return isLongOptionFormat (text); }

bool ArgumentList::Argument::isShortOption() const { return isShortOptionFormat (text); }

bool ArgumentList::Argument::isOption() const { return isOptionFormat (text); }

bool ArgumentList::Argument::isLongOption (const String& option) const
{
    if (! isLongOptionFormat (option))
    {
        jassert (! isShortOptionFormat (option)); // this will always fail to match
        return isLongOption ("--" + option);
    }

    return text.upToFirstOccurrenceOf ("=", false, false) == option;
}

std::optional<String> ArgumentList::Argument::getLongOptionValue() const
{
    if (! isLongOption())
        return std::nullopt;

    const auto equalsIndex = text.indexOfChar ('=');
    if (equalsIndex < 0)
        return std::nullopt;

    return text.substring (equalsIndex + 1);
}

bool ArgumentList::Argument::isShortOption (char option) const
{
    jassert (option != '-'); // this is probably not what you intended to pass in

    return text.length() == 2 && isShortOption() && text[1] == (yup_wchar) option;
}

bool ArgumentList::Argument::operator== (StringRef wildcard) const
{
    for (auto& o : StringArray::fromTokens (wildcard, "|", {}))
    {
        if (text == o)
            return true;

        if (isLongOptionFormat (o) && isLongOption (o))
            return true;
    }

    return false;
}

//==============================================================================
namespace
{

struct OptionOccurrence
{
    int index = -1;             // Of the option, -1 if not found
    int numArguments = 0;       // The option plus its separate value arguments
    StringArray values;
};

int getTerminatorIndex (const Array<ArgumentList::Argument>& arguments)
{
    for (int i = 0; i < arguments.size(); ++i)
        if (arguments.getReference (i).text == "--")
            return i;

    return arguments.size();
}

OptionOccurrence findOptionOccurrence (const Array<ArgumentList::Argument>& arguments, StringRef option, int startIndex)
{
    jassert (isOptionFormat (option)); // the thing you're searching for must be an option

    const int endIndex = getTerminatorIndex (arguments);

    for (int i = startIndex; i < endIndex; ++i)
    {
        const auto& arg = arguments.getReference (i);
        if (! (arg == option))
            continue;

        OptionOccurrence occurrence { i, 1, {} };

        if (auto assigned = arg.getLongOptionValue())
        {
            occurrence.values.add (*assigned);
            return occurrence;
        }

        for (int v = i + 1; v < endIndex && ! arguments.getReference (v).isOption(); ++v)
        {
            occurrence.values.add (arguments.getReference (v).text);
            ++occurrence.numArguments;
        }

        return occurrence;
    }

    return {};
}

} // namespace

//==============================================================================
ArgumentList::ArgumentList (String exeName, StringArray args)
    : executableName (std::move (exeName))
{
    args.trim();
    args.removeEmptyStrings();

    for (auto& a : args)
        arguments.add ({ a.unquoted() });
}

ArgumentList::ArgumentList (int argc, char* argv[])
    : ArgumentList (argv[0], StringArray (argv + 1, argc - 1))
{
}

ArgumentList::ArgumentList (const String& exeName, const String& args)
    : ArgumentList (exeName, StringArray::fromTokens (args, true))
{
}

int ArgumentList::size() const { return arguments.size(); }

ArgumentList::Argument ArgumentList::operator[] (int index) const { return arguments[index]; }

void ArgumentList::checkMinNumArguments (int expectedMinNumberOfArgs) const
{
    if (size() < expectedMinNumberOfArgs)
        ConsoleApplication::fail ("Not enough arguments!");
}

int ArgumentList::indexOfOption (StringRef option) const
{
    jassert (option == String (option).trim()); // passing non-trimmed strings will always fail to find a match!

    const int endIndex = getTerminatorIndex (arguments);

    for (int i = 0; i < endIndex; ++i)
        if (arguments.getReference (i) == option)
            return i;

    return -1;
}

bool ArgumentList::containsOption (StringRef option) const
{
    return indexOfOption (option) >= 0;
}

bool ArgumentList::removeOptionIfFound (StringRef option)
{
    const auto i = indexOfOption (option);

    if (i >= 0)
        arguments.remove (i);

    return i >= 0;
}

void ArgumentList::failIfOptionIsMissing (StringRef option) const
{
    if (indexOfOption (option) < 0)
        ConsoleApplication::fail ("Expected the option " + option);
}

StringArray ArgumentList::getTrailingArguments() const
{
    StringArray result;

    for (int i = getTerminatorIndex (arguments) + 1; i < arguments.size(); ++i)
        result.add (arguments.getReference (i).text);

    return result;
}

//==============================================================================
std::optional<String> ArgumentList::getValueForOption (StringRef option) const
{
    const auto occurrence = findOptionOccurrence (arguments, option, 0);
    if (occurrence.values.isEmpty())
        return std::nullopt;

    return occurrence.values[0];
}

StringArray ArgumentList::getValuesForOption (StringRef option) const
{
    StringArray result;

    for (auto o = findOptionOccurrence (arguments, option, 0); o.index >= 0; o = findOptionOccurrence (arguments, option, o.index + o.numArguments))
        result.addArray (o.values);

    return result;
}

std::optional<String> ArgumentList::removeValueForOption (StringRef option)
{
    const auto occurrence = findOptionOccurrence (arguments, option, 0);
    if (occurrence.index < 0)
        return std::nullopt;

    arguments.removeRange (occurrence.index, jmin (occurrence.numArguments, 2));

    if (occurrence.values.isEmpty())
        return std::nullopt;

    return occurrence.values[0];
}

StringArray ArgumentList::removeValuesForOption (StringRef option)
{
    StringArray result;

    for (auto o = findOptionOccurrence (arguments, option, 0); o.index >= 0; o = findOptionOccurrence (arguments, option, o.index))
    {
        result.addArray (o.values);
        arguments.removeRange (o.index, o.numArguments);
    }

    return result;
}

int ArgumentList::getIntValueForOption (StringRef option, int defaultValue) const
{
    const auto value = getValueForOption (option);
    if (! value.has_value() || ! isNumberFormat (*value, false))
        return defaultValue;

    return value->trim().trimCharactersAtStart ("+").getIntValue();
}

double ArgumentList::getDoubleValueForOption (StringRef option, double defaultValue) const
{
    const auto value = getValueForOption (option);
    if (! value.has_value() || ! isNumberFormat (*value, true))
        return defaultValue;

    return value->trim().trimCharactersAtStart ("+").getDoubleValue();
}

bool ArgumentList::getBoolValueForOption (StringRef option, bool defaultValue) const
{
    if (! containsOption (option))
        return defaultValue;

    const auto value = getValueForOption (option).value_or (String()).trim();

    return ! (value.equalsIgnoreCase ("false") || value.equalsIgnoreCase ("no") || value.equalsIgnoreCase ("off") || value == "0");
}

//==============================================================================
ResultValue<File> ArgumentList::tryGetFileForOption (StringRef option) const
{
    if (! containsOption (option))
        return makeResultValueFail ("Expected the option " + option);

    const auto value = getValueForOption (option);
    if (! value.has_value() || value->isEmpty())
        return makeResultValueFail ("Expected a filename after the " + option + " option");

    return makeResultValueOk (resolveFilename (*value));
}

ResultValue<File> ArgumentList::tryGetExistingFileForOption (StringRef option) const
{
    auto file = tryGetFileForOption (option);
    if (file.failed())
        return file;

    return checkFileExists (file.getReference());
}

ResultValue<File> ArgumentList::tryGetExistingFolderForOption (StringRef option) const
{
    auto folder = tryGetFileForOption (option);
    if (folder.failed())
        return folder;

    return checkFolderExists (folder.getReference());
}

File ArgumentList::getFileForOption (StringRef option) const
{
    return getFileOrFail (tryGetFileForOption (option));
}

File ArgumentList::getFileForOptionAndRemove (StringRef option)
{
    auto file = getFileForOption (option);
    removeValueForOption (option);
    return file;
}

File ArgumentList::getExistingFileForOption (StringRef option) const
{
    return getFileOrFail (tryGetExistingFileForOption (option));
}

File ArgumentList::getExistingFileForOptionAndRemove (StringRef option)
{
    auto file = getExistingFileForOption (option);
    removeValueForOption (option);
    return file;
}

File ArgumentList::getExistingFolderForOption (StringRef option) const
{
    return getFileOrFail (tryGetExistingFolderForOption (option));
}

File ArgumentList::getExistingFolderForOptionAndRemove (StringRef option)
{
    auto folder = getExistingFolderForOption (option);
    removeValueForOption (option);
    return folder;
}

//==============================================================================
struct ConsoleAppFailureCode
{
    String errorMessage;
    int returnCode;
};

void ConsoleApplication::fail (String errorMessage, int returnCode)
{
    throw ConsoleAppFailureCode { std::move (errorMessage), returnCode };
}

int ConsoleApplication::invokeCatchingFailures (std::function<int()>&& f)
{
    int returnCode = 0;

    try
    {
        returnCode = f();
    }
    catch (const ConsoleAppFailureCode& error)
    {
        std::cerr << error.errorMessage << std::endl;
        returnCode = error.returnCode;
    }

    return returnCode;
}

const ConsoleApplication::Command* ConsoleApplication::findCommand (const ArgumentList& args, bool optionMustBeFirstArg) const
{
    for (auto& c : commands)
    {
        auto index = args.indexOfOption (c.commandOption);

        if (optionMustBeFirstArg ? (index == 0) : (index >= 0))
            return &c;
    }

    if (commandIfNoOthersRecognised >= 0)
        return &commands[(size_t) commandIfNoOthersRecognised];

    return {};
}

int ConsoleApplication::findAndRunCommand (const ArgumentList& args, bool optionMustBeFirstArg) const
{
    return invokeCatchingFailures ([&args, optionMustBeFirstArg, this]
    {
        if (auto c = findCommand (args, optionMustBeFirstArg))
            c->command (args);
        else
            fail ("Unrecognised arguments");

        return 0;
    });
}

int ConsoleApplication::findAndRunCommand (int argc, char* argv[]) const
{
    return findAndRunCommand (ArgumentList (argc, argv));
}

void ConsoleApplication::addCommand (Command c)
{
    commands.emplace_back (std::move (c));
}

void ConsoleApplication::addDefaultCommand (Command c)
{
    commandIfNoOthersRecognised = (int) commands.size();
    addCommand (std::move (c));
}

void ConsoleApplication::addHelpCommand (String arg, String helpMessage, bool makeDefaultCommand)
{
    Command c { arg, arg, "Prints the list of commands", {}, [this, helpMessage] (const ArgumentList& args)
    {
        std::cout << helpMessage << std::endl;
        printCommandList (args);
    } };

    if (makeDefaultCommand)
        addDefaultCommand (std::move (c));
    else
        addCommand (std::move (c));
}

void ConsoleApplication::addVersionCommand (String arg, String versionText)
{
    addCommand ({ arg, arg, "Prints the current version number", {}, [versionText] (const ArgumentList&)
    {
        std::cout << versionText << std::endl;
    } });
}

const std::vector<ConsoleApplication::Command>& ConsoleApplication::getCommands() const
{
    return commands;
}

static String getExeNameAndArgs (const ArgumentList& args, const ConsoleApplication::Command& command)
{
    auto exeName = args.executableName.fromLastOccurrenceOf ("/", false, false)
                       .fromLastOccurrenceOf ("\\", false, false);

    return " " + exeName + " " + command.argumentDescription;
}

static void printCommandDescription (const ArgumentList& args, const ConsoleApplication::Command& command, int descriptionIndent)
{
    auto nameAndArgs = getExeNameAndArgs (args, command);

    if (nameAndArgs.length() > descriptionIndent)
        std::cout << nameAndArgs << std::endl
                  << String().paddedRight (' ', descriptionIndent);
    else
        std::cout << nameAndArgs.paddedRight (' ', descriptionIndent);

    std::cout << command.shortDescription << std::endl;
}

void ConsoleApplication::printCommandList (const ArgumentList& args) const
{
    int descriptionIndent = 0;

    for (auto& c : commands)
        descriptionIndent = std::max (descriptionIndent, getExeNameAndArgs (args, c).length());

    descriptionIndent = std::min (descriptionIndent + 2, 40);

    for (auto& c : commands)
        printCommandDescription (args, c, descriptionIndent);

    std::cout << std::endl;
}

void ConsoleApplication::printCommandDetails (const ArgumentList& args, const Command& command) const
{
    auto len = getExeNameAndArgs (args, command).length();

    printCommandDescription (args, command, std::min (len + 3, 40));

    if (command.longDescription.isNotEmpty())
        std::cout << std::endl
                  << command.longDescription << std::endl;
}

} // namespace yup
