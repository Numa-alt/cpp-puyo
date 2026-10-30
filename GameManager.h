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

public:
    void Execute(std::uint32_t systemTimeMs, std::uint32_t held);

    //
    void SetState(GameState next);

    GameState GetState()const { return mGameState; };

    void Title();
    void Playing(std::uint32_t systemTime);
    void GameOver();
    void GameClear();

    const Board& GetBoard() const
    {
        return mPlayingManager.GetBoard();
    }

    const FallingPair& GetFallingPair() const
    {
        return mPlayingManager.GetFallingPair();
    }
    const FallingPair& GetNextPair() const
    {
        return mPlayingManager.GetNextPair();
    }

    PlayState GetPlayState()const { return mPlayingManager.GetState(); }

    void SetRandomSeed(std::uint32_t randomSeed)
    {
        mPlayingManager.SetRandomSeed(randomSeed);
    }

    std::uint32_t GetRandomSeed()const { return mPlayingManager.GetRandomSeed(); }

    int GetChainCount() const { return mPlayingManager.GetChainCount(); }
    int GetScore() const { return mPlayingManager.GetScore(); }

    bool IsGameEventEmpty()const { return mGameEventManager.IsEmpty(); }
    GameEvent PopGameEvent() { return mGameEventManager.Pop(); }

    std::uint32_t GetAutoFallIntervalMs()const { return mPlayingManager.GetAutoFallIntervalMs(); }
    std::uint32_t GetTotalEraseBlockCount()const { return mPlayingManager.GetTotalEraseBlockCount(); }

};
