#pragma once

#include <array>
#include <cassert>

#include "GameTypes.h"

// 操作中のブロック
class FallingPair
{
private:
    int mX = 0; // 左端を0 右端をWIDTH-1とする
    int mY = HEIGHT - 2; // 一番上の段をHEIGHT-1、一番下の段を0とする
    Direction mDirection = Direction::Up;
    std::array<BlockColor, 2> mColors{};

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
        assert(index < 2);
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
