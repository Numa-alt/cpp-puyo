#include "Board.h"

#include <cassert>
#include <iostream>

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
bool Board::IsEraseBlock(int x, int y) const
{
    return mEraseFlags.IsOn(x, y);
}
//真ん中あたりの消えるボロックの位置を取得する
void Board::SearchCenterEraseBlock(int& x, int& y) const
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

int Board::CountEraseBlocks() const
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

void Board::Print() const
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
