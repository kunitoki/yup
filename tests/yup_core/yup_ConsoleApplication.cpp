/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include <yup_core/yup_core.h>

using namespace yup;

//==============================================================================
// ArgumentList::Argument Tests
//==============================================================================

TEST (ArgumentTests, IsLongOption)
{
    ArgumentList::Argument arg1 { "--help" };
    ArgumentList::Argument arg2 { "-h" };
    ArgumentList::Argument arg3 { "file.txt" };
    ArgumentList::Argument arg4 { "--" };

    EXPECT_TRUE (arg1.isLongOption());
    EXPECT_FALSE (arg2.isLongOption());
    EXPECT_FALSE (arg3.isLongOption());
    EXPECT_TRUE (arg4.isLongOption());
}

TEST (ArgumentTests, IsShortOption)
{
    ArgumentList::Argument arg1 { "-h" };
    ArgumentList::Argument arg2 { "--help" };
    ArgumentList::Argument arg3 { "file.txt" };
    ArgumentList::Argument arg4 { "-" };

    EXPECT_TRUE (arg1.isShortOption());
    EXPECT_FALSE (arg2.isShortOption());
    EXPECT_FALSE (arg3.isShortOption());
    EXPECT_TRUE (arg4.isShortOption());
}

TEST (ArgumentTests, IsLongOptionWithRoot)
{
    ArgumentList::Argument arg1 { "--help" };
    ArgumentList::Argument arg2 { "--help=text" };
    ArgumentList::Argument arg3 { "--version" };

    EXPECT_TRUE (arg1.isLongOption ("help"));
    EXPECT_TRUE (arg2.isLongOption ("help"));
    EXPECT_FALSE (arg1.isLongOption ("version"));
    EXPECT_TRUE (arg3.isLongOption ("version"));
}

TEST (ArgumentTests, GetLongOptionValue)
{
    ArgumentList::Argument arg1 { "--file=test.txt" };
    ArgumentList::Argument arg2 { "--output=result.dat" };
    ArgumentList::Argument arg3 { "--flag" };

    EXPECT_EQ (arg1.getLongOptionValue(), "test.txt");
    EXPECT_EQ (arg2.getLongOptionValue(), "result.dat");
    EXPECT_FALSE (arg3.getLongOptionValue().has_value());
    EXPECT_EQ (ArgumentList::Argument { "--empty=" }.getLongOptionValue(), "");
    EXPECT_FALSE (ArgumentList::Argument { "-f=x" }.getLongOptionValue().has_value());
}

TEST (ArgumentTests, IsShortOptionWithChar)
{
    ArgumentList list ("test", "-h -abc --help");

    EXPECT_TRUE (list[0].isShortOption ('h'));
    EXPECT_FALSE (list[0].isShortOption ('x'));
    EXPECT_TRUE (list[1].isShortOption ('a'));
    EXPECT_TRUE (list[1].isShortOption ('b'));
    EXPECT_TRUE (list[1].isShortOption ('c'));
    EXPECT_FALSE (list[1].isShortOption ('d'));
    EXPECT_FALSE (list[2].isShortOption ('h'));
}

TEST (ArgumentTests, IsOption)
{
    ArgumentList::Argument arg1 { "--help" };
    ArgumentList::Argument arg2 { "-h" };
    ArgumentList::Argument arg3 { "file.txt" };
    ArgumentList::Argument arg4 { "-" };
    ArgumentList::Argument arg5 { "--" };

    EXPECT_TRUE (arg1.isOption());
    EXPECT_TRUE (arg2.isOption());
    EXPECT_FALSE (arg3.isOption());
    EXPECT_TRUE (arg4.isOption());
    EXPECT_TRUE (arg5.isOption());
}

TEST (ArgumentTests, NegativeNumbersAreNotOptions)
{
    for (auto text : { "-6", "-0.5", "-.5", "-1e3" })
    {
        ArgumentList::Argument arg { text };

        EXPECT_FALSE (arg.isOption()) << text;
        EXPECT_FALSE (arg.isShortOption()) << text;
    }

    EXPECT_TRUE (ArgumentList::Argument { "-e" }.isOption());
    EXPECT_TRUE (ArgumentList::Argument { "-6x" }.isOption());
}

TEST (ArgumentTests, EqualityOperator)
{
    ArgumentList::Argument arg { "--help" };

    EXPECT_TRUE (arg == "--help");
    EXPECT_FALSE (arg == "--version");

    // Test pipe-separated list
    EXPECT_TRUE (arg == "--help|--h|-h");
    EXPECT_TRUE (arg == "-h|--help|--h");
    EXPECT_FALSE (arg == "-v|--version");
}

TEST (ArgumentTests, InequalityOperator)
{
    ArgumentList::Argument arg { "--help" };

    EXPECT_FALSE (arg != "--help");
    EXPECT_TRUE (arg != "--version");

    EXPECT_FALSE (arg != "--help|--h|-h");
    EXPECT_TRUE (arg != "-v|--version");
}

TEST (ArgumentTests, ResolveAsFile)
{
    ArgumentList::Argument arg { "test.txt" };
    auto file = arg.resolveAsFile();

    EXPECT_TRUE (file.getFileName() == "test.txt");
}

//==============================================================================
// ArgumentList Tests
//==============================================================================

TEST (ArgumentListTests, ConstructFromExecutableAndArray)
{
    StringArray args;
    args.add ("--help");
    args.add ("file.txt");

    ArgumentList list ("myapp", args);

    EXPECT_EQ (list.executableName, "myapp");
    EXPECT_EQ (list.size(), 2);
    EXPECT_EQ (list[0].text, "--help");
    EXPECT_EQ (list[1].text, "file.txt");
}

TEST (ArgumentListTests, ConstructFromArgcArgv)
{
    const char* argv[] = { "myapp", "--help", "file.txt" };
    int argc = 3;

    ArgumentList list (argc, const_cast<char**> (argv));

    EXPECT_EQ (list.executableName, "myapp");
    EXPECT_EQ (list.size(), 2);
    EXPECT_EQ (list[0].text, "--help");
    EXPECT_EQ (list[1].text, "file.txt");
}

TEST (ArgumentListTests, ConstructFromString)
{
    ArgumentList list ("myapp", "--help --verbose file.txt");

    EXPECT_EQ (list.executableName, "myapp");
    EXPECT_EQ (list.size(), 3);
    EXPECT_EQ (list[0].text, "--help");
    EXPECT_EQ (list[1].text, "--verbose");
    EXPECT_EQ (list[2].text, "file.txt");
}

TEST (ArgumentListTests, ConstructFromStringWithQuotes)
{
    ArgumentList list ("myapp", "--file \"my file.txt\" --output result.dat");

    EXPECT_EQ (list.size(), 4);
    EXPECT_EQ (list[0].text, "--file");
    EXPECT_EQ (list[1].text, "my file.txt");
    EXPECT_EQ (list[2].text, "--output");
    EXPECT_EQ (list[3].text, "result.dat");
}

TEST (ArgumentListTests, Size)
{
    ArgumentList list1 ("myapp", "--help");
    ArgumentList list2 ("myapp", "");

    EXPECT_EQ (list1.size(), 1);
    EXPECT_EQ (list2.size(), 0);
}

TEST (ArgumentListTests, IndexOperator)
{
    ArgumentList list ("myapp", "--help --verbose");

    EXPECT_EQ (list[0].text, "--help");
    EXPECT_EQ (list[1].text, "--verbose");
}

TEST (ArgumentListTests, ContainsOption)
{
    ArgumentList list ("myapp", "--help --verbose file.txt");

    EXPECT_TRUE (list.containsOption ("--help"));
    EXPECT_TRUE (list.containsOption ("--verbose"));
    EXPECT_FALSE (list.containsOption ("--version"));

    // Test pipe-separated list
    EXPECT_TRUE (list.containsOption ("--help|-h"));
    EXPECT_FALSE (list.containsOption ("--version|-v"));
}

TEST (ArgumentListTests, RemoveOptionIfFound)
{
    ArgumentList list ("myapp", "--help --verbose file.txt");

    EXPECT_TRUE (list.removeOptionIfFound ("--help"));
    EXPECT_EQ (list.size(), 2);
    EXPECT_FALSE (list.containsOption ("--help"));

    EXPECT_FALSE (list.removeOptionIfFound ("--version"));
    EXPECT_EQ (list.size(), 2);

    EXPECT_TRUE (list.removeOptionIfFound ("--verbose"));
    EXPECT_EQ (list.size(), 1);
}

TEST (ArgumentListTests, IndexOfOption)
{
    ArgumentList list ("myapp", "--help --verbose file.txt");

    EXPECT_EQ (list.indexOfOption ("--help"), 0);
    EXPECT_EQ (list.indexOfOption ("--verbose"), 1);
    EXPECT_EQ (list.indexOfOption ("--version"), -1);
    EXPECT_EQ (list.indexOfOption ("file.txt"), 2);
}

TEST (ArgumentListTests, GetValueForOption)
{
    ArgumentList list ("myapp", "--file=test.txt --output=result.dat -v");

    EXPECT_EQ (list.getValueForOption ("--file"), "test.txt");
    EXPECT_EQ (list.getValueForOption ("--output"), "result.dat");
    EXPECT_FALSE (list.getValueForOption ("-v").has_value());
    EXPECT_FALSE (list.getValueForOption ("--missing").has_value());
}

TEST (ArgumentListTests, GetValueForShortOption)
{
    ArgumentList list ("myapp", "-f input.txt -o output.dat");

    EXPECT_EQ (list.getValueForOption ("-f"), "input.txt");
    EXPECT_EQ (list.getValueForOption ("-o"), "output.dat");
}

TEST (ArgumentListTests, RemoveValueForOption)
{
    ArgumentList list ("myapp", "--file=test.txt --output=result.dat");

    EXPECT_EQ (list.removeValueForOption ("--file"), "test.txt");
    EXPECT_FALSE (list.containsOption ("--file"));

    EXPECT_EQ (list.removeValueForOption ("--output"), "result.dat");
    EXPECT_EQ (list.size(), 0);
}

TEST (ArgumentListTests, GetFileForOption)
{
    ArgumentList list ("myapp", "--input=/tmp/test.txt");

    auto file = list.getFileForOption ("--input");
    EXPECT_TRUE (file.getFullPathName().contains ("test.txt"));
}

TEST (ArgumentListTests, GetFileForOptionAndRemove)
{
    ArgumentList list ("myapp", "--input=/tmp/test.txt --output=result.dat");

    auto file = list.getFileForOptionAndRemove ("--input");
    EXPECT_TRUE (file.getFullPathName().contains ("test.txt"));
    EXPECT_FALSE (list.containsOption ("--input"));
    EXPECT_EQ (list.size(), 1);
}

TEST (ArgumentListTests, GetValueForOptionWithSpace)
{
    ArgumentList list ("myapp", "--file test.txt --level 3 positional");

    EXPECT_EQ (list.getValueForOption ("--file"), "test.txt");
    EXPECT_EQ (list.getValueForOption ("--level"), "3");
}

TEST (ArgumentListTests, GetValueForOptionWithoutValue)
{
    ArgumentList list ("myapp", "--flag --other x --empty=");

    EXPECT_TRUE (list.containsOption ("--flag"));
    EXPECT_FALSE (list.getValueForOption ("--flag").has_value());
    EXPECT_EQ (list.getValueForOption ("--empty"), "");
}

TEST (ArgumentListTests, GetValuesForOption)
{
    ArgumentList list ("myapp", "--files a b c --verbose");

    EXPECT_EQ (list.getValuesForOption ("--files"), StringArray ({ "a", "b", "c" }));
    EXPECT_EQ (list.getValueForOption ("--files"), "a");
    EXPECT_TRUE (list.getValuesForOption ("--verbose").isEmpty());
    EXPECT_TRUE (list.getValuesForOption ("--missing").isEmpty());
}

TEST (ArgumentListTests, GetValuesForRepeatedOption)
{
    ArgumentList list ("myapp", "--inc a --inc=b -x --inc c d");

    EXPECT_EQ (list.getValuesForOption ("--inc"), StringArray ({ "a", "b", "c", "d" }));
    EXPECT_EQ (list.getValueForOption ("--inc"), "a");
}

TEST (ArgumentListTests, GetValuesForOptionWithAlternatives)
{
    ArgumentList list ("myapp", "-i a --include b");

    EXPECT_EQ (list.getValuesForOption ("--include|-i"), StringArray ({ "a", "b" }));
}

TEST (ArgumentListTests, NegativeNumbersAreValues)
{
    ArgumentList list ("myapp", "--gain -6 --offset -0.5 --range -1 1");

    EXPECT_EQ (list.getIntValueForOption ("--gain", 0), -6);
    EXPECT_DOUBLE_EQ (list.getDoubleValueForOption ("--offset", 0.0), -0.5);
    EXPECT_EQ (list.getValuesForOption ("--range"), StringArray ({ "-1", "1" }));
}

TEST (ArgumentListTests, TerminatorEndsOptions)
{
    ArgumentList list ("myapp", "--files a b -- c --files d -x");

    EXPECT_EQ (list.getValuesForOption ("--files"), StringArray ({ "a", "b" }));
    EXPECT_FALSE (list.containsOption ("-x"));
    EXPECT_EQ (list.getTrailingArguments(), StringArray ({ "c", "--files", "d", "-x" }));
}

TEST (ArgumentListTests, NoTrailingArgumentsWithoutTerminator)
{
    ArgumentList list ("myapp", "--file a b");

    EXPECT_TRUE (list.getTrailingArguments().isEmpty());
}

TEST (ArgumentListTests, RemoveValueForOptionKeepsFurtherValues)
{
    ArgumentList list ("myapp", "--file a b");

    EXPECT_EQ (list.removeValueForOption ("--file"), "a");
    ASSERT_EQ (list.size(), 1);
    EXPECT_EQ (list[0].text, "b");
}

TEST (ArgumentListTests, RemoveValueForMissingOption)
{
    ArgumentList list ("myapp", "--flag");

    EXPECT_FALSE (list.removeValueForOption ("--missing").has_value());
    EXPECT_FALSE (list.removeValueForOption ("--flag").has_value());
    EXPECT_EQ (list.size(), 0);
}

TEST (ArgumentListTests, RemoveValuesForOption)
{
    ArgumentList list ("myapp", "--inc a b --verbose --inc=c --inc d");

    EXPECT_EQ (list.removeValuesForOption ("--inc"), StringArray ({ "a", "b", "c", "d" }));
    ASSERT_EQ (list.size(), 1);
    EXPECT_TRUE (list.containsOption ("--verbose"));
}

TEST (ArgumentListTests, GetIntValueForOption)
{
    ArgumentList list ("myapp", "--count 42 --plus=+7 --bad abc --fraction 1.5 --flag");

    EXPECT_EQ (list.getIntValueForOption ("--count", 0), 42);
    EXPECT_EQ (list.getIntValueForOption ("--plus", 0), 7);
    EXPECT_EQ (list.getIntValueForOption ("--bad", -1), -1);
    EXPECT_EQ (list.getIntValueForOption ("--fraction", -1), -1);
    EXPECT_EQ (list.getIntValueForOption ("--flag", -1), -1);
    EXPECT_EQ (list.getIntValueForOption ("--missing", 5), 5);
}

TEST (ArgumentListTests, GetDoubleValueForOption)
{
    ArgumentList list ("myapp", "--rate 0.25 --big=1e3 --integer 2 --bad x1");

    EXPECT_DOUBLE_EQ (list.getDoubleValueForOption ("--rate", 0.0), 0.25);
    EXPECT_DOUBLE_EQ (list.getDoubleValueForOption ("--big", 0.0), 1000.0);
    EXPECT_DOUBLE_EQ (list.getDoubleValueForOption ("--integer", 0.0), 2.0);
    EXPECT_DOUBLE_EQ (list.getDoubleValueForOption ("--bad", -1.0), -1.0);
    EXPECT_DOUBLE_EQ (list.getDoubleValueForOption ("--missing", 3.0), 3.0);
}

TEST (ArgumentListTests, GetBoolValueForOption)
{
    ArgumentList list ("myapp", "--verbose --color=off --fast NO --zero 0 --on=yes");

    EXPECT_TRUE (list.getBoolValueForOption ("--verbose", false));
    EXPECT_FALSE (list.getBoolValueForOption ("--color", true));
    EXPECT_FALSE (list.getBoolValueForOption ("--fast", true));
    EXPECT_FALSE (list.getBoolValueForOption ("--zero", true));
    EXPECT_TRUE (list.getBoolValueForOption ("--on", false));
    EXPECT_TRUE (list.getBoolValueForOption ("--missing", true));
    EXPECT_FALSE (list.getBoolValueForOption ("--missing", false));
}

TEST (ArgumentListTests, TryGetFileForOption)
{
    ArgumentList list ("myapp", "--input /tmp/test.txt --flag");

    auto file = list.tryGetFileForOption ("--input");
    ASSERT_TRUE (file.wasOk());
    EXPECT_EQ (file.getReference().getFileName(), "test.txt");

    EXPECT_TRUE (list.tryGetFileForOption ("--flag").failed());
    EXPECT_TRUE (list.tryGetFileForOption ("--missing").failed());
}

TEST (ArgumentListTests, TryGetExistingFileAndFolderForOption)
{
    const auto folder = File::getSpecialLocation (File::tempDirectory);
    const auto missing = folder.getNonexistentChildFile ("yup_argument_list_test", ".txt");

    ArgumentList list ("myapp", StringArray { "--dir", folder.getFullPathName(), "--file", missing.getFullPathName() });

    auto existingFolder = list.tryGetExistingFolderForOption ("--dir");
    ASSERT_TRUE (existingFolder.wasOk());
    EXPECT_EQ (existingFolder.getReference(), folder);

    EXPECT_TRUE (list.tryGetExistingFileForOption ("--file").failed());
    EXPECT_TRUE (list.tryGetExistingFolderForOption ("--file").failed());
    EXPECT_TRUE (list.tryGetExistingFileForOption ("--missing").failed());
}

TEST (ArgumentListTests, TryResolveAsExistingFile)
{
    const auto folder = File::getSpecialLocation (File::tempDirectory);
    const auto missing = folder.getNonexistentChildFile ("yup_argument_list_test", ".txt");

    EXPECT_TRUE (ArgumentList::Argument { folder.getFullPathName() }.tryResolveAsExistingFolder().wasOk());
    EXPECT_TRUE (ArgumentList::Argument { missing.getFullPathName() }.tryResolveAsExistingFile().failed());
}

TEST (ArgumentListTests, RangeForIteratesArguments)
{
    ArgumentList list ("myapp", "a b c");

    StringArray texts;
    for (const auto& arg : list)
        texts.add (arg.text);

    EXPECT_EQ (texts, StringArray ({ "a", "b", "c" }));
}

//==============================================================================
// ConsoleApplication::Command Tests
//==============================================================================

TEST (ConsoleApplicationCommandTests, BasicCommand)
{
    ConsoleApplication::Command cmd;
    cmd.commandOption = "--test";
    cmd.argumentDescription = "--test <file>";
    cmd.shortDescription = "Test command";
    cmd.longDescription = "This is a longer description of the test command.";

    bool executed = false;
    cmd.command = [&executed] (const ArgumentList&)
    {
        executed = true;
    };

    EXPECT_EQ (cmd.commandOption, "--test");
    EXPECT_EQ (cmd.argumentDescription, "--test <file>");
    EXPECT_EQ (cmd.shortDescription, "Test command");
    EXPECT_EQ (cmd.longDescription, "This is a longer description of the test command.");

    ArgumentList list ("myapp", "--test file.txt");
    cmd.command (list);
    EXPECT_TRUE (executed);
}

//==============================================================================
// ConsoleApplication Tests
//==============================================================================

TEST (ConsoleApplicationTests, AddCommand)
{
    ConsoleApplication app;

    app.addCommand ({ "--test", "--test <file>", "Test command", "", [] (const auto&) {} });

    EXPECT_EQ (app.getCommands().size(), 1);
    EXPECT_EQ (app.getCommands()[0].commandOption, "--test");
}

TEST (ConsoleApplicationTests, AddMultipleCommands)
{
    ConsoleApplication app;

    app.addCommand ({ "--foo", "--foo", "Foo command", "", [] (const auto&) {} });
    app.addCommand ({ "--bar", "--bar", "Bar command", "", [] (const auto&) {} });
    app.addCommand ({ "--baz", "--baz", "Baz command", "", [] (const auto&) {} });

    EXPECT_EQ (app.getCommands().size(), 3);
}

TEST (ConsoleApplicationTests, AddDefaultCommand)
{
    ConsoleApplication app;

    app.addDefaultCommand ({ "", "", "Default command", "", [] (const auto&) {} });

    EXPECT_EQ (app.getCommands().size(), 1);
}

TEST (ConsoleApplicationTests, AddVersionCommand)
{
    ConsoleApplication app;

    app.addVersionCommand ("--version|-v", "MyApp v1.0.0");

    EXPECT_EQ (app.getCommands().size(), 1);
    EXPECT_EQ (app.getCommands()[0].commandOption, "--version|-v");
}

TEST (ConsoleApplicationTests, AddHelpCommand)
{
    ConsoleApplication app;

    app.addHelpCommand ("--help|-h", "Usage: myapp [options]", false);

    EXPECT_EQ (app.getCommands().size(), 1);
    EXPECT_EQ (app.getCommands()[0].commandOption, "--help|-h");
}

TEST (ConsoleApplicationTests, FindCommand)
{
    ConsoleApplication app;

    app.addCommand ({ "--foo", "--foo", "Foo", "", [] (const auto&) {} });
    app.addCommand ({ "--bar", "--bar", "Bar", "", [] (const auto&) {} });

    ArgumentList list1 ("myapp", "--foo");
    ArgumentList list2 ("myapp", "--bar");
    ArgumentList list3 ("myapp", "--baz");

    auto* cmd1 = app.findCommand (list1, false);
    auto* cmd2 = app.findCommand (list2, false);
    auto* cmd3 = app.findCommand (list3, false);

    EXPECT_NE (cmd1, nullptr);
    EXPECT_EQ (cmd1->commandOption, "--foo");

    EXPECT_NE (cmd2, nullptr);
    EXPECT_EQ (cmd2->commandOption, "--bar");

    EXPECT_EQ (cmd3, nullptr);
}

TEST (ConsoleApplicationTests, FindCommandWithDefault)
{
    ConsoleApplication app;

    app.addCommand ({ "--foo", "--foo", "Foo", "", [] (const auto&) {} });
    app.addDefaultCommand ({ "", "", "Default", "", [] (const auto&) {} });

    ArgumentList list1 ("myapp", "--foo");
    ArgumentList list2 ("myapp", "--unknown");

    auto* cmd1 = app.findCommand (list1, false);
    auto* cmd2 = app.findCommand (list2, false);

    EXPECT_NE (cmd1, nullptr);
    EXPECT_EQ (cmd1->commandOption, "--foo");

    EXPECT_NE (cmd2, nullptr);
    EXPECT_TRUE (cmd2->commandOption.isEmpty());
}

TEST (ConsoleApplicationTests, FindCommandOptionMustBeFirst)
{
    ConsoleApplication app;

    app.addCommand ({ "build", "build", "Build", "", [] (const auto&) {} });
    app.addCommand ({ "test", "test", "Test", "", [] (const auto&) {} });

    ArgumentList list1 ("myapp", "build --verbose");
    ArgumentList list2 ("myapp", "--verbose build");

    auto* cmd1 = app.findCommand (list1, true);
    auto* cmd2 = app.findCommand (list2, true);

    EXPECT_NE (cmd1, nullptr);
    EXPECT_EQ (cmd1->commandOption, "build");

    EXPECT_EQ (cmd2, nullptr);
}

TEST (ConsoleApplicationTests, FindAndRunCommand)
{
    ConsoleApplication app;

    bool fooExecuted = false;
    app.addCommand ({ "--foo", "--foo", "Foo", "", [&fooExecuted] (const auto&)
    {
        fooExecuted = true;
    } });

    ArgumentList list ("myapp", "--foo");

    int result = app.findAndRunCommand (list, false);

    EXPECT_EQ (result, 0);
    EXPECT_TRUE (fooExecuted);
}

TEST (ConsoleApplicationTests, FindAndRunCommandWithReturnCode)
{
    ConsoleApplication app;

    app.addCommand ({ "--fail", "--fail", "Fail", "", [] (const auto&)
    {
        ConsoleApplication::fail ("Error occurred", 42);
    } });

    ArgumentList list ("myapp", "--fail");

    int result = app.findAndRunCommand (list, false);

    EXPECT_EQ (result, 42);
}

TEST (ConsoleApplicationTests, InvokeCatchingFailures)
{
    int result = ConsoleApplication::invokeCatchingFailures ([]
    {
        return 123;
    });

    EXPECT_EQ (result, 123);
}

TEST (ConsoleApplicationTests, InvokeCatchingFailuresWithFail)
{
    int result = ConsoleApplication::invokeCatchingFailures ([]
    {
        ConsoleApplication::fail ("Test error", 99);
        return 0;
    });

    EXPECT_EQ (result, 99);
}

TEST (ConsoleApplicationTests, PipeSeparatedCommandOptions)
{
    ConsoleApplication app;

    bool executed = false;
    app.addCommand ({ "--help|-h|--usage", "--help", "Help", "", [&executed] (const auto&)
    {
        executed = true;
    } });

    ArgumentList list1 ("myapp", "--help");
    ArgumentList list2 ("myapp", "-h");
    ArgumentList list3 ("myapp", "--usage");

    app.findAndRunCommand (list1, false);
    EXPECT_TRUE (executed);

    executed = false;
    app.findAndRunCommand (list2, false);
    EXPECT_TRUE (executed);

    executed = false;
    app.findAndRunCommand (list3, false);
    EXPECT_TRUE (executed);
}

TEST (ConsoleApplicationTests, CommandReceivesCorrectArgumentList)
{
    ConsoleApplication app;

    String receivedArg;
    app.addCommand ({ "--process", "--process <file>", "Process", "", [&receivedArg] (const auto& args)
    {
        if (args.size() > 1)
            receivedArg = args[1].text;
    } });

    ArgumentList list ("myapp", "--process test.txt");

    app.findAndRunCommand (list, false);

    EXPECT_EQ (receivedArg, "test.txt");
}

TEST (ConsoleApplicationTests, MultipleCommandsWithDifferentOptions)
{
    ConsoleApplication app;

    int executedCommand = 0;
    app.addCommand ({ "--cmd1", "--cmd1", "Command 1", "", [&executedCommand] (const auto&)
    {
        executedCommand = 1;
    } });
    app.addCommand ({ "--cmd2", "--cmd2", "Command 2", "", [&executedCommand] (const auto&)
    {
        executedCommand = 2;
    } });
    app.addCommand ({ "--cmd3", "--cmd3", "Command 3", "", [&executedCommand] (const auto&)
    {
        executedCommand = 3;
    } });

    ArgumentList list1 ("myapp", "--cmd1");
    ArgumentList list2 ("myapp", "--cmd2");
    ArgumentList list3 ("myapp", "--cmd3");

    app.findAndRunCommand (list1, false);
    EXPECT_EQ (executedCommand, 1);

    app.findAndRunCommand (list2, false);
    EXPECT_EQ (executedCommand, 2);

    app.findAndRunCommand (list3, false);
    EXPECT_EQ (executedCommand, 3);
}

TEST (ConsoleApplicationTests, EmptyArgumentListWithDefaultCommand)
{
    ConsoleApplication app;

    bool defaultExecuted = false;
    app.addDefaultCommand ({ "", "", "Default", "", [&defaultExecuted] (const auto&)
    {
        defaultExecuted = true;
    } });

    ArgumentList list ("myapp", "");

    app.findAndRunCommand (list, false);

    EXPECT_TRUE (defaultExecuted);
}

TEST (ConsoleApplicationTests, GetCommands)
{
    ConsoleApplication app;

    EXPECT_EQ (app.getCommands().size(), 0);

    app.addCommand ({ "--foo", "--foo", "Foo", "", [] (const auto&) {} });
    app.addCommand ({ "--bar", "--bar", "Bar", "", [] (const auto&) {} });

    const auto& commands = app.getCommands();
    EXPECT_EQ (commands.size(), 2);
    EXPECT_EQ (commands[0].commandOption, "--foo");
    EXPECT_EQ (commands[1].commandOption, "--bar");
}
