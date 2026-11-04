#include <cassert>
#include "InputManager.h"

int main()
{
    InputManager input;

    // 入力がない状態ではHeld / Pressedともにfalse
    input.Execute(0, 0);

    assert(input.IsHeld(Button::Left) == false);
    assert(input.IsPressed(Button::Left) == false);

    // Leftを押した瞬間はHeld / Pressedがtrueになる
    input.Execute(10, ButtonBit(Button::Left));

    assert(input.IsHeld(Button::Left) == true);
    assert(input.IsPressed(Button::Left) == true);
    assert(input.IsReleased(Button::Left) == false);

    // 押し続けている間はHeldのみtrueになり、
    // Pressedは最初の1フレームだけでfalseになる
    input.Execute(20, ButtonBit(Button::Left));

    assert(input.IsHeld(Button::Left) == true);
    assert(input.IsPressed(Button::Left) == false);
    assert(input.IsReleased(Button::Left) == false);

    // Leftを離した瞬間はReleasedがtrueになる
    input.Execute(30, 0);

    assert(input.IsHeld(Button::Left) == false);
    assert(input.IsPressed(Button::Left) == false);
    assert(input.IsReleased(Button::Left) == true);

    // 離した次のフレームではReleasedもfalseに戻る
    input.Execute(40, 0);

    assert(input.IsHeld(Button::Left) == false);
    assert(input.IsPressed(Button::Left) == false);
    assert(input.IsReleased(Button::Left) == false);

    // Rightを押した瞬間はPressedに加えてRepeatもtrueになる
    input.Execute(100, ButtonBit(Button::Right));

    assert(input.IsHeld(Button::Right) == true);
    assert(input.IsPressed(Button::Right) == true);
    assert(input.IsRepeat(Button::Right) == true);

    // 初回Repeat待ち時間より前ではRepeatしない
    input.Execute(399, ButtonBit(Button::Right));
    assert(input.IsRepeat(Button::Right) == false);

    // 初回Repeat待ち時間に到達するとRepeatする
    input.Execute(400, ButtonBit(Button::Right));
    assert(input.IsRepeat(Button::Right) == true);

    // 2回目以降のRepeat間隔より前ではRepeatしない
    input.Execute(499, ButtonBit(Button::Right));
    assert(input.IsRepeat(Button::Right) == false);

    // 2回目以降のRepeat間隔に到達するとRepeatする
    input.Execute(500, ButtonBit(Button::Right));
    assert(input.IsRepeat(Button::Right) == true);

    // ボタンを離すとReleasedがtrueになり、Repeat状態も解除される
    input.Execute(510, 0);
    assert(input.IsReleased(Button::Right) == true);
    assert(input.IsRepeat(Button::Right) == false);

    // 離した後に再度押すと、新しいPressed / Repeatとして扱われる
    input.Execute(600, ButtonBit(Button::Right));
    assert(input.IsPressed(Button::Right) == true);
    assert(input.IsRepeat(Button::Right) == true);

    return 0;
}