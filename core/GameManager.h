#pragma once

#include <cstdint>

#include "GameTypes.h"
#include "InputManager.h"
#include "PlayingManager.h"
#include "GameEvent.h"

class GameManager
{
private:
    GameState mGameState = GameState::Title;

    InputManager mInputManager;

    PlayingManager mPlayingManager;

    std::uint32_t mSystemTimeMs = 0;

    GameEventManager mGameEventManager;

    void Title();
    void Playing(std::uint32_t systemTime);
    void GameOver();
    void GameClear();

public:
    void Execute(std::uint32_t systemTimeMs, std::uint32_t held);

    //
    void SetState(GameState next);

    GameState GetState() const;

    const Board& GetBoard() const;

    const FallingPair& GetFallingPair() const;

    const FallingPair& GetNextPair() const;

    PlayState GetPlayState()const;

    void SetRandomSeed(std::uint32_t randomSeed);

    std::uint32_t GetRandomSeed()const;

    std::uint32_t GetChainCount() const;
    std::uint32_t GetScore() const;

    bool IsGameEventEmpty()const;
    GameEvent PopGameEvent();

    std::uint32_t GetAutoFallIntervalMs()const;
    std::uint32_t GetTotalEraseBlockCount()const;

};
