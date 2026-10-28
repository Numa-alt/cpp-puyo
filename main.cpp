#include <iostream>
#include <sstream>
#include <cassert>
#include <array>
#include <chrono>
#include <iomanip>

#include <SDL.h>

#include "font8x8.h"

#define BIT(x) (1 << (x))

// 盤面のサイズ
static constexpr int HEIGHT = 12;
static constexpr int WIDTH = 6;

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
unsigned int ButtonBit(Button button)
{
    return 1u << static_cast<unsigned int>(button);
}

class InputManager
{
private:
    unsigned int mHeld = 0;//押されている間１
    unsigned int mPressed = 0;//押された瞬間１
    unsigned int mReleased = 0;//離した瞬間１
    unsigned int mRepeat = 0;//リピート

    std::array<unsigned long, static_cast<size_t>(Button::Max)> mSystemTimeMs;
    std::array<unsigned int, static_cast<size_t>(Button::Max)> mRepeatCount;

public:

    void Execute(unsigned long systemTimeMs, unsigned int held);

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

void InputManager::Execute(unsigned long systemTimeMs, unsigned int held)
{
    mPressed = 0;
    mRepeat = 0;
    mReleased = 0;

    for (int i = 0; i < static_cast<int>(Button::Max); i++)
    {
        Button b = static_cast<Button>(i);
        unsigned int f = ButtonBit(b);
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
            unsigned long intervalTimeMs = 300;
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
    void SetColor(int index, BlockColor color)
    {
        assert((index >= 0) && (index < 2));
        mColors[index] = color;
    }
    BlockColor GetColor(int index) const
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
    unsigned int mFlags[WIDTH];
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
    // 一番上の段をHEGHT-1 一番下の段を0とする
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

    //mEraseFlagsに従ってブロックを消す
    void EraseBlocks();

    void Print();

    BlockColor GetColor(int x, int y)const { return mCells[y][x]; }
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
            if (mCells[y][x] != BlockColor::Empty) {

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
// その位置を e に記録する
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
class PlayingManager
{
private:
    FallingPair mFallingPair; // 操作中のブロック
    FallingPair mNextPair;    // 次に出現するブロック
    Board mBoard;
    PlayState mState = PlayState::Spawn;
    int mChainCount = 0;
    GameResult mGameResult = GameResult::Playing;

    //この時間経過すると操作中のブロックが１マス下に落ちる(ミリ秒)
    unsigned long mAutoFallIntervalMs = 0;

    //現在の時間を保持し、mAutoFallIntervalMs分時間が経過したらリセット(ミリ秒)
    unsigned long mLastAutoFallTimeMs = 0;

    //落下開始時の時間
    unsigned long mDropStartTimeMs = 0;

    unsigned long mSpawnTimeMs = 0;

    //
    unsigned long mEraseCheckTimeMs = 0;

    //
    unsigned long mEraseStartTimeMs = 0;

    //
    unsigned long mSystemTimeMs = 0;

public:
    void Start()
    {
        mChainCount = 0;
        mGameResult = GameResult::Playing;
        mBoard.Reset();
        mFallingPair.Reset();
        mNextPair.Reset();

        mAutoFallIntervalMs = 1000;//開始時は2000ミリ秒ごとに１マス落下。ブロックを消していくと徐々に少なくする

        mDropStartTimeMs = 0;

        mEraseCheckTimeMs = 0;

        mEraseStartTimeMs = 0;

        mSpawnTimeMs = 0;

        mSystemTimeMs = 0;

        GenerateNextPair();

        SetState(PlayState::Spawn);
    }

    void GenerateNextPair();

    // ステータス変更時に行う処理
    void SetState(PlayState next);

    void Execute(const InputManager& input, unsigned long systemTimerMs);

    void Spawn();//新しいブロック出現
    void Control(const InputManager& input);
    void DropBlocks();
    void EraseCheck();
    void Erase();

    // GameManager側から結果を取得するため
    GameResult GetGameResult() const { return mGameResult; }

    //落下時間計測のタイマーをリセット
    void ResetAutoFallTimer();

    //前回の落下から一定時間経過したか？
    bool IsAutoFallDue() const;

    const FallingPair& GetFallingPair() const { return mFallingPair; }
    const FallingPair& GetNextPair() const { return mNextPair; }
    const Board& GetBoard()const { return mBoard; }

    PlayState GetState()const { return mState; }
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

void PlayingManager::GenerateNextPair()
{
    // 出現位置
    mNextPair.SetPosition(2, HEIGHT - 2);
    // ランダムに色を設定する
    mNextPair.SetColor(0, BlockColor::Blue);
    mNextPair.SetColor(1, BlockColor::Red);
}

// ステータス変更時に行う処理
void PlayingManager::SetState(PlayState next)
{

    switch (next) {
    case PlayState::Control: std::cout << "Control\n"; break;
    case PlayState::DropBlocks: std::cout << "DropBlocks\n";break;
    case PlayState::Erase: std::cout << "Erase\n";break;
    case PlayState::EraseCheck:std::cout << "EraseCheck\n";break;
    case PlayState::Result: std::cout << "Result\n"; break;
    case PlayState::Spawn: std::cout << "Spawn\n";break;
    }

    // mBoard.Print();

    mState = next;

    switch (next)
    {
    case PlayState::Spawn:
        {
            mFallingPair = mNextPair;

            if (!mBoard.CanPlace(mFallingPair)) // 追加できる場所がない?
            {
                mGameResult = GameResult::GameOver;
                SetState(PlayState::Result);
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
            break;
        }
    case PlayState::Result:
        break;
    }
}

void PlayingManager::Execute(const InputManager& input, unsigned long systemTimeMs)
{
    mSystemTimeMs = systemTimeMs;
    switch (mState)
    {
        // 新規ブロック追加
    case PlayState::Spawn:
        Spawn();
        break;

        // 操作中
    case PlayState::Control:
        Control(input);
        break;

        // 落下中
    case PlayState::DropBlocks:
        DropBlocks();
        break;

        // 消えるかチェック
    case PlayState::EraseCheck:
        EraseCheck();
        break;

        // 消去
    case PlayState::Erase:
        Erase();
        break;

        // 結果表示中
    case PlayState::Result:
        break;
    }
}
// 新しいブロック出現
void PlayingManager::Spawn()
{
    if ((mSystemTimeMs - mSpawnTimeMs) >= 500) // ブロック追加演出完了？
    {
        SetState(PlayState::Control);
    }
}
// 落下中のブロックの操作
void PlayingManager::Control(const InputManager& input)
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
    }

    if (input.IsPressed(Button::Rotate)) {
        // 回転処理
        FallingPair pair = mFallingPair;

        std::cout << "rotate!\n";

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
            SetState(PlayState::DropBlocks);

            mBoard.PutPair(mFallingPair);
        }
    }
}

// 消去チェック
void PlayingManager::EraseCheck() {

    if ((mSystemTimeMs - mEraseCheckTimeMs) >= 500) {
        if (mBoard.FindEraseBlocks()) {// 消える？
            SetState(PlayState::Erase);
        }
        else {
            SetState(PlayState::Spawn);
        }
    }
}

// ブロックを消去
void PlayingManager::Erase()
{
    if ((mSystemTimeMs - mEraseStartTimeMs) >= 500) // 消去処理終了？
    {
        //ブロックを消す
        mBoard.EraseBlocks();

        if (mBoard.HasFloatingBlocks()) // 浮いているブロックがある？
        {
            SetState(PlayState::DropBlocks); //
        }
        else
        {
            SetState(PlayState::Spawn);
        }
    }
}

// ブロック落下処理
void PlayingManager::DropBlocks()
{
    if ((mSystemTimeMs - mDropStartTimeMs) >= 200) // 落下処理終了？
    {
        mBoard.DropFloatingBlocks();

        if (!mBoard.HasFloatingBlocks())//浮いているブロックがある？
        {
            //浮いているブロックがないので消えるブロックがあるかチェック
            SetState(PlayState::EraseCheck);
        }
        else {
            //再度落下処理
            SetState(PlayState::DropBlocks);
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

    unsigned long mSystemTimeMs = 0;

public:
    void Execute(unsigned long systemTimeMs, unsigned int held);

    //
    void SetState(GameState next);

    GameState GetState()const { return mGameState; };

    void Title();
    void Playing(unsigned long systemTime);
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
};

void GameManager::SetState(GameState next)
{
    if (next == GameState::Playing)
    {
        mPlayingManager.Start();
    }
    mGameState = next;
}

void GameManager::Execute(unsigned long systemTimeMs, unsigned int held)
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
void GameManager::Playing(unsigned long systemTimeMs)
{
    mPlayingManager.Execute(mInputManager, systemTimeMs);

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

void DrawChar(SDL_Renderer* renderer, int x, int y, char c, int scale, unsigned int color)
{
    //char c = 'A';
    if (c >= 0x20 && c <= 0x5F) {
        const uint8_t* data = font8x8[c - 0x20];

        SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, 255);
        for (int ix = 0; ix < 8; ix++) {
            int px = x + ix * scale;
            for (int iy = 0; iy < 8; iy++) {
                if (data[iy] & BIT(7 - ix)) {
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
void DrawString(SDL_Renderer* renderer, int x, int y, const std::string& str, int scale, unsigned int color, bool shadow)
{
    std::ostringstream oss;
    oss << std::uppercase << str;
    std::string strUpper = oss.str();

    if (shadow) {
        for (int i = 0; i < strUpper.size(); i++) {
            char c = strUpper[i];
            DrawChar(renderer, x + scale * 8 * i + scale, y + scale, c, scale, 0x000000);
        }
    }
    for (int i = 0; i < strUpper.size(); i++) {
        char c = strUpper[i];
        DrawChar(renderer, x + scale * 8 * i, y, c, scale, color);
    }
}

void DrawBlock(SDL_Renderer* renderer, int ix, int iy, BlockColor blockColor)
{
    unsigned int color = 0x202020;
    switch (blockColor) {
    case BlockColor::Red: color = 0xFF0000; break;
    case BlockColor::Blue: color = 0x0000FF; break;
    case BlockColor::Green: color = 0x00FF00; break;
    case BlockColor::Yellow: color = 0xFFFF00; break;
    case BlockColor::Empty: color = 0x202020;break;
    }

    const int blockSize = 16;

    const int offsetX = 8;
    const int offsetY = 240 - 8 - blockSize;

    int x = offsetX + blockSize * ix + 1;
    int y = offsetY - blockSize * iy + 1;
    SDL_Rect rect{
        x,y,blockSize - 2,blockSize - 2
    };
    SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, 255);
    SDL_RenderFillRect(renderer, &rect);
}

void DrawBoard(SDL_Renderer* renderer, const Board& board)
{
    for (int ix = 0; ix < WIDTH; ix++) {
        for (int iy = 0; iy < HEIGHT; iy++) {
            BlockColor blockColor = board.GetColor(ix, iy);
            DrawBlock(renderer, ix, iy, blockColor);
        }
    }
}
void DrawFallingPair(SDL_Renderer* renderer, const FallingPair& fallingPair)
{
    int ix = fallingPair.GetX();
    int iy = fallingPair.GetY();
    BlockColor blockColor = fallingPair.GetColor(0);

    DrawBlock(renderer, ix, iy, blockColor);

    //相方
    switch (fallingPair.GetDirection())
    {
    case Direction::Up: ++iy; break;
    case Direction::Down: --iy; break;
    case Direction::Left: --ix; break;
    case Direction::Right: ++ix; break;
    }
    BlockColor pairBlockColor = fallingPair.GetColor(1);
    DrawBlock(renderer, ix, iy, pairBlockColor);
}

void ClearScreen(SDL_Renderer* renderer)
{
    //画面クリア
    {
        SDL_SetRenderDrawColor(renderer, 0, 0x20, 0, 255);
        SDL_RenderClear(renderer);
    }
}
void DrawScreen(SDL_Renderer* renderer, const GameManager& gameManager)
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
        DrawString(renderer, 112, 10, "NEXT", 1, 0xFFFFFF, true);
        FallingPair pair = gameManager.GetNextPair();
        DrawBlock(renderer, WIDTH + 1, HEIGHT + 0, pair.GetColor(0));
        DrawBlock(renderer, WIDTH + 1, HEIGHT - 1, pair.GetColor(1));
    }

    SDL_RenderPresent(renderer);
}


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

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        std::cout << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    GameManager gameManager;

    gameManager.SetState(GameState::Playing);

    auto startTime = std::chrono::steady_clock::now();

    bool running = true;

    int frameCount = 0;
    int count = 0;
    unsigned int frameCountSystemTimeMs = 0;

    while (true) {

        // unsigned int pressed = 0;
        // unsigned int repeat = 0;
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
        unsigned int held = 0;
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
            DrawString(renderer, 160, 2, str, 1, 0xFFFFFF, false);
        }

        {
            std::ostringstream oss;
            oss << "KEY" << std::uppercase << std::hex << held;
            std::string str = oss.str();
            DrawString(renderer, 160, 10, str, 1, 0xFFFFFF, false);
        }

        auto now = std::chrono::steady_clock::now();

        unsigned long systemTimeMs = static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count());

        if ((systemTimeMs - frameCountSystemTimeMs) >= 1000) {
            frameCount = count;
            count = 0;
            frameCountSystemTimeMs = systemTimeMs;
        }

        count++;


        gameManager.Execute(systemTimeMs, held);

        DrawScreen(renderer, gameManager);

        if (gameManager.GetState() != GameState::Playing) {
            break;
        }

    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;

}

