#include <iostream>
#include <sstream>
#include <cassert>
#include <array>
#include <chrono>
#include <functional>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

#include <SDL.h>

#include "font8x8.h"

#include "GameTypes.h"
#include "FallingPair.h"
#include "Board.h"

// #define NUM_OF_ARRAY(x) (sizeof(x)/sizeof(x[0]))


static const int FONT_SIZE = 8;
static const int WINDOW_WIDTH = 320;
static const int WINDOW_HEIGHT = 240;

static const int BlockSize = 16;
static const int BoardOffsetX = 8;
static const int BoardOffsetY = 240 - 8 - BlockSize;

static const std::uint32_t SpawnDelayMs = 300;
static const std::uint32_t EraseCheckTimeMs = 10;
static const std::uint32_t EraseDurationMs = 300;
static const std::uint32_t DropTimeMs = 200;
static const std::uint32_t InitialAutoFallIntervalMs = 1000;

static const std::uint32_t ChainEffectDispTimeMs = 1500;
static const std::uint32_t BlockEraseEffectTimeMs = 1500;

//この個数を消すごとに落下速度が早くなる
static const std::uint32_t EraseCountForSpeedUp = 4;
//どのくらい早くなるか
static const std::uint32_t AutoFallDecreaseMs = 50;
//落下速度はここまで減る
static const std::uint32_t MinAutoFallIntervalMs = 100;

int CalcBoardX(int ix) { return BoardOffsetX + ix * BlockSize; }
int CalcBoardY(int iy) { return BoardOffsetY - iy * BlockSize; }



class InputManager
{
private:
    std::uint32_t mHeld = 0;//押されている間１
    std::uint32_t mPressed = 0;//押された瞬間１
    std::uint32_t mReleased = 0;//離した瞬間１
    std::uint32_t mRepeat = 0;//リピート

    std::array<std::uint32_t, static_cast<size_t>(Button::Max)> mSystemTimeMs{};
    std::array<std::uint32_t, static_cast<size_t>(Button::Max)> mRepeatCount{};

public:

    void Execute(std::uint32_t systemTimeMs, std::uint32_t held);

    // 押されている
    bool IsHeld(Button button)const {
        return (mHeld & ButtonBit(button)) != 0;
    }

    // 押された瞬間
    bool IsPressed(Button button) const {
        return (mPressed & ButtonBit(button)) != 0;
    }

    // 離された瞬間
    bool IsReleased(Button button) const {
        return (mReleased & ButtonBit(button)) != 0;
    }

    // //リピート
    bool IsRepeat(Button button) const {
        return (mRepeat & ButtonBit(button)) != 0;
    }
};

void InputManager::Execute(std::uint32_t systemTimeMs, std::uint32_t held)
{
    mPressed = 0;
    mRepeat = 0;
    mReleased = 0;

    for (std::size_t i = 0; i < static_cast<std::size_t>(Button::Max); i++)
    {
        Button b = static_cast<Button>(i);
        std::uint32_t f = ButtonBit(b);
        if (held & f) {
            if (!(mHeld & f)) {
                //前回おされてなくて、今回押されたとき
                mPressed |= f;
                mRepeat |= f;
                //押され続ける時間を計測開始
                mSystemTimeMs[i] = systemTimeMs;
                mRepeatCount[i] = 0;
            }
            //リピート入力の初回だけ多めに待つ
            std::uint32_t intervalTimeMs = 300;
            if (mRepeatCount[i] > 0)
            {
                intervalTimeMs = 100;
            }
            if ((systemTimeMs - mSystemTimeMs[i]) >= intervalTimeMs) {
                mRepeat |= f;
                mSystemTimeMs[i] = systemTimeMs;
                ++mRepeatCount[i];
            }
        }
        else {
            if (mHeld & f) {
                mReleased |= f;
            }
        }
    }

    mHeld = held;

}


//------------------------------------------
enum class GameEventType {
    Chain,
    BlockErased,
    GameOver,
};

struct GameEvent {

    GameEvent(GameEventType type, int x, int y, std::uint32_t chainCount, BlockColor blockColor)
        :mEventType(type), mX(x), mY(y), mChainCount(chainCount), mBlockColor(blockColor)
    {

    }

    GameEvent() = default;

    void Set(GameEventType type, int x, int y, std::uint32_t chainCount, BlockColor blockColor)
    {
        mEventType = type;
        mX = x;
        mY = y;
        mChainCount = chainCount;
        mBlockColor = blockColor;
    }

    GameEventType mEventType{};
    int mX = 0;
    int mY = 0;
    std::uint32_t mChainCount = 0;
    BlockColor mBlockColor = BlockColor::Empty;
};

class GameEventManager {
private:
    static constexpr std::size_t MaxEvents = 80;
    std::array<GameEvent, MaxEvents> mGameEvents;
    std::uint8_t mTop = 0;
    std::uint8_t mEnd = 0;
public:
    void Push(GameEventType type, int x, int y, std::uint32_t chainCount, BlockColor blockColor)
    {
        std::uint8_t next = mEnd + 1;
        if (next >= MaxEvents)
        {
            next = 0;
        }

        //次の書き込み先が先頭なら満杯
        assert(next != mTop);

        mGameEvents[mEnd].Set(type, x, y, chainCount, blockColor);
        mEnd = next;
    }

    bool IsEmpty() const
    {
        return mTop == mEnd;
    }
    GameEvent Pop()
    {
        assert(mTop != mEnd);

        GameEvent gameEvent = mGameEvents[mTop];
        mTop++;
        if (mTop >= MaxEvents)
        {
            mTop = 0;
        }
        return gameEvent;
    }
};

//------------------------------------------
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

public:
    void Start(std::uint32_t systemTimeMs, GameEventManager& gameEventManager)
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

    std::uint32_t GetChainCount() const { return mChainCount; }
    std::uint32_t GetScore()const { return mScore; }
    //スコア計算
    std::uint32_t CalcScore(std::uint32_t eraseBlockCount, std::uint32_t chainCount)const;

    void SetRandomSeed(std::uint32_t randomSeed)
    {
        mRandomSeed = (randomSeed != 0) ? randomSeed : 0xDEADBEEF;
    }

    std::uint32_t GenerateRandom()
    {
        mRandomSeed ^= mRandomSeed << 13;
        mRandomSeed ^= mRandomSeed >> 17;
        mRandomSeed ^= mRandomSeed << 5;
        return static_cast<std::uint32_t>(mRandomSeed);
    }

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
    GameResult GetGameResult() const { return mGameResult; }

    //落下時間計測のタイマーをリセット
    void ResetAutoFallTimer();

    //前回の落下から一定時間経過したか？
    bool IsAutoFallDue() const;

    BlockColor GenerateBlock();

    const FallingPair& GetFallingPair() const { return mFallingPair; }
    const FallingPair& GetNextPair() const { return mNextPair; }
    const Board& GetBoard()const { return mBoard; }

    std::uint32_t GetRandomSeed()const { return mRandomSeed; }

    std::uint32_t GetAutoFallIntervalMs()const { return mAutoFallIntervalMs; }
    std::uint32_t GetTotalEraseBlockCount()const { return mTotalEraseBlockCount; }
};

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
                            gameEventManager.Push(GameEventType::BlockErased, ix, iy, 0, color);

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
                gameEventManager.Push(GameEventType::Chain, x, y, mChainCount, BlockColor::Empty);
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

    if ((mSystemTimeMs - mEraseCheckTimeMs) >= EraseCheckTimeMs)
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
    if ((mSystemTimeMs - mDropStartTimeMs) >= DropTimeMs) // 落下処理終了？
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

//--------------------------------------

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

class Effect {
private:
    std::uint32_t mLifeTimeMs = 0;
    std::uint32_t mStartTimeMs = 0;
public:
    Effect(std::uint32_t lifeTimeMs, std::uint32_t startTimeMs) :mLifeTimeMs(lifeTimeMs), mStartTimeMs(startTimeMs) {

    }
    virtual ~Effect() = default;

    virtual void Execute(std::uint32_t) = 0;
    virtual void Draw(SDL_Renderer*) = 0;

    std::uint32_t GetLifeTimeMs()const { return mLifeTimeMs; }
    std::uint32_t GetStartTimeMs()const { return mStartTimeMs; }

    float CalcRate(std::uint32_t systemTimeMs) const {
        float nowMs = static_cast<float>(systemTimeMs) - static_cast<float>(mStartTimeMs);
        float rate = nowMs / static_cast<float>(mLifeTimeMs);
        if (rate < 0.0f) {
            return 0.0f;
        }
        if (rate > 1.0f) {
            return 1.0f;
        }
        return rate;
    }
};


class EffectManager {
private:
    std::vector<std::unique_ptr<Effect>> mEffects;
public:
    void AddEffect(std::unique_ptr<Effect> effect) {
        mEffects.push_back(std::move(effect));
    }

    void Execute(std::uint32_t systemTimeMs)
    {

        for (auto& e : mEffects) {
            e->Execute(systemTimeMs);
        }

        //終了処理
        auto it = std::remove_if(mEffects.begin(), mEffects.end(), [systemTimeMs](std::unique_ptr<Effect>& e)
            {
                return (systemTimeMs - e->GetStartTimeMs()) > e->GetLifeTimeMs();
            });
        mEffects.erase(it, mEffects.end());
    }

    void Draw(SDL_Renderer* renderer)const
    {
        for (const auto& e : mEffects) {
            e->Draw(renderer);
        }
    }
};


void DrawChar(SDL_Renderer* renderer, int x, int y, char c, int scale, std::uint32_t color)
{
    //char c = 'A';
    if (c >= 0x20 && c <= 0x5F) {
        const std::uint8_t* data = font8x8[c - 0x20];

        SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
        for (int ix = 0; ix < FONT_SIZE; ix++) {
            int px = x + ix * scale;
            for (int iy = 0; iy < FONT_SIZE; iy++) {
                if (data[iy] & BIT((FONT_SIZE - 1) - ix)) {
                    int py = y + iy * scale;
                    SDL_Rect rect{
                        px,py,scale,scale
                    };
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }
    }
}
void DrawString(SDL_Renderer* renderer, int x, int y, const std::string& str, int scale, std::uint32_t color, bool shadow)
{
    //フォントデータは大文字しか用意していないので変換
    std::string strUpper = str;
    for (char& c : strUpper) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }

    //影のためのアルファ
    std::uint32_t alpha = (color & 0xFF000000);

    if (shadow) {
        for (std::size_t i = 0; i < strUpper.size(); i++) {
            char c = strUpper[i];
            DrawChar(renderer, x + scale * 8 * static_cast<int>(i) + scale, y + scale, c, scale, alpha | 0x00000000);
        }
    }

    for (std::size_t i = 0; i < strUpper.size(); i++) {
        char c = strUpper[i];
        DrawChar(renderer, x + scale * 8 * static_cast<int>(i), y, c, scale, color);
    }
}

void DrawBlock(SDL_Renderer* renderer, int ix, int iy, BlockColor blockColor, std::uint32_t alpha)
{
    std::uint32_t color = 0x202020;
    switch (blockColor) {
    case BlockColor::Red: color = 0xFF0000; break;
    case BlockColor::Blue: color = 0x0000FF; break;
    case BlockColor::Green: color = 0x00FF00; break;
    case BlockColor::Yellow: color = 0xFFFF00; break;
    case BlockColor::Empty: color = 0x202020;break;
    }

    int x = CalcBoardX(ix);
    int y = CalcBoardY(iy);
    SDL_Rect rect{
        x,y,BlockSize - 2,BlockSize - 2
    };
    SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, static_cast<std::uint8_t>(alpha));
    SDL_RenderFillRect(renderer, &rect);
}

void DrawBoard(SDL_Renderer* renderer, const Board& board)
{
    for (int ix = 0; ix < WIDTH; ix++) {
        for (int iy = 0; iy < HEIGHT; iy++) {
            BlockColor blockColor = board.GetColor(ix, iy);
            DrawBlock(renderer, ix, iy, blockColor, 0xFF);
        }
    }
}
void DrawFallingPair(SDL_Renderer* renderer, const FallingPair& fallingPair)
{
    int ix = fallingPair.GetX();
    int iy = fallingPair.GetY();
    BlockColor blockColor = fallingPair.GetColor(0);

    DrawBlock(renderer, ix, iy, blockColor, 0xFF);

    //相方
    switch (fallingPair.GetDirection())
    {
    case Direction::Up: ++iy; break;
    case Direction::Down: --iy; break;
    case Direction::Left: --ix; break;
    case Direction::Right: ++ix; break;
    }
    BlockColor pairBlockColor = fallingPair.GetColor(1);
    DrawBlock(renderer, ix, iy, pairBlockColor, 0xFF);
}

void ClearScreen(SDL_Renderer* renderer)
{
    //画面クリア
    {
        SDL_SetRenderDrawColor(renderer, 0, 0x20, 0, 255);
        SDL_RenderClear(renderer);
    }
}

void DrawTitle(SDL_Renderer* renderer)
{
    {
        std::string str = "PUYO";
        int x = WINDOW_WIDTH / 2 - FONT_SIZE * static_cast<int>(str.size()) / 2;
        int y = WINDOW_HEIGHT / 2 - FONT_SIZE / 2;
        DrawString(renderer, x, y, str, 1, 0xFFFFFFFF, false);
    }

    {
        std::string str = "PRESS SPACE";
        int x = WINDOW_WIDTH / 2 - FONT_SIZE * static_cast<int>(str.size()) / 2;
        int y = WINDOW_HEIGHT / 2 + FONT_SIZE / 2 * 3;
        DrawString(renderer, x, y, str, 1, 0xFFFFFFFF, false);
    }
}
void DrawPlaying(SDL_Renderer* renderer, const GameManager& gameManager)
{
    //盤面
    DrawBoard(renderer, gameManager.GetBoard());

    //操作中
    if (gameManager.GetPlayState() == PlayState::Control)
    {
        DrawFallingPair(renderer, gameManager.GetFallingPair());
    }

    //Next
    {
        DrawString(renderer, 112, 10, "NEXT", 1, 0xFFFFFFFF, true);
        FallingPair pair = gameManager.GetNextPair();
        DrawBlock(renderer, WIDTH + 1, HEIGHT + 0, pair.GetColor(0), 0xFF);
        DrawBlock(renderer, WIDTH + 1, HEIGHT - 1, pair.GetColor(1), 0xFF);
    }

    //スコア
    {
        int x = 8;
        int y = 2;
        DrawString(renderer, x, y, "SCORE", 1, 0xFFFFFFFF, false);

        int score = gameManager.GetScore();
        std::string str = std::to_string(score);
        DrawString(renderer, x, y + FONT_SIZE, str, 1, 0xFFFFFFFF, false);
    }

    //消した総数
    {
        std::uint32_t total = gameManager.GetTotalEraseBlockCount();
        int x = 112;
        int y = 120;
        DrawString(renderer, x, y, "TOTAL", 1, 0xFFFFFFFF, false);

        std::string str = std::to_string(total);
        DrawString(renderer, x, y + FONT_SIZE, str, 1, 0xFFFFFFFF, false);
    }

    //落下速度
    {
        std::uint32_t total = gameManager.GetAutoFallIntervalMs();
        int x = 112;
        int y = 144;
        DrawString(renderer, x, y, "INTERVAL", 1, 0xFFFFFFFF, false);

        std::string str = std::to_string(total);
        DrawString(renderer, x, y + FONT_SIZE, str, 1, 0xFFFFFFFF, false);
    }

}
void DrawGameOver(SDL_Renderer* renderer)
{
    {
        std::string str = "GAME OVER";
        int x = WINDOW_WIDTH / 2 - FONT_SIZE * static_cast<int>(str.size()) / 2;
        int y = WINDOW_HEIGHT / 2 - FONT_SIZE / 2;
        DrawString(renderer, x, y, str, 1, 0xFFFFFFFF, false);
    }

    {
        std::string str = "PRESS SPACE";
        int x = WINDOW_WIDTH / 2 - FONT_SIZE * static_cast<int>(str.size()) / 2;
        int y = WINDOW_HEIGHT / 2 + FONT_SIZE / 2 * 3;
        DrawString(renderer, x, y, str, 1, 0xFFFFFFFF, false);
    }

}
void DrawClear(SDL_Renderer* renderer)
{
    std::string str = "CLEAR";
    int x = WINDOW_WIDTH / 2 - FONT_SIZE * static_cast<int>(str.size()) / 2;
    int y = WINDOW_HEIGHT / 2 - FONT_SIZE / 2;
    DrawString(renderer, x, y, str, 1, 0xFFFFFFFF, false);

}

void DrawScreen(SDL_Renderer* renderer, const GameManager& gameManager, const EffectManager& effectManager)
{
    switch (gameManager.GetState()) {
    case GameState::Title:
        {
            DrawTitle(renderer);
            break;
        }
    case GameState::Playing:
        {
            DrawPlaying(renderer, gameManager);
            break;
        }
    case GameState::GameOver:
        {
            DrawGameOver(renderer);
            break;
        }
    case GameState::GameClear:
        {
            DrawClear(renderer);
            break;
        }
    }

    effectManager.Draw(renderer);

    SDL_RenderPresent(renderer);
}
//ブロック消去エフェクト
class EffectBlockErase : public Effect
{
private:
    int mX = 0;
    int mY = 0;
    BlockColor mBlockColor;
    std::uint32_t mAlpha = 0;
    std::uint32_t mColor = 0;
    int mSize = 0;
public:
    EffectBlockErase(int x, int y, BlockColor blockColor, std::uint32_t lifeTimeMs, std::uint32_t startTimeMs)
        :Effect(lifeTimeMs, startTimeMs), mX(x), mY(y), mBlockColor(blockColor)
    {
        mAlpha = 255;
        mSize = 0;
        switch (mBlockColor) {
        case BlockColor::Red: mColor = 0xFF0000; break;
        case BlockColor::Blue: mColor = 0x0000FF; break;
        case BlockColor::Yellow: mColor = 0xFFFF00; break;
        case BlockColor::Green: mColor = 0x00FF00; break;
        case BlockColor::Empty: mColor = 0x202020; break;
        }
    }

    void Execute(std::uint32_t systemTimeMs) override {
        float rate = CalcRate(systemTimeMs);
        float alpha = 255.0f - 255.0f * rate;
        mAlpha = static_cast<std::uint32_t>(alpha) & 0xFF;
        mSize = BlockSize - static_cast<int>(static_cast<float>(BlockSize) * rate);
    }
    void Draw(SDL_Renderer* renderer) override
    {
        SDL_SetRenderDrawColor(renderer, (mColor >> 16) & 0xFF, (mColor >> 8) & 0xFF, mColor & 0xFF, static_cast<std::uint8_t>(mAlpha));
        int x = mX - mSize / 2;
        int y = mY - mSize / 2;
        SDL_Rect rect{
            x,y,mSize,mSize
        };
        SDL_RenderFillRect(renderer, &rect);
    }
};
//連鎖数エフェクト
class EffectChain : public Effect
{
private:
    int mX = 0;
    int mY = 0;
    int mOffsetY = 0;
    std::uint32_t mAlpha = 255;
    std::uint32_t mColor = 0xFFFFFF;
    int mChainNum = 0;
public:
    EffectChain(int x, int y, std::uint32_t lifeTimeMs, std::uint32_t startTimeMs, int chainNum)
        : Effect(lifeTimeMs, startTimeMs), mX(x), mY(y), mChainNum(chainNum) {
        mOffsetY = 0;
        mAlpha = 255;
        mColor = 0xFFFFFF;
    }
    void Execute(std::uint32_t systemTimeMs) override {
        float rate = CalcRate(systemTimeMs);
        mAlpha = static_cast<std::uint32_t>(255 - 255 * rate) & 0xFF;
        mOffsetY = static_cast<int>(32 - 32 * rate);
        static const std::array<std::uint32_t, 5> colorTbl = {
            0xFF0000,
            0xFFFF00,
            0xFFFFFF,
            0x00FF00,
            0xFF00FF,
        };
        int index = 0;
        if (rate < 0.2f) {
            index = 0;
        }
        else if (rate < 0.4f) {
            index = 1;
        }
        else if (rate < 0.6f) {
            index = 2;
        }
        else if (rate < 0.8f) {
            index = 3;
        }
        else {
            index = 4;
        }
        assert(index < colorTbl.size());
        mColor = colorTbl[index];

    }
    void Draw(SDL_Renderer* renderer) override {
        std::string str = std::to_string(mChainNum) + " CHAIN";
        DrawString(renderer, mX, mY + mOffsetY, str, 1, ((mAlpha & 0xFF) << 24) | mColor, false);
    }
};

int main(int, char* [])
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << SDL_GetError() << '\n';
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "cpp-puyo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (window == nullptr)
    {
        std::cout << "SDL_CreateWindow Error : " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr)
    {
        std::cout << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    GameManager gameManager;
    EffectManager effectManager;

    auto startTime = std::chrono::steady_clock::now();
    {
        auto now = std::chrono::steady_clock::now();

        std::uint32_t randomSeed = static_cast<std::uint32_t>(now.time_since_epoch().count());
        gameManager.SetRandomSeed(randomSeed);
    }

    gameManager.SetState(GameState::Title);

    bool running = true;

    int frameCount = 0;
    int count = 0;
    std::uint32_t frameCountSystemTimeMs = 0;

    while (true) {

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }

            if (!running) {
                break;
            }
        }

        if (!running) {
            break;
        }

        //今押されているか？
        std::uint32_t held = 0;
        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        if (keyState[SDL_SCANCODE_LEFT]) {
            held |= ButtonBit(Button::Left);
        }
        if (keyState[SDL_SCANCODE_RIGHT]) {
            held |= ButtonBit(Button::Right);
        }
        if (keyState[SDL_SCANCODE_DOWN]) {
            held |= ButtonBit(Button::Down);
        }
        if (keyState[SDL_SCANCODE_SPACE]) {
            held |= ButtonBit(Button::Rotate);
        }

        if (keyState[SDL_SCANCODE_ESCAPE]) {
            break;
        }

        ClearScreen(renderer);

        //FPS
        {
            std::ostringstream oss;
            oss << "FPS " << frameCount;
            std::string str = oss.str();
            DrawString(renderer, 240, 2, str, 1, 0xFFFFFFFF, false);
        }

        {
            PlayState state = gameManager.GetPlayState();
            std::string str;
            switch (state) {
            case PlayState::Control: str += "Control";break;
            case PlayState::DropBlocks: str += "DropBlocks";break;
            case PlayState::Erase: str += "Erase";break;
            case PlayState::EraseCheck: str += "EraseCheck";break;
            case PlayState::Result: str += "Result";break;
            case PlayState::Spawn: str += "Spawn";break;
            }
            DrawString(renderer, 80, 2, str, 1, 0xFFFFFFFF, false);
        }

        auto now = std::chrono::steady_clock::now();

        std::uint32_t systemTimeMs = static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count());

        if ((systemTimeMs - frameCountSystemTimeMs) >= 1000) {
            frameCount = count;
            count = 0;
            frameCountSystemTimeMs = systemTimeMs;
        }

        count++;

        gameManager.Execute(systemTimeMs, held);

        //GameEvent処理
        {
            while (!gameManager.IsGameEventEmpty()) {
                auto ge = gameManager.PopGameEvent();
                switch (ge.mEventType) {
                case GameEventType::Chain:
                    {
                        int x = CalcBoardX(ge.mX);
                        int y = CalcBoardY(ge.mY);
                        std::unique_ptr<EffectChain> effectChain = std::make_unique<EffectChain>(x, y, ChainEffectDispTimeMs, systemTimeMs, ge.mChainCount);
                        effectManager.AddEffect(std::move(effectChain));
                        break;
                    }
                case GameEventType::BlockErased:
                    {
                        int x = CalcBoardX(ge.mX) + BlockSize / 2;
                        int y = CalcBoardY(ge.mY) + BlockSize / 2;
                        std::unique_ptr<EffectBlockErase> effectEraseBlock = std::make_unique<EffectBlockErase>(x, y, ge.mBlockColor, BlockEraseEffectTimeMs, systemTimeMs);
                        effectManager.AddEffect(std::move(effectEraseBlock));
                        break;
                    }
                case GameEventType::GameOver:
                    {
                        break;
                    }
                }
            }
        }

        effectManager.Execute(systemTimeMs);

        DrawScreen(renderer, gameManager, effectManager);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;

}

