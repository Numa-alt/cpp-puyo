#include "InputManager.h"

void InputManager::Execute(std::uint32_t systemTimeMs, std::uint32_t held)
{
    mPressed = 0;
    mRepeat = 0;
    mReleased = 0;

    for (std::size_t i = 0; i < static_cast<std::size_t>(Button::Max); i++)
    {
        Button b = static_cast<Button>(i);
        std::uint32_t f = ButtonBit(b);
        if (held & f)
        {
            if (!(mHeld & f))
            {
                //前回おされてなくて、今回押されたとき
                mPressed |= f;
                mRepeat |= f;
                //押され続ける時間を計測開始
                mSystemTimeMs[i] = systemTimeMs;
                mRepeatCount[i] = 0;
            }
            //リピート入力の初回だけ多めに待つ
            std::uint32_t intervalTimeMs = 300;
            if (mRepeatCount[i] > 0)
            {
                intervalTimeMs = 100;
            }
            if ((systemTimeMs - mSystemTimeMs[i]) >= intervalTimeMs)
            {
                mRepeat |= f;
                mSystemTimeMs[i] = systemTimeMs;
                ++mRepeatCount[i];
            }
        }
        else
        {
            if (mHeld & f)
            {
                mReleased |= f;
            }
        }
    }
    mHeld = held;
}
