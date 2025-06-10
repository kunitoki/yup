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

#pragma once

#include "juce_TargetPlatform.h"

/** Return the Nth argument. By passing a variadic pack followed by N other
    parameters, we can select one of those N parameter based on the length of
    the parameter pack.
*/
#define YUP_NTH_ARG_(_00, _01, _02, _03, _04, _05, _06, _07, _08, _09, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, N, ...) \
    N

#define YUP_EACH_00_(FN)
#define YUP_EACH_01_(FN, X) FN (X)
#define YUP_EACH_02_(FN, X, ...) FN (X) YUP_EACH_01_ (FN, __VA_ARGS__)
#define YUP_EACH_03_(FN, X, ...) FN (X) YUP_EACH_02_ (FN, __VA_ARGS__)
#define YUP_EACH_04_(FN, X, ...) FN (X) YUP_EACH_03_ (FN, __VA_ARGS__)
#define YUP_EACH_05_(FN, X, ...) FN (X) YUP_EACH_04_ (FN, __VA_ARGS__)
#define YUP_EACH_06_(FN, X, ...) FN (X) YUP_EACH_05_ (FN, __VA_ARGS__)
#define YUP_EACH_07_(FN, X, ...) FN (X) YUP_EACH_06_ (FN, __VA_ARGS__)
#define YUP_EACH_08_(FN, X, ...) FN (X) YUP_EACH_07_ (FN, __VA_ARGS__)
#define YUP_EACH_09_(FN, X, ...) FN (X) YUP_EACH_08_ (FN, __VA_ARGS__)
#define YUP_EACH_10_(FN, X, ...) FN (X) YUP_EACH_09_ (FN, __VA_ARGS__)
#define YUP_EACH_11_(FN, X, ...) FN (X) YUP_EACH_10_ (FN, __VA_ARGS__)
#define YUP_EACH_12_(FN, X, ...) FN (X) YUP_EACH_11_ (FN, __VA_ARGS__)
#define YUP_EACH_13_(FN, X, ...) FN (X) YUP_EACH_12_ (FN, __VA_ARGS__)
#define YUP_EACH_14_(FN, X, ...) FN (X) YUP_EACH_13_ (FN, __VA_ARGS__)
#define YUP_EACH_15_(FN, X, ...) FN (X) YUP_EACH_14_ (FN, __VA_ARGS__)
#define YUP_EACH_16_(FN, X, ...) FN (X) YUP_EACH_15_ (FN, __VA_ARGS__)
#define YUP_EACH_17_(FN, X, ...) FN (X) YUP_EACH_16_ (FN, __VA_ARGS__)
#define YUP_EACH_18_(FN, X, ...) FN (X) YUP_EACH_17_ (FN, __VA_ARGS__)
#define YUP_EACH_19_(FN, X, ...) FN (X) YUP_EACH_18_ (FN, __VA_ARGS__)
#define YUP_EACH_20_(FN, X, ...) FN (X) YUP_EACH_19_ (FN, __VA_ARGS__)
#define YUP_EACH_21_(FN, X, ...) FN (X) YUP_EACH_20_ (FN, __VA_ARGS__)
#define YUP_EACH_22_(FN, X, ...) FN (X) YUP_EACH_21_ (FN, __VA_ARGS__)
#define YUP_EACH_23_(FN, X, ...) FN (X) YUP_EACH_22_ (FN, __VA_ARGS__)
#define YUP_EACH_24_(FN, X, ...) FN (X) YUP_EACH_23_ (FN, __VA_ARGS__)
#define YUP_EACH_25_(FN, X, ...) FN (X) YUP_EACH_24_ (FN, __VA_ARGS__)
#define YUP_EACH_26_(FN, X, ...) FN (X) YUP_EACH_25_ (FN, __VA_ARGS__)
#define YUP_EACH_27_(FN, X, ...) FN (X) YUP_EACH_26_ (FN, __VA_ARGS__)
#define YUP_EACH_28_(FN, X, ...) FN (X) YUP_EACH_27_ (FN, __VA_ARGS__)
#define YUP_EACH_29_(FN, X, ...) FN (X) YUP_EACH_28_ (FN, __VA_ARGS__)
#define YUP_EACH_30_(FN, X, ...) FN (X) YUP_EACH_29_ (FN, __VA_ARGS__)
#define YUP_EACH_31_(FN, X, ...) FN (X) YUP_EACH_30_ (FN, __VA_ARGS__)
#define YUP_EACH_32_(FN, X, ...) FN (X) YUP_EACH_31_ (FN, __VA_ARGS__)
#define YUP_EACH_33_(FN, X, ...) FN (X) YUP_EACH_32_ (FN, __VA_ARGS__)
#define YUP_EACH_34_(FN, X, ...) FN (X) YUP_EACH_33_ (FN, __VA_ARGS__)
#define YUP_EACH_35_(FN, X, ...) FN (X) YUP_EACH_34_ (FN, __VA_ARGS__)
#define YUP_EACH_36_(FN, X, ...) FN (X) YUP_EACH_35_ (FN, __VA_ARGS__)
#define YUP_EACH_37_(FN, X, ...) FN (X) YUP_EACH_36_ (FN, __VA_ARGS__)
#define YUP_EACH_38_(FN, X, ...) FN (X) YUP_EACH_37_ (FN, __VA_ARGS__)
#define YUP_EACH_39_(FN, X, ...) FN (X) YUP_EACH_38_ (FN, __VA_ARGS__)
#define YUP_EACH_40_(FN, X, ...) FN (X) YUP_EACH_39_ (FN, __VA_ARGS__)
#define YUP_EACH_41_(FN, X, ...) FN (X) YUP_EACH_40_ (FN, __VA_ARGS__)
#define YUP_EACH_42_(FN, X, ...) FN (X) YUP_EACH_41_ (FN, __VA_ARGS__)
#define YUP_EACH_43_(FN, X, ...) FN (X) YUP_EACH_42_ (FN, __VA_ARGS__)
#define YUP_EACH_44_(FN, X, ...) FN (X) YUP_EACH_43_ (FN, __VA_ARGS__)
#define YUP_EACH_45_(FN, X, ...) FN (X) YUP_EACH_44_ (FN, __VA_ARGS__)
#define YUP_EACH_46_(FN, X, ...) FN (X) YUP_EACH_45_ (FN, __VA_ARGS__)
#define YUP_EACH_47_(FN, X, ...) FN (X) YUP_EACH_46_ (FN, __VA_ARGS__)
#define YUP_EACH_48_(FN, X, ...) FN (X) YUP_EACH_47_ (FN, __VA_ARGS__)
#define YUP_EACH_49_(FN, X, ...) FN (X) YUP_EACH_48_ (FN, __VA_ARGS__)

/** Apply the macro FN to each of the other arguments. */
#define YUP_EACH(FN, ...)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
    YUP_NTH_ARG_ (, __VA_ARGS__, YUP_EACH_49_, YUP_EACH_48_, YUP_EACH_47_, YUP_EACH_46_, YUP_EACH_45_, YUP_EACH_44_, YUP_EACH_43_, YUP_EACH_42_, YUP_EACH_41_, YUP_EACH_40_, YUP_EACH_39_, YUP_EACH_38_, YUP_EACH_37_, YUP_EACH_36_, YUP_EACH_35_, YUP_EACH_34_, YUP_EACH_33_, YUP_EACH_32_, YUP_EACH_31_, YUP_EACH_30_, YUP_EACH_29_, YUP_EACH_28_, YUP_EACH_27_, YUP_EACH_26_, YUP_EACH_25_, YUP_EACH_24_, YUP_EACH_23_, YUP_EACH_22_, YUP_EACH_21_, YUP_EACH_20_, YUP_EACH_19_, YUP_EACH_18_, YUP_EACH_17_, YUP_EACH_16_, YUP_EACH_15_, YUP_EACH_14_, YUP_EACH_13_, YUP_EACH_12_, YUP_EACH_11_, YUP_EACH_10_, YUP_EACH_09_, YUP_EACH_08_, YUP_EACH_07_, YUP_EACH_06_, YUP_EACH_05_, YUP_EACH_04_, YUP_EACH_03_, YUP_EACH_02_, YUP_EACH_01_, YUP_EACH_00_) \
    (FN, __VA_ARGS__)

/** Concatenate two tokens to form a new token. */
#define YUP_CONCAT_(a, b) a##b
#define YUP_CONCAT(a, b) YUP_CONCAT_ (a, b)

/** Quote the argument, turning it into a string. */
#define YUP_TO_STRING(x) #x

#if YUP_CLANG || YUP_GCC || YUP_MINGW
#define YUP_IGNORE_GCC_IMPL_(compiler, warning)
#define YUP_IGNORE_GCC_IMPL_0(compiler, warning)
#define YUP_IGNORE_GCC_IMPL_1(compiler, warning) \
    _Pragma (YUP_TO_STRING (compiler diagnostic ignored warning))

/** If 'warning' is recognised by this compiler, ignore it. */
#if defined(__has_warning)
#define YUP_IGNORE_GCC_LIKE(compiler, warning)                  \
    YUP_CONCAT (YUP_IGNORE_GCC_IMPL_, __has_warning (warning)) \
    (compiler, warning)
#else
#define YUP_IGNORE_GCC_LIKE(compiler, warning) \
    YUP_IGNORE_GCC_IMPL_1 (compiler, warning)
#endif

/** Ignore GCC/clang-specific warnings. */
#define YUP_IGNORE_GCC(warning) YUP_IGNORE_GCC_LIKE (GCC, warning)
#define YUP_IGNORE_clang(warning) YUP_IGNORE_GCC_LIKE (clang, warning)

#define YUP_IGNORE_WARNINGS_GCC_LIKE(compiler, ...)    \
    _Pragma (YUP_TO_STRING (compiler diagnostic push)) \
        YUP_EACH (YUP_CONCAT (YUP_IGNORE_, compiler), __VA_ARGS__)

/** Push a new warning scope, and then ignore each warning for either clang
        or gcc. If the compiler doesn't support __has_warning, we add -Wpragmas
        as the first disabled warning because otherwise we might get complaints
        about unknown warning options.
    */
#if defined(__has_warning)
#define YUP_PUSH_WARNINGS_GCC_LIKE(compiler, ...) \
    YUP_IGNORE_WARNINGS_GCC_LIKE (compiler, __VA_ARGS__)
#else
#define YUP_PUSH_WARNINGS_GCC_LIKE(compiler, ...) \
    YUP_IGNORE_WARNINGS_GCC_LIKE (compiler, "-Wpragmas", __VA_ARGS__)
#endif

/** Pop the current warning scope. */
#define YUP_POP_WARNINGS_GCC_LIKE(compiler) \
    _Pragma (YUP_TO_STRING (compiler diagnostic pop))

/** Push/pop warnings on compilers with gcc-like warning flags.
        These macros expand to nothing on other compilers (like MSVC).
    */
#if YUP_CLANG
#define YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE(...) YUP_PUSH_WARNINGS_GCC_LIKE (clang, __VA_ARGS__)
#define YUP_END_IGNORE_WARNINGS_GCC_LIKE YUP_POP_WARNINGS_GCC_LIKE (clang)
#else
#define YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE(...) YUP_PUSH_WARNINGS_GCC_LIKE (GCC, __VA_ARGS__)
#define YUP_END_IGNORE_WARNINGS_GCC_LIKE YUP_POP_WARNINGS_GCC_LIKE (GCC)
#endif
#else
#define YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE(...)
#define YUP_END_IGNORE_WARNINGS_GCC_LIKE
#endif

/** Push/pop warnings on MSVC. These macros expand to nothing on other
    compilers (like clang and gcc).
*/
#if YUP_MSVC
#define YUP_IGNORE_MSVC(warnings) __pragma (warning (disable \
                                                      : warnings))
#define YUP_BEGIN_IGNORE_WARNINGS_LEVEL_MSVC(level, warnings) \
    __pragma (warning (push, level)) YUP_IGNORE_MSVC (warnings)
#define YUP_BEGIN_IGNORE_WARNINGS_MSVC(warnings) \
    __pragma (warning (push)) YUP_IGNORE_MSVC (warnings)
#define YUP_END_IGNORE_WARNINGS_MSVC __pragma (warning (pop))
#else
#define YUP_IGNORE_MSVC(warnings)
#define YUP_BEGIN_IGNORE_WARNINGS_LEVEL_MSVC(level, warnings)
#define YUP_BEGIN_IGNORE_WARNINGS_MSVC(warnings)
#define YUP_END_IGNORE_WARNINGS_MSVC
#endif

#if YUP_MAC || YUP_IOS
#define YUP_SANITIZER_ATTRIBUTE_MINIMUM_CLANG_VERSION 11
#else
#define YUP_SANITIZER_ATTRIBUTE_MINIMUM_CLANG_VERSION 9
#endif

#define YUP_BEGIN_IGNORE_DEPRECATION_WARNINGS                        \
    YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations") \
    YUP_BEGIN_IGNORE_WARNINGS_MSVC (4996)

#define YUP_END_IGNORE_DEPRECATION_WARNINGS \
    YUP_END_IGNORE_WARNINGS_MSVC            \
    YUP_END_IGNORE_WARNINGS_GCC_LIKE

/** Disable sanitizers for a range of functions.

    This functionality doesn't seem to exist on GCC yet, so at the moment this only works for clang.
*/
#if YUP_CLANG && __clang_major__ >= YUP_SANITIZER_ATTRIBUTE_MINIMUM_CLANG_VERSION
#define YUP_BEGIN_NO_SANITIZE(warnings) \
    _Pragma (YUP_TO_STRING (clang attribute push (__attribute__ ((no_sanitize (warnings))), apply_to = function)))
#define YUP_END_NO_SANITIZE _Pragma (YUP_TO_STRING (clang attribute pop))
#else
#define YUP_BEGIN_NO_SANITIZE(warnings)
#define YUP_END_NO_SANITIZE
#endif

#undef YUP_SANITIZER_ATTRIBUTE_MINIMUM_CLANG_VERSION
