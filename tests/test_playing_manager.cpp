#include <cassert>
#include "PlayingManager.h"
#include "GameConfig.h"

constexpr std::uint32_t ElapsedBeforePauseMs = 800;
constexpr std::uint32_t RemainingBeforeFallMs = InitialAutoFallIntervalMs - ElapsedBeforePauseMs;

// ゲームを開始し、PlayState::Controlになるまで進める
void StartAndAdvanceToControl(PlayingManager& pm, InputManager& input, GameEventManager& gem, std::uint32_t& systemTimeMs);

// Pause中の経過時間が自動落下に加算されず、
// Resume後は通常どおり自動落下が再開することを確認する
void TestPauseDoesNotAdvanceAutoFall();

// Pause前の自動落下までの残り時間が、Resume後にも保持されることを確認する
void TestPausePreservesAutoFallRemainingTime();

// PlayState::Spawn -> PlayState::Control へ遷移することを確認する
void TestSpawnToControl();

// PlayState::Control -> PlayState::EraseCheck へ遷移することを確認する
void TestControlToEraseCheck();

// 片側が浮いたFallingPairの着地時に、
// PlayState::Control -> PlayState::DropBlocks へ遷移することを確認する
void TestControlToDropBlocks();

int main()
{
    TestPauseDoesNotAdvanceAutoFall();
    TestPausePreservesAutoFallRemainingTime();

    TestSpawnToControl();
    TestControlToEraseCheck();
    TestControlToDropBlocks();

    return 0;
}
// ゲームを開始し、PlayState::Controlになるまで進める
void StartAndAdvanceToControl(PlayingManager& pm, InputManager& input, GameEventManager& gem, std::uint32_t& systemTimeMs)
{
    systemTimeMs = 0;

    pm.Start(systemTimeMs, gem);

    systemTimeMs += SpawnDelayMs;

    pm.Execute(input, systemTimeMs, gem);

    assert(pm.GetState() == PlayState::Control);
}

// Pause中の経過時間が自動落下に加算されず、
// Resume後は通常どおり自動落下が再開することを確認する
void TestPauseDoesNotAdvanceAutoFall()
{
    PlayingManager pm{};
    InputManager input{};
    GameEventManager gem{};
    std::uint32_t systemTimeMs = 0;

    StartAndAdvanceToControl(pm, input, gem, systemTimeMs);

    const int beforeY = pm.GetFallingPair().GetY();
    input.Execute(systemTimeMs, ButtonBit(Button::Up));
    pm.Execute(input, systemTimeMs, gem);
    ++systemTimeMs;
    input.Execute(systemTimeMs, 0);
    systemTimeMs += 5000;
    input.Execute(systemTimeMs, ButtonBit(Button::Up));
    pm.Execute(input, systemTimeMs, gem);
    ++systemTimeMs;
    input.Execute(systemTimeMs, 0);

    const int afterY = pm.GetFallingPair().GetY();
    assert(beforeY == afterY);

    systemTimeMs += InitialAutoFallIntervalMs;
    pm.Execute(input, systemTimeMs, gem);
    const int movedY = pm.GetFallingPair().GetY();
    assert(movedY == beforeY - 1);
}
// Pause前の自動落下までの残り時間が、Resume後にも保持されることを確認する
void TestPausePreservesAutoFallRemainingTime()
{
    PlayingManager pm{};
    InputManager input{};
    GameEventManager gem{};
    std::uint32_t systemTimeMs = 0;

    StartAndAdvanceToControl(pm, input, gem, systemTimeMs);

    systemTimeMs += ElapsedBeforePauseMs;

    const int beforeY = pm.GetFallingPair().GetY();

    // Pause
    input.Execute(systemTimeMs, ButtonBit(Button::Up));
    pm.Execute(input, systemTimeMs, gem);
    input.Execute(systemTimeMs, 0);
    pm.Execute(input, systemTimeMs, gem);

    systemTimeMs += 5000;

    // Resume
    input.Execute(systemTimeMs, ButtonBit(Button::Up));
    pm.Execute(input, systemTimeMs, gem);
    input.Execute(systemTimeMs, 0);
    pm.Execute(input, systemTimeMs, gem);

    // 自動落下まで残り1msでは落下しない
    systemTimeMs += RemainingBeforeFallMs - 1;
    pm.Execute(input, systemTimeMs, gem);
    assert(pm.GetFallingPair().GetY() == beforeY);

    // 残り1ms経過すると落下する
    ++systemTimeMs;
    pm.Execute(input, systemTimeMs, gem);
    assert(pm.GetFallingPair().GetY() == (beforeY - 1));
}

// 状態遷移 PlayState::Spawn -> PlayState::Control へ遷移することを確認する
void TestSpawnToControl()
{
    PlayingManager pm{};
    InputManager input{};
    GameEventManager gem{};
    std::uint32_t systemTimeMs = 0;

    pm.Start(systemTimeMs, gem);
    assert(pm.GetState() == PlayState::Spawn);

    systemTimeMs += SpawnDelayMs;
    pm.Execute(input, systemTimeMs, gem);

    assert(pm.GetState() == PlayState::Control);
}

// 状態遷移 PlayState::Control -> PlayState::EraseCheck へ遷移することを確認する
void TestControlToEraseCheck()
{
    PlayingManager pm{};
    InputManager input{};
    GameEventManager gem{};
    std::uint32_t systemTimeMs = 0;

    StartAndAdvanceToControl(pm, input, gem, systemTimeMs);

    // 着地するまで待つ
    for (int i = 0; i < HEIGHT + 5; ++i)
    {
        systemTimeMs += InitialAutoFallIntervalMs;
        pm.Execute(input, systemTimeMs, gem);
        if (pm.GetState() != PlayState::Control)
        {
            break;
        }
    }

    assert(pm.GetState() == PlayState::EraseCheck);
}

// 片側が浮いたFallingPairの着地時に、
// PlayState::Control -> PlayState::DropBlocks へ遷移することを確認する
void TestControlToDropBlocks()
{
    PlayingManager pm{};
    InputManager input{};
    GameEventManager gem{};
    std::uint32_t systemTimeMs = 0;

    StartAndAdvanceToControl(pm, input, gem, systemTimeMs);

    // 1組目を縦向きのまま着地させて、Spawn位置の下に2段の山を作る
    // 着地するまで待つ
    for (int i = 0; i < HEIGHT + 5; ++i)
    {
        systemTimeMs += InitialAutoFallIntervalMs;
        pm.Execute(input, systemTimeMs, gem);
        if (pm.GetState() != PlayState::Control)
        {
            break;
        }
    }

    assert(pm.GetState() == PlayState::EraseCheck);

    systemTimeMs += EraseCheckDelayMs;
    pm.Execute(input, systemTimeMs, gem);

    // まだ消せるだけのブロックがないので、PlayState::Spawnになる
    assert(pm.GetState() == PlayState::Spawn);

    // 2組目を右向きにして着地させる。
    // 左側だけ既存ブロックに支えられ、右側が浮くため DropBlocks へ遷移する。

    // PlayState::Controlまで進める
    systemTimeMs += SpawnDelayMs;
    pm.Execute(input, systemTimeMs, gem);
    assert(pm.GetState() == PlayState::Control);

    // 横向きにする
    input.Execute(systemTimeMs, ButtonBit(Button::Rotate));
    pm.Execute(input, systemTimeMs, gem);
    const FallingPair fp = pm.GetFallingPair();
    assert(fp.GetDirection() == Direction::Right);

    input.Execute(systemTimeMs, 0);

    // 着地するまで待つ
    for (int i = 0; i < HEIGHT + 5; ++i)
    {
        systemTimeMs += InitialAutoFallIntervalMs;
        pm.Execute(input, systemTimeMs, gem);
        if (pm.GetState() != PlayState::Control)
        {
            break;
        }
    }

    assert(pm.GetState() == PlayState::DropBlocks);
}
