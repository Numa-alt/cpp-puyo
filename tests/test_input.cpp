#include <cassert>
#include "InputManager.h"

int main()
{
    InputManager input;

    input.Execute(0, 0);

    assert(input.IsHeld(Button::Left) == false);
    assert(input.IsPressed(Button::Left) == false);

    input.Execute(10, ButtonBit(Button::Left));

    assert(input.IsHeld(Button::Left) == true);
    assert(input.IsPressed(Button::Left) == true);
    assert(input.IsReleased(Button::Left) == false);

    input.Execute(20, ButtonBit(Button::Left));

    assert(input.IsHeld(Button::Left) == true);
    assert(input.IsPressed(Button::Left) == false);
    assert(input.IsReleased(Button::Left) == false);

    input.Execute(30, 0);

    assert(input.IsHeld(Button::Left) == false);
    assert(input.IsPressed(Button::Left) == false);
    assert(input.IsReleased(Button::Left) == true);

    input.Execute(40, 0);

    assert(input.IsHeld(Button::Left) == false);
    assert(input.IsPressed(Button::Left) == false);
    assert(input.IsReleased(Button::Left) == false);

    input.Execute(100, ButtonBit(Button::Right));

    assert(input.IsHeld(Button::Right) == true);
    assert(input.IsPressed(Button::Right) == true);
    assert(input.IsRepeat(Button::Right) == true);

    input.Execute(399, ButtonBit(Button::Right));
    assert(input.IsRepeat(Button::Right) == false);

    input.Execute(400, ButtonBit(Button::Right));
    assert(input.IsRepeat(Button::Right) == true);

    input.Execute(499, ButtonBit(Button::Right));
    assert(input.IsRepeat(Button::Right) == false);

    input.Execute(500, ButtonBit(Button::Right));
    assert(input.IsRepeat(Button::Right) == true);

    input.Execute(510, 0);
    assert(input.IsReleased(Button::Right) == true);
    assert(input.IsRepeat(Button::Right) == false);

    input.Execute(600, ButtonBit(Button::Right));
    assert(input.IsPressed(Button::Right) == true);
    assert(input.IsRepeat(Button::Right) == true);

    return 0;
}