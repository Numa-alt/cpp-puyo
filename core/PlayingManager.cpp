
#include "PlayingManager.h"
#include "GameConfig.h"

#include <algorithm>

std::uint32_t PlayingManager::GetChainCount() const
{
    return mChainCount;
}
std::uint32_t PlayingManager::GetScore()const
{
    return mScore;
}

void PlayingManager::SetRandomSeed(std::uint32_t randomSeed)
{
    mRandomSeed = (randomSeed != 0) ? randomSeed : 0xDEADBEEF;
}

std::uint32_t PlayingManager::GenerateRandom()
{
    mRandomSeed ^= mRandomSeed << 13;
    mRandomSeed ^= mRandomSeed >> 17;
    mRandomSeed ^= mRandomSeed << 5;
    return static_cast<std::uint32_t>(mRandomSeed);
}
const FallingPair& PlayingManager::GetFallingPair() const
{
    return mFallingPair;
}
const FallingPair& PlayingManager::GetNextPair() const
{
    return mNextPair;
}
const Board& PlayingManager::GetBoard()const
{
    return mBoard;
}

std::uint32_t PlayingManager::GetRandomSeed()const
{
    return mRandomSeed;
}

std::uint32_t PlayingManager::GetAutoFallIntervalMs()const
{
    return mAutoFallIntervalMs;
}
std::uint32_t PlayingManager::GetTotalEraseBlockCount()const
{
    return mTotalEraseBlockCount;
}
// GameManager側から結果を取得するため
GameResult PlayingManager::GetGameResult() const
{
    return mGameResult;
}

void PlayingManager::Start(std::uint32_t systemTimeMs, GameEventManager& gameEventManager)
{
    mScore = 0;
    mChainCount = 0;
    mGameResult = GameResult::Playing;
    mBoard.Reset();
    mFallingPair.Reset();
    mNextPair.Reset();

    mTotalEraseBlockCount = 0;//全部で何個消したか
    mEraseBlockCountForSpeedUp = 0;//落下速度を加速させるための消したブロックのカウンタ

    mAutoFallIntervalMs = InitialAutoFallIntervalMs;//開始時はInitialAutoFallIntervalMsミリ秒ごとに１マス落下。ブロックを消していくと徐々に少なくする

    mDropStartTimeMs = 0;

    mEraseCheckTimeMs = 0;

    mEraseStartTimeMs = 0;

    mSpawnStartTimeMs = 0;

    mSystemTimeMs = systemTimeMs;

    GenerateNextPair();

    SetState(PlayState::Spawn, gameEventManager);
}

//落下時間計測のタイマーをリセット
void PlayingManager::ResetAutoFallTimer()
{
    //現在の時刻でリセットする
    mLastAutoFallTimeMs = mSystemTimeMs;
}
//前回の落下から一定時間経過したか？
bool PlayingManager::IsAutoFallDue() const
{
    if ((mSystemTimeMs - mLastAutoFallTimeMs) >= mAutoFallIntervalMs)
    {
        return true;
    }
    return false;
}
BlockColor PlayingManager::GenerateBlock()
{
    // ランダムに色を設定する
    switch (GenerateRandom() % 4) {
    case 0: return BlockColor::Red;
    case 1: return BlockColor::Blue;
    case 2: return BlockColor::Green;
    case 3: return BlockColor::Yellow;
    }
    return BlockColor::Red;
}
void PlayingManager::GenerateNextPair()
{
    // 出現位置
    mNextPair.SetPosition(2, HEIGHT - 2);
    mNextPair.SetColor(0, GenerateBlock());
    mNextPair.SetColor(1, GenerateBlock());
}

// ステータス変更時に行う処理
void PlayingManager::SetState(PlayState next, GameEventManager& gameEventManager)
{
    mState = next;

    switch (next)
    {
    case PlayState::Spawn:
        {
            mFallingPair = mNextPair;

            if (!mBoard.CanPlace(mFallingPair)) // 追加できる場所がない?
            {
                mGameResult = GameResult::GameOver;
                SetState(PlayState::Result, gameEventManager);
                return;
            }

            // 次のブロックの色をランダムで設定する
            GenerateNextPair();

            //連鎖数リセット
            mChainCount = 0;

            mSpawnStartTimeMs = mSystemTimeMs;
            break;
        }

    case PlayState::Control:
        {
            //落下タイマーをリセット
            ResetAutoFallTimer();
            break;
        }

    case PlayState::DropBlocks:
        {
            mDropStartTimeMs = mSystemTimeMs;
            break;
        }

    case PlayState::EraseCheck:
        {
            mEraseCheckTimeMs = mSystemTimeMs;
            break;
        }

    case PlayState::Erase:
        {
            mEraseStartTimeMs = mSystemTimeMs;

            //連鎖数増加
            mChainCount++;

            //消えるブロック
            {
                for (int ix = 0; ix < WIDTH; ix++)
                {
                    for (int iy = 0; iy < HEIGHT; iy++) {
                        if (mBoard.IsEraseBlock(ix, iy)) {
                            BlockColor color = mBoard.GetColor(ix, iy);
                            std::uint32_t chainCount = 0;
                            bool success = gameEventManager.Push(GameEventType::BlockErased, ix, iy, chainCount, color);
                        }
                    }
                }
            }

            //連鎖数
            {
                int x = 0;
                int y = 0;
                //真ん中あたりの消えるボロックの位置を取得する
                mBoard.SearchCenterEraseBlock(x, y);
                bool success = gameEventManager.Push(GameEventType::Chain, x, y, mChainCount, BlockColor::Empty);
            }

            break;
        }
    case PlayState::Result:
        break;
    }
}

void PlayingManager::Execute(const InputManager& input, std::uint32_t systemTimeMs, GameEventManager& gameEventManager)
{
    mSystemTimeMs = systemTimeMs;
    switch (mState)
    {
        // 新規ブロック追加
    case PlayState::Spawn:
        Spawn(gameEventManager);
        break;

        // 操作中
    case PlayState::Control:
        Control(input, gameEventManager);
        break;

        // 落下中
    case PlayState::DropBlocks:
        DropBlocks(gameEventManager);
        break;

        // 消えるかチェック
    case PlayState::EraseCheck:
        EraseCheck(gameEventManager);
        break;

        // 消去
    case PlayState::Erase:
        Erase(gameEventManager);
        break;

        // 結果表示中
    case PlayState::Result:
        break;
    }
}

// 新しいブロック出現
void PlayingManager::Spawn(GameEventManager& gameEventManager)
{
    if ((mSystemTimeMs - mSpawnStartTimeMs) >= SpawnDelayMs) // ブロック追加演出完了？
    {
        SetState(PlayState::Control, gameEventManager);
    }
}
// 落下中のブロックの操作
void PlayingManager::Control(const InputManager& input, GameEventManager& gameEventManager)
{
    if (input.IsRepeat(Button::Left)) {
        FallingPair pair = mFallingPair;
        pair.Move(-1, 0);
        if (mBoard.CanPlace(pair)) { // 左へ移動可能？
            // 左へ移動
            mFallingPair = pair;
        }
    }
    else if (input.IsRepeat(Button::Right)) {
        FallingPair pair = mFallingPair;
        pair.Move(1, 0);
        if (mBoard.CanPlace(pair)) { // 右へ移動可能？
            // 右へ移動
            mFallingPair = pair;
        }
    }
    else if (input.IsRepeat(Button::Down)) {
        FallingPair pair = mFallingPair;
        pair.Move(0, -1);
        if (mBoard.CanPlace(pair)) {//下へ移動可能？
            // 1段下へ移動
            mFallingPair = pair;
        }
        else {
            // 着地処理
            Land(gameEventManager);
            return;
        }
    }

    if (input.IsPressed(Button::Rotate)) {
        // 回転処理
        FallingPair pair = mFallingPair;

        // 90度
        pair.Rotate();
        if (mBoard.CanPlace(pair))
        {
            mFallingPair = pair;
        }
        else
        {
            bool slide = false;
            //相方が左にいる場合は右にずらしてみる
            if (pair.GetDirection() == Direction::Left)
            {
                FallingPair pairSlide = pair;
                pairSlide.Move(+1, 0);
                if (mBoard.CanPlace(pairSlide))
                {
                    mFallingPair = pairSlide;
                    slide = true;
                }
            }
            //相方が右にいる場合は左にずらしてみる
            else if (pair.GetDirection() == Direction::Right)
            {
                FallingPair pairSlide = pair;
                pairSlide.Move(-1, 0);
                if (mBoard.CanPlace(pairSlide))
                {
                    mFallingPair = pairSlide;
                    slide = true;
                }
            }

            if (!slide)
            {
                //スライドできなかった場合は
                // 180度
                pair.Rotate();
                if (mBoard.CanPlace(pair))
                {
                    mFallingPair = pair;
                }
            }
        }
    }

    //
    if (IsAutoFallDue()) { // 前回の落下から一定時間経過？
        // 1段落下
        FallingPair pair = mFallingPair;
        pair.Move(0, -1);
        if (mBoard.CanPlace(pair)) {
            mFallingPair = pair;
            //落下タイマーをリセット
            ResetAutoFallTimer();
        }
        else {
            // 着地して、一定時間経過している
            // 着地処理
            Land(gameEventManager);
        }
    }
}
//着地処理
void PlayingManager::Land(GameEventManager& gameEventManager)
{
    mBoard.PutPair(mFallingPair);

    if (!mBoard.HasFloatingBlocks())//浮いているブロックがない？
    {
        //浮いているブロックがないので消えるブロックがあるかチェック
        SetState(PlayState::EraseCheck, gameEventManager);
    }
    else {
        //浮いているブロックがある場合
        SetState(PlayState::DropBlocks, gameEventManager);
    }

}

// 消去チェック
void PlayingManager::EraseCheck(GameEventManager& gameEventManager) {

    if ((mSystemTimeMs - mEraseCheckTimeMs) >= EraseCheckDelayMs)
    {
        if (mBoard.FindEraseBlocks()) // 消える？
        {
            SetState(PlayState::Erase, gameEventManager);
        }
        else
        {
            SetState(PlayState::Spawn, gameEventManager);
        }
    }
}
//得点計算
std::uint32_t PlayingManager::CalcScore(std::uint32_t eraseBlockCount, std::uint32_t chainCount) const
{
    return eraseBlockCount * chainCount;
}
// ブロックを消去
void PlayingManager::Erase(GameEventManager& gameEventManager)
{
    if ((mSystemTimeMs - mEraseStartTimeMs) >= EraseDurationMs) // 消去処理終了？
    {
        std::uint32_t eraseBlockCount = mBoard.CountEraseBlocks();
        //スコア計算
        mScore += CalcScore(eraseBlockCount, mChainCount);

        mTotalEraseBlockCount += eraseBlockCount;//全部で何個消したか

        mEraseBlockCountForSpeedUp += eraseBlockCount;//落下速度を加速させるための消したブロックのカウンタ

        //規定の個数を消したか？
        while (mEraseBlockCountForSpeedUp >= EraseCountForSpeedUp) {
            mEraseBlockCountForSpeedUp -= EraseCountForSpeedUp;

            static_assert(MinAutoFallIntervalMs >= AutoFallDecreaseMs, "underflow guard");

            //落下速度アップ
            mAutoFallIntervalMs = std::max(MinAutoFallIntervalMs, mAutoFallIntervalMs - AutoFallDecreaseMs);
        }

        //ブロックを消す
        mBoard.EraseBlocks();

        if (mBoard.HasFloatingBlocks()) // 浮いているブロックがある？
        {
            SetState(PlayState::DropBlocks, gameEventManager); //
        }
        else
        {
            SetState(PlayState::Spawn, gameEventManager);
        }
    }
}

// ブロック落下処理
void PlayingManager::DropBlocks(GameEventManager& gameEventManager)
{
    if ((mSystemTimeMs - mDropStartTimeMs) >= DropStepMs) // 落下処理終了？
    {
        mBoard.DropFloatingBlocks();

        if (!mBoard.HasFloatingBlocks())//浮いているブロックがない？
        {
            //浮いているブロックがないので消えるブロックがあるかチェック
            SetState(PlayState::EraseCheck, gameEventManager);
        }
        else {
            //再度落下処理
            SetState(PlayState::DropBlocks, gameEventManager);
        }
    }
}
