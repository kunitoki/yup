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

#include <yup_core/yup_core.h>

YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wcast-align")
YUP_BEGIN_IGNORE_WARNINGS_MSVC (4180)

#if YUP_PYTHON_USE_EXTERNAL_PYBIND11
#include <pybind11/embed.h>

#if defined (YUP_PYTHON_INCLUDE_PYBIND11_CAST)
#include <pybind11/cast.h>
#undef YUP_PYTHON_INCLUDE_PYBIND11_CAST
#endif

#if defined (YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS)
#include <pybind11/operators.h>
#undef YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#endif

#if defined (YUP_PYTHON_INCLUDE_PYBIND11_STL)
#include <pybind11/stl.h>
#undef YUP_PYTHON_INCLUDE_PYBIND11_STL
#endif

#if defined (YUP_PYTHON_INCLUDE_PYBIND11_FUNCTIONAL)
#include <pybind11/functional.h>
#undef YUP_PYTHON_INCLUDE_PYBIND11_FUNCTIONAL
#endif

#if defined (YUP_PYTHON_INCLUDE_PYBIND11_IOSTREAM)
#include <pybind11/iostream.h>
#undef YUP_PYTHON_INCLUDE_PYBIND11_IOSTREAM
#endif

#if defined (YUP_PYTHON_INCLUDE_PYBIND11_NUMPY)
#include <pybind11/numpy.h>
#undef YUP_PYTHON_INCLUDE_PYBIND11_NUMPY
#endif

#else // YUP_PYTHON_USE_EXTERNAL_PYBIND11
#include "../pybind11/embed.h"

#if defined (YUP_PYTHON_INCLUDE_PYBIND11_CAST)
#include "../pybind11/cast.h"
#undef YUP_PYTHON_INCLUDE_PYBIND11_CAST
#endif

#if defined (YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS)
#include "../pybind11/operators.h"
#undef YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#endif

#if defined (YUP_PYTHON_INCLUDE_PYBIND11_STL)
#include "../pybind11/stl.h"
#undef YUP_PYTHON_INCLUDE_PYBIND11_STL
#endif

#if defined (YUP_PYTHON_INCLUDE_PYBIND11_FUNCTIONAL)
#include "../pybind11/functional.h"
#undef YUP_PYTHON_INCLUDE_PYBIND11_FUNCTIONAL
#endif

#if defined (YUP_PYTHON_INCLUDE_PYBIND11_IOSTREAM)
#include "../pybind11/iostream.h"
#undef YUP_PYTHON_INCLUDE_PYBIND11_IOSTREAM
#endif

#if defined (YUP_PYTHON_INCLUDE_PYBIND11_NUMPY)
#include "../pybind11/numpy.h"
#undef YUP_PYTHON_INCLUDE_PYBIND11_NUMPY
#endif

#endif

YUP_END_IGNORE_WARNINGS_GCC_LIKE
YUP_END_IGNORE_WARNINGS_MSVC
