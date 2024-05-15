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
/** Represents a single key press, encapsulating information about the key and any associated modifiers.

    This class is designed to represent a key press in a platform-independent manner, storing details about the key code,
    any modifier keys that were active (like Shift, Ctrl, etc.), and the Unicode character for key presses that generate text.
*/
class JUCE_API KeyPress
{
public:
    //==============================================================================
    /** Default constructor, initializes a key press with no key or modifiers. */
    constexpr KeyPress () noexcept = default;

    /** Constructs a KeyPress with a specified key code.

        @param newKey The key code associated with the key press.
    */
    constexpr KeyPress (int newKey) noexcept
        : key (newKey)
    {
    }

    /** Constructs a KeyPress with a specified key code and modifiers.

        @param newKey The key code for the key press.
        @param newModifiers The modifiers that were active during the key press.
    */
    constexpr KeyPress (int newKey, KeyModifiers newModifiers) noexcept
        : key (newKey)
        , modifiers (newModifiers)
    {
    }

    /** Constructs a KeyPress with a specified key code, modifiers, and a scancode representing the text character.

        @param newKey The key code for the key press.
        @param newModifiers The modifiers that were active during the key press.
        @param newScancode The Unicode scancode that represents the text character produced by the key press.
    */
    constexpr KeyPress (int newKey, KeyModifiers newModifiers, char32_t newScancode) noexcept
        : key (newKey)
        , modifiers (newModifiers)
        , scancode (newScancode)
    {
    }

    //==============================================================================
    constexpr KeyPress (const KeyPress& other) noexcept = default;
    constexpr KeyPress (KeyPress&& other) noexcept = default;
    constexpr KeyPress& operator= (const KeyPress& other) noexcept = default;
    constexpr KeyPress& operator= (KeyPress&& other) noexcept = default;

    //==============================================================================
    /** Gets the key code of the key press.

        @return The key code.
    */
    constexpr int getKey() const noexcept
    {
        return key;
    }

    //==============================================================================
    /** Gets the modifiers associated with the key press.

        @return The modifiers.
    */
    constexpr KeyModifiers getModifiers() const noexcept
    {
        return modifiers;
    }

    //==============================================================================
    /** Gets the Unicode text character produced by the key press, if any.

        This is typically relevant for key presses that result in a printable character.

        @return The Unicode character code.
    */
    constexpr char32_t getTextCharacter() const noexcept
    {
        return scancode;
    }

    //==============================================================================
    /** Compares two KeyPress objects for equality.

        Two key presses are considered equal if they have the same key code, modifiers, and scancode.

        @param other The other KeyPress to compare with.

        @return True if the KeyPress objects are equal, false otherwise.
    */
    constexpr bool operator== (const KeyPress& other) const noexcept
    {
        return key == other.key && modifiers == other.modifiers && scancode == other.scancode;
    }

    /** Compares two KeyPress objects for inequality.

        @param other The other KeyPress to compare with.

        @return True if the KeyPress objects are not equal, false otherwise.
    */
    constexpr bool operator!= (const KeyPress& other) const noexcept
    {
        return !(*this == other);
    }

    //==============================================================================
    /** Aliases for commonly used keys. */
    static constexpr int spaceKey = 32;
    static constexpr int apostropheKey = 39;
    static constexpr int commaKey = 44;
    static constexpr int minusKey = 45;
    static constexpr int periodKey = 46;
    static constexpr int slashKey = 47;
    static constexpr int number0Key = 48;
    static constexpr int number1Key = 49;
    static constexpr int number2Key = 50;
    static constexpr int number3Key = 51;
    static constexpr int number4Key = 52;
    static constexpr int number5Key = 53;
    static constexpr int number6Key = 54;
    static constexpr int number7Key = 55;
    static constexpr int number8Key = 56;
    static constexpr int number9Key = 57;
    static constexpr int semicolonKey = 59;
    static constexpr int equalKey = 61;
    static constexpr int textAKey = 65;
    static constexpr int textBKey = 66;
    static constexpr int textCKey = 67;
    static constexpr int textDKey = 68;
    static constexpr int textEKey = 69;
    static constexpr int textFKey = 70;
    static constexpr int textGKey = 71;
    static constexpr int textHKey = 72;
    static constexpr int textIKey = 73;
    static constexpr int textJKey = 74;
    static constexpr int textKKey = 75;
    static constexpr int textLKey = 76;
    static constexpr int textMKey = 77;
    static constexpr int textNKey = 78;
    static constexpr int textOKey = 79;
    static constexpr int textPKey = 80;
    static constexpr int textQKey = 81;
    static constexpr int textRKey = 82;
    static constexpr int textSKey = 83;
    static constexpr int textTKey = 84;
    static constexpr int textUKey = 85;
    static constexpr int textVKey = 86;
    static constexpr int textWKey = 87;
    static constexpr int textXKey = 88;
    static constexpr int textYKey = 89;
    static constexpr int textZKey = 90;
    static constexpr int leftBracketKey = 91;
    static constexpr int backslashKey = 92;
    static constexpr int rightBracketKey = 93;
    static constexpr int graveAccentKey = 96;
    static constexpr int world1Key = 161;
    static constexpr int world2Key = 162;
    static constexpr int escapeKey = 256;
    static constexpr int enterKey = 257;
    static constexpr int tabKey = 258;
    static constexpr int backspaceKey = 259;
    static constexpr int insertKey = 260;
    static constexpr int deleteKey = 261;
    static constexpr int rightKey = 262;
    static constexpr int leftKey = 263;
    static constexpr int downKey = 264;
    static constexpr int upKey = 265;
    static constexpr int pageUpKey = 266;
    static constexpr int pageDownKey = 267;
    static constexpr int homeKey = 268;
    static constexpr int endKey = 269;
    static constexpr int capsLockKey = 280;
    static constexpr int scrollLockKey = 281;
    static constexpr int numLockKey = 282;
    static constexpr int printScreenKey = 283;
    static constexpr int pauseKey = 284;
    static constexpr int f1Key = 290;
    static constexpr int f2Key = 291;
    static constexpr int f3Key = 292;
    static constexpr int f4Key = 293;
    static constexpr int f5Key = 294;
    static constexpr int f6Key = 295;
    static constexpr int f7Key = 296;
    static constexpr int f8Key = 297;
    static constexpr int f9Key = 298;
    static constexpr int f10Key = 299;
    static constexpr int f11Key = 300;
    static constexpr int f12Key = 301;
    static constexpr int f13Key = 302;
    static constexpr int f14Key = 303;
    static constexpr int f15Key = 304;
    static constexpr int f16Key = 305;
    static constexpr int f17Key = 306;
    static constexpr int f18Key = 307;
    static constexpr int f19Key = 308;
    static constexpr int f20Key = 309;
    static constexpr int f21Key = 310;
    static constexpr int f22Key = 311;
    static constexpr int f23Key = 312;
    static constexpr int f24Key = 313;
    static constexpr int f25Key = 314;
    static constexpr int kp0Key = 320;
    static constexpr int kp1Key = 321;
    static constexpr int kp2Key = 322;
    static constexpr int kp3Key = 323;
    static constexpr int kp4Key = 324;
    static constexpr int kp5Key = 325;
    static constexpr int kp6Key = 326;
    static constexpr int kp7Key = 327;
    static constexpr int kp8Key = 328;
    static constexpr int kp9Key = 329;
    static constexpr int kpDecimalKey = 330;
    static constexpr int kpDivideKey = 331;
    static constexpr int kpMultiplyKey = 332;
    static constexpr int kpSubtractKey = 333;
    static constexpr int kpAddKey = 334;
    static constexpr int kpEnterKey = 335;
    static constexpr int kpEqualKey = 336;
    static constexpr int leftShiftKey = 340;
    static constexpr int leftControlKey = 341;
    static constexpr int leftAltKey = 342;
    static constexpr int leftSuperKey = 343;
    static constexpr int rightShiftKey = 344;
    static constexpr int rightControlKey = 345;
    static constexpr int rightAltKey = 346;
    static constexpr int rightSuperKey = 347;
    static constexpr int menuKey = 348;

private:
    int32_t key = 0;
    char32_t scancode = '\0';
    KeyModifiers modifiers;
};

} // namespace yup
