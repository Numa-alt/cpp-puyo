#include "GameManager.h"

void GameManager::SetState(GameState next)
{
    if (next == GameState::Playing)
    {
        mPlayingManager.Start(mSystemTimeMs, mGameEventManager);
    }
    mGameState = next;
}

void GameManager::Execute(std::uint32_t systemTimeMs, std::uint32_t held)
{
    mSystemTimeMs = systemTimeMs;

    mInputManager.Execute(mSystemTimeMs, held);

    switch (mGameState)
    {
    case GameState::Title:
        Title();
        break;

    case GameState::Playing:
        Playing(mSystemTimeMs);
        break;

    case GameState::GameOver:
        GameOver();
        break;

    case GameState::GameClear:
        GameClear();
        break;
    }
}

// タイトル
void GameManager::Title()
{
    if (mInputManager.IsPressed(Button::Rotate))
    {
        SetState(GameState::Playing);
    }
}
// ゲーム中
void GameManager::Playing(std::uint32_t systemTimeMs)
{
    mPlayingManager.Execute(mInputManager, systemTimeMs, mGameEventManager);

    GameResult result = mPlayingManager.GetGameResult();
    switch (result)
    {
    case GameResult::Playing:
        break;

    case GameResult::GameOver:
        SetState(GameState::GameOver);
        break;

    case GameResult::GameClear:
        SetState(GameState::GameClear);
        break;
    }
}
// ゲームオーバー
void GameManager::GameOver()
{
    if (mInputManager.IsPressed(Button::Rotate))
    {
        // タイトルへ
        SetState(GameState::Title);
    }
}
// ゲームクリア
void GameManager::GameClear()
{
    if (mInputManager.IsPressed(Button::Rotate))
    {
        // タイトルへ
        SetState(GameState::Title);
    }
}

const Board& GameManager::GetBoard() const
{
    return mPlayingManager.GetBoard();
}

const FallingPair& GameManager::GetFallingPair() const
{
    return mPlayingManager.GetFallingPair();
}
const FallingPair& GameManager::GetNextPair() const
{
    return mPlayingManager.GetNextPair();
}

PlayState GameManager::GetPlayState()const
{
    return mPlayingManager.GetState();
}

void GameManager::SetRandomSeed(std::uint32_t randomSeed)
{
    mPlayingManager.SetRandomSeed(randomSeed);
}

std::uint32_t GameManager::GetRandomSeed()const
{
    return mPlayingManager.GetRandomSeed();
}

std::uint32_t GameManager::GetChainCount() const
{
    return mPlayingManager.GetChainCount();
}
std::uint32_t GameManager::GetScore() const
{
    return mPlayingManager.GetScore();
}

bool GameManager::IsGameEventEmpty()const
{
    return mGameEventManager.IsEmpty();
}
GameEvent GameManager::PopGameEvent()
{
    return mGameEventManager.Pop();
}

std::uint32_t GameManager::GetAutoFallIntervalMs()const
{
    return mPlayingManager.GetAutoFallIntervalMs();
}
std::uint32_t GameManager::GetTotalEraseBlockCount()const
{
    return mPlayingManager.GetTotalEraseBlockCount();
}
GameState GameManager::GetState() const
{
    return mGameState;
}
