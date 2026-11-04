#include <cassert>
#include "GameEvent.h"

int main()
{
    // GameEventに設定したtype / x / y / valueが
    // 16bitパック後も正しく復元できることを確認する
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

    // イベントキューの実効容量73件までPushでき、
    // FIFO順で正しくPopできることを確認する
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

    // キューが満杯のとき、74件目のPushを拒否することを確認する
    for (int i = 0; i < 73; ++i)
    {
        int v = i & 0x3F;
        bool success = manager.Push(GameEventType::Chain, 0, 0, static_cast<std::uint16_t>(v));
        assert(success);
    }
    bool push74 = manager.Push(GameEventType::Chain, 0, 0, static_cast<std::uint16_t>(0));
    assert(push74 == false);


    // 次のテストのため、キューを空にする
    while (!manager.IsEmpty())
    {
        GameEvent ge = manager.Pop();
        assert(ge.GetX() == 0);
    }

    // 読み書き位置が配列末尾をまたいでも、
    // FIFO順が維持されることを確認する
    for (int i = 0; i < 60; ++i)
    {
        int v = i & 0x3F;
        bool success = manager.Push(GameEventType::Chain, 0, 0, static_cast<std::uint16_t>(v));
        assert(success);
    }

    // 先頭50件を取り出す
    for (int i = 0; i < 50; ++i)
    {
        GameEvent ge = manager.Pop();
        int v = i & 0x3F;
        assert(ge.GetValue() == v);
    }

    // 空いた領域へさらに60件追加し、wrap-aroundを発生させる
    for (int i = 0; i < 60; ++i)
    {
        int v = i & 0x3F;
        bool success = manager.Push(GameEventType::Chain, 0, 0, static_cast<std::uint16_t>(v));
        assert(success);
    }

    // 最初に残っていた50〜59が先に取り出されることを確認する
    for (int i = 0; i < 10; ++i)
    {
        GameEvent ge = manager.Pop();
        assert(ge.GetValue() == (50 + i));
    }
    // その後に追加した0〜59が順番どおり取り出されることを確認する
    for (int i = 0; i < 60; ++i)
    {
        GameEvent ge = manager.Pop();
        assert(ge.GetValue() == i);
    }

    // 全件取り出した後は空になっていることを確認する
    assert(manager.IsEmpty());

    return 0;
}