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

#if YUP_EMBED_DEFAULT_THEME_TEXT_SERIF_FONT
/**
 * Copyright 2011 The Roboto Flex Project Authors (https://github.com/googlefonts/roboto-flex)
 * 
 * This Font Software is licensed under the SIL Open Font License, Version 1.1.
 * 
 * This license is available with a FAQ at: http://scripts.sil.org/OFL
 */
const uint8_t RobotoFlexFont_data[] = {
#include "RobotoFlexFont.inc"
};

const std::size_t RobotoFlexFont_size = sizeof (RobotoFlexFont_data);
#endif

//==============================================================================

#if YUP_EMBED_DEFAULT_THEME_TEXT_MONOSPACE_FONT
/**
 * Copyright 2020 The JetBrains Mono Project Authors (https://github.com/JetBrains/JetBrainsMono)
 * 
 * This Font Software is licensed under the SIL Open Font License, Version 1.1.
 * 
 * This license is available with a FAQ at: https://scripts.sil.org/OFL
 */
const uint8_t JetBrainsMonoFont_data[] = {
#include "JetBrainsMonoFont.inc"
};

const std::size_t JetBrainsMonoFont_size = sizeof (JetBrainsMonoFont_data);
#endif

//==============================================================================

#if YUP_EMBED_DEFAULT_THEME_ICON_FONT
/**
 * Copyright (c) 2026 Fonticons, Inc. (https://fontawesome.com) with Reserved Font Name: "Font Awesome".
 * 
 * This Font Software is licensed under the SIL Open Font License, Version 1.1.
 * 
 * This license is available with a FAQ at: http://scripts.sil.org/OFL
 */
const uint8_t FontAwesome7Font_data[] = {
#include "FontAwesome7Font.inc"
};

const std::size_t FontAwesome7Font_size = sizeof (FontAwesome7Font_data);
#endif

} // namespace yup
