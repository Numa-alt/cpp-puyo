#include <cassert>
#include "PlayingManager.h"


int main()
{
    PlayingManager pm{};
    InputManager input{};
    GameEventManager gem{};

    std::uint32_t systemTimeMs = 0;

    //pm.SetState(PlayState::Spawn);
    pm.Start(systemTimeMs, gem);

    for (int i = 0; i < 100; ++i)
    {
        pm.Execute(input, systemTimeMs, gem);
        if (pm.GetState() == PlayState::Control)
        {
            break;
        }
        systemTimeMs += 10;
    }

    int beforeY = pm.GetFallingPair().GetY();
    input.Execute(systemTimeMs, ButtonBit(Button::Up));
    pm.Execute(input, systemTimeMs, gem);
    ++systemTimeMs;
    input.Execute(systemTimeMs, 0);
    systemTimeMs += 5000;
    input.Execute(systemTimeMs, ButtonBit(Button::Up));
    pm.Execute(input, systemTimeMs, gem);
    ++systemTimeMs;
    input.Execute(systemTimeMs, 0);

    int afterY = pm.GetFallingPair().GetY();
    assert(beforeY == afterY);

    systemTimeMs += 2000;
    pm.Execute(input, systemTimeMs, gem);
    int movedY = pm.GetFallingPair().GetY();
    assert(beforeY == movedY - 1);

    return 0;
}