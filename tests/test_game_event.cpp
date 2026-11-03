#include <cassert>
#include <iostream>
#include "GameEvent.h"

int main()
{
    GameEvent event(
        GameEventType::Chain,
        5,
        11,
        10
        );

    assert(event.GetType() == GameEventType::Chain);
    assert(event.GetX() == 5);
    assert(event.GetY() == 11);
    assert(event.GetValue() == 10);

    GameEventManager manager{};

    //73件いれる
    for (int i = 0; i < 73; ++i)
    {
        int v = i & 0x3F;
        bool success = manager.Push(GameEventType::Chain, 0, 0, static_cast<std::uint16_t>(v));
        assert(success);
    }
    for (int i = 0; i < 73; ++i)
    {
        GameEvent ge = manager.Pop();
        std::uint16_t v = i & 0x3F;
        assert(ge.GetValue() == v);
    }

    //74件目を入れる
    for (int i = 0; i < 73; ++i)
    {
        int v = i & 0x3F;
        bool success = manager.Push(GameEventType::Chain, 0, 0, static_cast<std::uint16_t>(v));
        assert(success);
    }

    //満杯、何もせず捨てる
    bool push74 = manager.Push(GameEventType::Chain, 0, 0, static_cast<std::uint16_t>(0));
    assert(push74 == false);
    //一旦空にする
    while (!manager.IsEmpty())
    {
        GameEvent ge = manager.Pop();
        assert(ge.GetX() == 0);
    }

    //wrap-around
    for (int i = 0; i < 60; ++i)
    {
        int v = i & 0x3F;
        bool success = manager.Push(GameEventType::Chain, 0, 0, static_cast<std::uint16_t>(v));
        assert(success);
    }

    for (int i = 0; i < 50; ++i)
    {
        GameEvent ge = manager.Pop();
        int v = i & 0x3F;
        assert(ge.GetValue() == v);
    }

    for (int i = 0; i < 60; ++i)
    {
        int v = i & 0x3F;
        bool success = manager.Push(GameEventType::Chain, 0, 0, static_cast<std::uint16_t>(v));
        assert(success);
    }

    //50 から 59を確認
    for (int i = 0; i < 10; ++i)
    {
        GameEvent ge = manager.Pop();
        assert(ge.GetValue() == (50 + i));
    }
    //0から59を確認
    for (int i = 0; i < 60; ++i)
    {
        GameEvent ge = manager.Pop();
        assert(ge.GetValue() == i);
    }

    assert(manager.IsEmpty());

    return 0;
}