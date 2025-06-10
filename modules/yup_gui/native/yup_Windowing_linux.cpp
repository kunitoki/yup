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

namespace yup
{

//==============================================================================

class X11Functions
{
public:
    ~X11Functions()
    {
        if (libraryHandle != nullptr)
        {
            dlclose (libraryHandle);
            libraryHandle = nullptr;
        }
    }

    bool isX11Available() const
    {
        return libraryHandle != nullptr;
    }

    void (*XReparentWindow) (Display*, Window, Window, int, int) = nullptr;

    YUP_DECLARE_SINGLETON (X11Functions, false)

private:
    template <typename F>
    bool lookupFunction (const char* functionName, F& function)
    {
        if (libraryHandle == nullptr)
            return false;

        function = reinterpret_cast<std::remove_reference_t<F>> (dlsym (libraryHandle, functionName));
        if (function == nullptr)
        {
            YUP_DBG ("Failed to load " << functionName);

            dlclose (libraryHandle);
            libraryHandle = nullptr;

            clearFunctions();

            return false;
        }

        return true;
    }

    void clearFunctions()
    {
        this->XReparentWindow = nullptr;
    }

    X11Functions()
    {
        libraryHandle = dlopen ("libX11.so", RTLD_GLOBAL | RTLD_LAZY);
        if (libraryHandle == nullptr)
        {
            YUP_DBG ("Failed to load libX11.so");
            return;
        }

        if (! lookupFunction ("XReparentWindow", this->XReparentWindow))
            return;
    }

    void* libraryHandle = nullptr;
};

YUP_IMPLEMENT_SINGLETON (X11Functions)

//==============================================================================

Rectangle<int> getNativeWindowPosition (void* nativeWindow)
{
    return {};
}

} // namespace yup
