/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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
MaterialTable::MaterialTable()
    : defaultEntry (GpuSceneData::packMaterial (Material()))
{
    clear();
}

//==============================================================================
void MaterialTable::clear()
{
    ids.clear();
    entries.clear();
    entries.push_back (defaultEntry);
    overflowLogged = false;
}

int MaterialTable::getId (const Material& material)
{
    if (const auto found = ids.find (&material); found != ids.end())
        return found->second;

    // Too many materials is data, not a programming error: they shade as the default one
    if (static_cast<int> (entries.size()) >= maxMaterials)
    {
        if (! overflowLogged)
            YUP_DBG ("MaterialTable: more than " << maxMaterials << " materials in a frame, the rest use the default material");

        overflowLogged = true;
        return 0;
    }

    const auto id = static_cast<int> (entries.size());
    entries.push_back (GpuSceneData::packMaterial (material));
    ids.emplace (&material, id);
    return id;
}

} // namespace yup
