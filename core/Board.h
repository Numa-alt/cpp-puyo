#pragma once

#include "GameTypes.h"
#include "FallingPair.h"

struct EraseFlags
{
    std::uint32_t mFlags[WIDTH]{};
    void Init()
    {
        for (auto& f : mFlags)
        {
            f = 0;
        }
    }
    void On(int x, int y)
    {
        mFlags[x] |= Bit(y);
    }
    void Off(int x, int y)
    {
        mFlags[x] &= ~Bit(y);
    }
    bool IsOn(int x, int y) const
    {
        return static_cast<bool>(mFlags[x] & Bit(y));
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

    void SearchConnectedBlocks(int x, int y, BlockColor color, EraseFlags& checkedFlags, EraseFlags& connectedFlags, int& count);
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

    std::uint32_t CountEraseBlocks() const;

    //mEraseFlagsに従ってブロックを消す
    void EraseBlocks();

    BlockColor GetColor(int x, int y) const;

    //指定のブロックは消えるフラグが立っている？
    bool IsEraseBlock(int x, int y) const;

    //真ん中あたりの消えるブロックの位置を取得する
    void SearchCenterEraseBlock(int& x, int& y) const;

};
