#include <iostream>
#include <cassert>
#include <array>

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

enum class Button
{
    Rotate = BIT(0),
    Left = BIT(1),
    Right = BIT(2),
    Down = BIT(3),
};

class InputManager
{
private:
    unsigned int mPressed = 0;
    unsigned int mReleased = 0;
    unsigned int mRepeat = 0;

public:
    bool IsPressed(Button button) const
    {
        return (mPressed & static_cast<unsigned int>(button)) != 0;
    }

    bool IsRepeat(Button button) const
    {
        return (mRepeat & static_cast<unsigned int>(button)) != 0;
    }
};

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
        mY = HEIGHT - 1;
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
            }
            std::cout << c;
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}


int main()
{
    std::cout << "puyo project\n";

    Board board;

    FallingPair pair;
    pair.Reset();
    pair.SetPosition(2, 5);
    //pair.SetDirection(Direction::Right);
    pair.SetColor(0, BlockColor::Red);
    pair.SetColor(1, BlockColor::Blue);

    board.PutOne(2, 0, BlockColor::Red);
    board.PutOne(2, 1, BlockColor::Red);
    board.PutOne(2, 2, BlockColor::Red);
    board.PutOne(2, 3, BlockColor::Red);
    board.PutOne(2, 4, BlockColor::Red);

    board.PutOne(3, 0, BlockColor::Blue);
    board.PutOne(3, 1, BlockColor::Blue);
    board.PutOne(3, 2, BlockColor::Blue);

    board.Print();

    while (true) {
        FallingPair fp = pair;
        fp.Move(0, -1);
        if (board.CanPlace(fp)) {
            pair = fp;
        }
        else {
            break;
        }
    }

    board.PutPair(pair);

    board.Print();

    board.FindEraseBlocks();

    board.EraseBlocks();

    std::cout << "EraseBlocks\n";

    board.Print();

    while (true)
    {
        if (board.HasFloatingBlocks()) {
            board.DropFloatingBlocks();
        }
        else {
            break;
        }
        board.Print();
    }

    board.FindEraseBlocks();

    board.EraseBlocks();

    board.Print();

    std::cout << board.CanPlace(pair) << '\n';

    return 0;

}

