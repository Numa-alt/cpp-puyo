#pragma once

#include <array>

#include "GameTypes.h"

class InputManager
{
private:
    std::uint32_t mHeld = 0;//押されている間１
    std::uint32_t mPressed = 0;//押された瞬間１
    std::uint32_t mReleased = 0;//離した瞬間１
    std::uint32_t mRepeat = 0;//リピート

    std::array<std::uint32_t, static_cast<size_t>(Button::Max)> mSystemTimeMs{};
    std::array<std::uint32_t, static_cast<size_t>(Button::Max)> mRepeatCount{};

public:

    void Execute(std::uint32_t systemTimeMs, std::uint32_t held);

    // 押されている
    bool IsHeld(Button button)const
    {
        return (mHeld & ButtonBit(button)) != 0;
    }

    // 押された瞬間
    bool IsPressed(Button button) const
    {
        return (mPressed & ButtonBit(button)) != 0;
    }

    // 離された瞬間
    bool IsReleased(Button button) const
    {
        return (mReleased & ButtonBit(button)) != 0;
    }

    // //リピート
    bool IsRepeat(Button button) const
    {
        return (mRepeat & ButtonBit(button)) != 0;
    }
};

