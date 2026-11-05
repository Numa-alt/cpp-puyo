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

    bool CanPlace(const FallingPair& pair) const;

    void PlacePair(const FallingPair& pair);

    void PlaceBlock(int x, int y, BlockColor color);

    bool HasFloatingBlocks() const;

    void DropFloatingBlocks();

    // 消去対象を記録し、1つ以上存在する場合はtrueを返す
    bool FindEraseBlocks();

    std::uint32_t CountEraseBlocks() const;

    void EraseBlocks();

    BlockColor GetColor(int x, int y) const;

    bool IsEraseBlock(int x, int y) const;

    void SearchCenterEraseBlock(int& x, int& y) const;

    void Fill(BlockColor color);
};

static_assert(sizeof(Board) == 96, "Board size changed");
