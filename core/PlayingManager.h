#pragma once

#include "GameTypes.h"
#include "FallingPair.h"
#include "Board.h"
#include "GameEvent.h"
#include "InputManager.h"

class PlayingManager
{
private:
    FallingPair mFallingPair; // 操作中のブロック
    FallingPair mNextPair;    // 次に出現するブロック
    Board mBoard;
    PlayState mState = PlayState::Spawn;
    std::uint32_t mChainCount = 0;
    std::uint32_t mScore = 0;
    GameResult mGameResult = GameResult::Playing;

    std::uint32_t mTotalEraseBlockCount = 0;//全部で何個消したか
    std::uint32_t mEraseBlockCountForSpeedUp = 0;//落下速度を加速させるための消したブロックのカウンタ

    std::uint32_t mRandomSeed = 0;

    //この時間経過すると操作中のブロックが１マス下に落ちる(ミリ秒)
    std::uint32_t mAutoFallIntervalMs = 0;

    //現在の時間を保持し、mAutoFallIntervalMs分時間が経過したらリセット(ミリ秒)
    std::uint32_t mLastAutoFallTimeMs = 0;

    //落下開始時の時間
    std::uint32_t mDropStartTimeMs = 0;

    std::uint32_t mSpawnStartTimeMs = 0;

    //
    std::uint32_t mEraseCheckTimeMs = 0;

    //
    std::uint32_t mEraseStartTimeMs = 0;

    //
    std::uint32_t mSystemTimeMs = 0;

    void PushEvent(GameEventManager& mgr, GameEventType type, int x, int y, std::uint32_t chainCount, BlockColor color);

public:
    void Start(std::uint32_t systemTimeMs, GameEventManager& gameEventManager);

    std::uint32_t GetChainCount() const;
    std::uint32_t GetScore()const;
    //スコア計算
    std::uint32_t CalcScore(std::uint32_t eraseBlockCount, std::uint32_t chainCount)const;

    void SetRandomSeed(std::uint32_t randomSeed);

    std::uint32_t GenerateRandom();

    void GenerateNextPair();

    // ステータス変更時に行う処理
    void SetState(PlayState next, GameEventManager& gameEventManager);
    PlayState GetState()const { return mState; }

    void Execute(const InputManager& input, std::uint32_t systemTimerMs, GameEventManager& gameEventManager);

    void Spawn(GameEventManager& gameEventManager);//新しいブロック出現
    void Control(const InputManager& input, GameEventManager& gameEventManager);
    void DropBlocks(GameEventManager& gameEventManager);
    void EraseCheck(GameEventManager& gameEventManager);
    void Erase(GameEventManager& gameEventManager);

    void Land(GameEventManager& gameEventManager);

    // GameManager側から結果を取得するため
    GameResult GetGameResult() const;

    //落下時間計測のタイマーをリセット
    void ResetAutoFallTimer();

    //前回の落下から一定時間経過したか？
    bool IsAutoFallDue() const;

    BlockColor GenerateBlock();

    const FallingPair& GetFallingPair() const;
    const FallingPair& GetNextPair() const;
    const Board& GetBoard() const;

    std::uint32_t GetRandomSeed() const;

    std::uint32_t GetAutoFallIntervalMs() const;
    std::uint32_t GetTotalEraseBlockCount() const;
};

