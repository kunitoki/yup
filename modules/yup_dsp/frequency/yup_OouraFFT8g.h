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

    Copyright(C) 1996-2001 Takuya OOURA
    email: ooura@mmm.t.u-tokyo.ac.jp
    download: http://momonga.t.u-tokyo.ac.jp/~ooura/fft.html
    You may use, copy, modify this code for any purpose and
    without fee. You may distribute this ORIGINAL package.

  ==============================================================================
*/

#pragma once

namespace yup
{

void cdft (int n, int isgn, float* a, int* ip, float* w);
void rdft (int n, int isgn, float* a, int* ip, float* w);
void ddct (int n, int isgn, float* a, int* ip, float* w);
void ddst (int n, int isgn, float* a, int* ip, float* w);
void dfct (int n, float* a, float* t, int* ip, float* w);
void dfst (int n, float* a, float* t, int* ip, float* w);

} // namespace yup