#include <iostream>
#include <sstream>
#include <cassert>
#include <array>
#include <chrono>
#include <iomanip>
#include <functional>
#include <vector>
#include <queue>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

#include <SDL.h>

#include "font8x8.h"

#define BIT(x) (1u << (x))

#define NUM_OF_ARRAY(x) ((sizeof(x)/sizeof((x)[0])))

// 盤面のサイズ
static constexpr int HEIGHT = 12;
static constexpr int WIDTH = 6;

static const int FONT_SIZE = 8;
static const int WINDOW_WIDTH = 320;
static const int WINDOW_HEIGHT = 240;

static const int BlockSize = 16;
static const int BoardOffsetX = 8;
static const int BoardOffsetY = 240 - 8 - BlockSize;

static const std::uint32_t SpawnTimeMs = 300;
static const std::uint32_t EraseCheckTimeMs = 10;
static const std::uint32_t EraseTimeMs = 300;
static const std::uint32_t DropTimeMs = 200;
static const std::uint32_t InitialAutoFallIntervalMs = 1000;

int CalcBoardX(int ix) { return BoardOffsetX + ix * BlockSize; }
int CalcBoardY(int iy) { return BoardOffsetY - iy * BlockSize; }


enum class GameState
{
    Title,     // タイトル画面
    Playing,   // ゲーム中
    GameOver,  // ゲームオーバー
    GameClear, // ゲームクリア
};

enum class PlayState
{
    Spawn,      // 新しいブロック出現
    Control,    // ブロックを操作中
    EraseCheck, // 消えるかチェック
    Erase,      // 消去中
    DropBlocks, // 落下中
    Result,     // 結果表示中
};

enum class GameResult
{
    Playing,
    GameOver,
    GameClear,
};

enum class BlockColor
{
    Empty, // 無し
    Red,
    Blue,
    Green,
    Yellow,
};

// 相方のブロックの位置
enum class Direction
{
    Up,
    Right,
    Down,
    Left,
};

//ボタン番号
enum class Button
{
    Rotate = 0,
    Left,
    Right,
    Down,
    Max
};
//ボタン番号をビットに変換
std::uint32_t ButtonBit(Button button)
{
    return 1u << static_cast<ustd::int32_t>(button);
}

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

// 操作中のブロック
class FallingPair
{
private:
    int mX; // 左端を0 右端をWIDTH-1とする
    int mY; // 一番上の段をHEIGHT-1、一番下の段を0とする
    Direction mDirection;
    std::array<BlockColor, 2> mColors;

public:
    void Reset()
    {
        mX = 0;
        mY = HEIGHT - 2;
        mDirection = Direction::Up;
        for (auto& c : mColors)
        {
            c = BlockColor::Empty;
        }
    }
    int GetX() const
    {
        return mX;
    }
    int GetY() const
    {
        return mY;
    }
    void SetDirection(Direction next)
    {
        mDirection = next;
    }
    Direction GetDirection() const
    {
        return mDirection;
    }
    void SetColor(std::size_t index, BlockColor color)
    {
        assert((index < 2));
        mColors[index] = color;
    }
    BlockColor GetColor(std::size_t index) const
    {
        return mColors[index];
    }

    void SetPosition(int x, int y)
    {
        mX = x;
        mY = y;
    }

    void Move(int dx, int dy)
    {
        mX += dx;
        mY += dy;
    }

    void Rotate()
    {
        switch (mDirection)
        {
        case Direction::Down:
            mDirection = Direction::Left;
            break;
        case Direction::Left:
            mDirection = Direction::Up;
            break;
        case Direction::Up:
            mDirection = Direction::Right;
            break;
        case Direction::Right:
            mDirection = Direction::Down;
            break;
        }
    }
};

struct EraseFlags
{
    std::uint32_t mFlags[WIDTH];
    void Init()
    {
        for (auto& f : mFlags)
        {
            f = 0;
        }
    }
    void On(int x, int y)
    {
        mFlags[x] |= BIT(y);
    }
    void Off(int x, int y)
    {
        mFlags[x] &= ~BIT(y);
    }
    bool IsOn(int x, int y) const
    {
        return static_cast<bool>(mFlags[x] & BIT(y));
    }
};

// 盤面
class Board
{
private:
    // 左端を0 右端をWIDTH-1
    // 一番上の段をHEIGHT-1 一番下の段を0とする
    BlockColor mCells[HEIGHT][WIDTH];

    //消えるブロックにフラグを立てる
    EraseFlags mEraseFlags;

public:
    Board()
    {
        Reset();
    }

    void Reset()
    {
        for (auto& row : mCells)
        {
            for (auto& c : row)
            {
                c = BlockColor::Empty;
            }
        }
        mEraseFlags.Init();
    }

    //ペアを配置できる？
    bool CanPlace(const FallingPair& pair) const;

    //ペアを配置
    void PutPair(const FallingPair& pair);

    //1ブロック配置
    void PutOne(int x, int y, BlockColor color);

    //浮いているブロックがあるか？
    bool HasFloatingBlocks() const;

    //浮いているブロックを1段下げる
    void DropFloatingBlocks();

    //消えるブロックにフラグを立てる
    bool FindEraseBlocks();
    void SearchConnectedBlocks(int x, int y, BlockColor color, EraseFlags& checkedFlags, EraseFlags& connectedFlags, int& count);

    int CountEraseBlocks()const;

    //mEraseFlagsに従ってブロックを消す
    void EraseBlocks();

    void Print();

    BlockColor GetColor(int x, int y)const { return mCells[y][x]; }

    //指定のブロックは消えるフラグが立っている？
    bool IsEraseBlock(int x, int y)const;

    //真ん中あたりの消えるブロックの位置を取得する
    void SearchCenterEraseBlock(int& x, int& y);
};
//1ブロック配置
void Board::PutOne(int x, int y, BlockColor color)
{
    assert((x >= 0) && (x < WIDTH) && (y >= 0) && (y < HEIGHT));
    mCells[y][x] = color;
}

//ペアを配置
void Board::PutPair(const FallingPair& pair)
{
    int x = pair.GetX();
    int y = pair.GetY();
    //軸を配置
    mCells[y][x] = pair.GetColor(0);

    //相方を配置
    switch (pair.GetDirection())
    {
    case Direction::Up:    y++;  break;
    case Direction::Down:  y--;  break;
    case Direction::Left:  x--;  break;
    case Direction::Right: x++;  break;
    }
    mCells[y][x] = pair.GetColor(1);
}
//配置できる？
bool Board::CanPlace(const FallingPair& pair) const
{
    // 軸のブロック
    int x = pair.GetX();
    int y = pair.GetY();

    // 軸のブロックが範囲外？
    if ((x < 0) || (x >= WIDTH) || (y < 0) || (y >= HEIGHT))
    {
        return false;
    }
    // 軸のブロックの位置が空いてない？
    if (mCells[y][x] != BlockColor::Empty)
    {
        return false;
    }

    // 相方のブロックの位置
    switch (pair.GetDirection())
    {
    case Direction::Down:
        y -= 1;
        break;
    case Direction::Left:
        x -= 1;
        break;
    case Direction::Up:
        y += 1;
        break;
    case Direction::Right:
        x += 1;
        break;
    }
    // 相方のブロックが範囲外？
    if ((x < 0) || (x >= WIDTH) || (y < 0) || (y >= HEIGHT))
    {
        return false;
    }

    // 相方のブロックの位置が空いない？
    if (mCells[y][x] != BlockColor::Empty)
    {
        return false;
    }

    return true;
}
//浮いているブロックがあるか？
bool Board::HasFloatingBlocks() const
{
    for (int x = 0; x < WIDTH; x++)
    {
        for (int y = 0; y < HEIGHT - 1; y++)
        {
            if (mCells[y][x] == BlockColor::Empty) {
                for (int yy = y + 1; yy < HEIGHT; yy++)
                {
                    if (mCells[yy][x] != BlockColor::Empty)
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}
//浮いているブロックを1段下げる
void Board::DropFloatingBlocks()
{
    for (int x = 0; x < WIDTH; x++)
    {
        for (int y = 1; y < HEIGHT; y++)
        {
            if (mCells[y - 1][x] == BlockColor::Empty)//一つ下が空いてる?
            {
                if (mCells[y][x] != BlockColor::Empty)//ブロックがある？
                {
                    //一つ下に移動
                    mCells[y - 1][x] = mCells[y][x];
                    mCells[y][x] = BlockColor::Empty;
                }
            }
        }
    }
}
//消えるブロックにフラグを立てる
//再帰を使って４つ以上つながった同色のブロックの位置のフラグを立てる
bool Board::FindEraseBlocks()
{
    bool hasErase = false;

    mEraseFlags.Init();

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if ((mCells[y][x] != BlockColor::Empty) && !mEraseFlags.IsOn(x, y)) {

                EraseFlags checkedFlags;
                checkedFlags.Init();
                checkedFlags.On(x, y);

                EraseFlags connectedFlags;
                connectedFlags.Init();
                connectedFlags.On(x, y);

                int count = 1;//つながった数

                SearchConnectedBlocks(x, y, mCells[y][x], checkedFlags, connectedFlags, count);

                if (count >= 4) {
                    //消えるものにフラグを立てる
                    for (int yy = 0; yy < HEIGHT; yy++) {
                        for (int xx = 0; xx < WIDTH; xx++) {
                            if (connectedFlags.IsOn(xx, yy)) {
                                mEraseFlags.On(xx, yy);
                            }
                        }
                    }
                    hasErase = true;

                }
            }
        }
    }

    return hasErase;
}
//指定位置から
// 同じ色でつながっているブロックを
// 再帰でたどって
// 個数を数え
// その位置を EraseFlags に記録する
void Board::SearchConnectedBlocks(int x, int y, BlockColor color, EraseFlags& checkedFlags, EraseFlags& connectedFlags, int& count)
{
    //上
    if ((y + 1) < HEIGHT) {
        if (!checkedFlags.IsOn(x, y + 1)) {//未チェック？
            checkedFlags.On(x, y + 1);
            if (mCells[y + 1][x] == color) {
                ++count;
                connectedFlags.On(x, y + 1);
                SearchConnectedBlocks(x, y + 1, color, checkedFlags, connectedFlags, count);
            }
        }
    }
    //下
    if ((y - 1) >= 0) {
        if (!checkedFlags.IsOn(x, y - 1)) {//未チェック？
            checkedFlags.On(x, y - 1);
            if (mCells[y - 1][x] == color) {
                ++count;
                connectedFlags.On(x, y - 1);
                SearchConnectedBlocks(x, y - 1, color, checkedFlags, connectedFlags, count);
            }
        }
    }
    //左
    if ((x - 1) >= 0) {
        if (!checkedFlags.IsOn(x - 1, y)) {//未チェック？
            checkedFlags.On(x - 1, y);
            if (mCells[y][x - 1] == color) {
                ++count;
                connectedFlags.On(x - 1, y);
                SearchConnectedBlocks(x - 1, y, color, checkedFlags, connectedFlags, count);
            }
        }
    }
    //右
    if ((x + 1) < WIDTH) {
        if (!checkedFlags.IsOn(x + 1, y)) {//未チェック？
            checkedFlags.On(x + 1, y);
            if (mCells[y][x + 1] == color) {
                ++count;
                connectedFlags.On(x + 1, y);
                SearchConnectedBlocks(x + 1, y, color, checkedFlags, connectedFlags, count);
            }
        }
    }
}

//mEraseFlagsに従ってブロックを消す
void Board::EraseBlocks()
{
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (mEraseFlags.IsOn(x, y)) {
                mCells[y][x] = BlockColor::Empty;
            }
        }
    }
}
//指定のブロックは消えるフラグが立っている？
bool Board::IsEraseBlock(int x, int y)const
{
    return mEraseFlags.IsOn(x, y);
}
//真ん中あたりの消えるボロックの位置を取得する
void Board::SearchCenterEraseBlock(int& x, int& y)
{
    int minX = WIDTH;
    int maxX = 0;
    int minY = HEIGHT;
    int maxY = 0;
    for (int iy = 0; iy < HEIGHT; iy++) {
        for (int ix = 0; ix < WIDTH; ix++) {
            if (mEraseFlags.IsOn(ix, iy)) {
                if (ix < minX) minX = ix;
                if (ix > maxX) maxX = ix;
                if (iy < minY) minY = iy;
                if (iy > maxY) maxY = iy;
            }
        }
    }
    x = (maxX + minX) / 2;
    y = (maxY + minY) / 2;
}

int Board::CountEraseBlocks()const
{
    int count = 0;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (mEraseFlags.IsOn(x, y)) {
                count++;
            }
        }
    }
    return count;
}

void Board::Print()
{
    for (int y = HEIGHT - 1; y >= 0; y--) {
        for (int x = 0; x < WIDTH; x++) {
            char c = '.';
            switch (mCells[y][x]) {
            case BlockColor::Red: c = 'r'; break;
            case BlockColor::Blue: c = 'b'; break;
            case BlockColor::Yellow: c = 'y'; break;
            case BlockColor::Green: c = 'g'; break;
            case BlockColor::Empty: c = '.'; break;
            }
            std::cout << c;
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}
//------------------------------------------
enum class GameEventType {
    Chain,
    BlockErased,
    GameOver,
};

struct GameEvent {
    GameEvent(GameEventType type, int x, int y, int chainCount, BlockColor blockColor)
        :mEventType(type), mX(x), mY(y), mChainCount(chainCount), mBlockColor(blockColor) {

    }
    GameEventType mEventType;
    int mX;
    int mY;
    int mChainCount;
    BlockColor mBlockColor;
};

class GameEventManager {
private:
    std::queue<GameEvent> mGameEvents;
public:
    void Push(GameEventType type, int x, int y, int chainCount, BlockColor blockColor) {
        mGameEvents.emplace(type, x, y, chainCount, blockColor);
    }
    bool IsEmpty() const {
        return mGameEvents.empty();
    }
    GameEvent Pop() {
        GameEvent gameEvent = mGameEvents.front();
        mGameEvents.pop();
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
    int mChainCount = 0;
    int mScore = 0;
    GameResult mGameResult = GameResult::Playing;

    std::uint32_t mRandomSeed = 0;

    //この時間経過すると操作中のブロックが１マス下に落ちる(ミリ秒)
    std::uint32_t mAutoFallIntervalMs = 0;

    //現在の時間を保持し、mAutoFallIntervalMs分時間が経過したらリセット(ミリ秒)
    std::uint32_t mLastAutoFallTimeMs = 0;

    //落下開始時の時間
    std::uint32_t mDropStartTimeMs = 0;

    std::uint32_t mSpawnTimeMs = 0;

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

        mAutoFallIntervalMs = InitialAutoFallIntervalMs;//開始時はInitialAutoFallIntervalMsミリ秒ごとに１マス落下。ブロックを消していくと徐々に少なくする

        mDropStartTimeMs = 0;

        mEraseCheckTimeMs = 0;

        mEraseStartTimeMs = 0;

        mSpawnTimeMs = 0;

        mSystemTimeMs = systemTimeMs;

        GenerateNextPair();

        SetState(PlayState::Spawn, gameEventManager);
    }

    int GetChainCount() const { return mChainCount; }
    int GetScore()const { return mScore; }
    int CalcScore()const;

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

            mSpawnTimeMs = mSystemTimeMs;
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
    if ((mSystemTimeMs - mSpawnTimeMs) >= SpawnTimeMs) // ブロック追加演出完了？
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
            SetState(PlayState::DropBlocks, gameEventManager);
            mBoard.PutPair(mFallingPair);
            return;
        }
    }

    if (input.IsPressed(Button::Rotate)) {
        // 回転処理
        FallingPair pair = mFallingPair;

        // 90度
        pair.Rotate();
        if (mBoard.CanPlace(pair)) {
            mFallingPair = pair;
        }
        else {
            // 180度
            pair.Rotate();
            if (mBoard.CanPlace(pair))
            {
                mFallingPair = pair;
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
            SetState(PlayState::DropBlocks, gameEventManager);

            mBoard.PutPair(mFallingPair);
        }
    }
}

// 消去チェック
void PlayingManager::EraseCheck(GameEventManager& gameEventManager) {

    if ((mSystemTimeMs - mEraseCheckTimeMs) >= EraseCheckTimeMs) {
        if (mBoard.FindEraseBlocks()) {// 消える？
            SetState(PlayState::Erase, gameEventManager);
        }
        else {
            SetState(PlayState::Spawn, gameEventManager);
        }
    }
}
int PlayingManager::CalcScore() const
{
    return mBoard.CountEraseBlocks() * mChainCount;
}
// ブロックを消去
void PlayingManager::Erase(GameEventManager& gameEventManager)
{
    if ((mSystemTimeMs - mEraseStartTimeMs) >= EraseTimeMs) // 消去処理終了？
    {
        //スコア計算
        mScore += CalcScore();

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

        if (!mBoard.HasFloatingBlocks())//浮いているブロックがある？
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
            c = c - 'a' + 'A';
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
    int mCount = 0;
    int mChainNum = 0;
public:
    EffectChain(int x, int y, std::uint32_t lifeTimeMs, std::uint32_t startTimeMs, int chainNum)
        : Effect(lifeTimeMs, startTimeMs), mX(x), mY(y), mChainNum(chainNum) {
        mOffsetY = 0;
        mAlpha = 255;
        mColor = 0xFFFFFF;
        mCount = 0;
    }
    void Execute(std::uint32_t systemTimeMs) override {
        float rate = CalcRate(systemTimeMs);
        mAlpha = static_cast<std::uint32_t>(255 - 255 * rate) & 0xFF;
        mOffsetY = static_cast<int>(32 - 32 * rate);
        static const std::uint32_t colorTbl[] = {
                0xFF0000,
                0xFFFF00,
                0xFFFFFF,
                0x00FF00,
                0xFF00FF,
                0x0000FF,
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
        mColor = colorTbl[index];

    }
    void Draw(SDL_Renderer* renderer) override {
        std::string str = std::to_string(mChainNum) + " CHAIN";
        DrawString(renderer, mX, mY + mOffsetY, str, 1, ((mAlpha & 0xFF) << 24) | mColor, false);
    }
};

int main(int argc, char* argv[])
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
        320,
        240,
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
                        std::unique_ptr<EffectChain> effectChain = std::make_unique<EffectChain>(x, y, 1500, systemTimeMs, ge.mChainCount);
                        effectManager.AddEffect(std::move(effectChain));
                        break;
                    }
                case GameEventType::BlockErased:
                    {
                        int x = CalcBoardX(ge.mX) + BlockSize / 2;
                        int y = CalcBoardY(ge.mY) + BlockSize / 2;
                        std::unique_ptr<EffectBlockErase> effectEraseBlock = std::make_unique<EffectBlockErase>(x, y, ge.mBlockColor, 1500, systemTimeMs);
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

