#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <cstddef>

#include "GameTypes.h"

enum class GameEventType {
    Chain,
    BlockErased,
    GameOver,
};

// 16bit packed GameEvent
//
// bit:  FEDCBA9876543210
//       EEEXXXYYYYVVVVVV
//
// E : GameEventType  3bit
// X : X coordinate    3bit
// Y : Y coordinate    4bit
// V : value           6bit
//
// value の意味はイベント種別によって変わる。
//   BlockErased : BlockColor
//   Chain       : chain count
//   GameOver    : unused
//
// 範囲:
//   type  : 0 - 7
//   x     : 0 - 7   (実際には 0 - WIDTH-1)
//   y     : 0 - 15  (実際には 0 - HEIGHT-1)
//   value : 0 - 63

struct GameEvent {

    std::uint16_t mData{};

    GameEvent() = default;
    GameEvent(GameEventType type, int x, int y, std::uint16_t value)
    {
        SetType(type);
        SetX(x);
        SetY(y);
        SetValue(value);
    }
    void Set(GameEventType type, int x, int y, std::uint16_t value)
    {
        SetType(type);
        SetX(x);
        SetY(y);
        SetValue(value);
    }

    static const std::uint16_t ShiftType = 13;
    static const std::uint16_t ShiftX = 10;
    static const std::uint16_t ShiftY = 6;
    static const std::uint16_t ShiftValue = 0;

    static const std::uint16_t MaskType = 0x7 << ShiftType;
    static const std::uint16_t MaskX = 0x7 << ShiftX;
    static const std::uint16_t MaskY = 0xF << ShiftY;
    static const std::uint16_t MaskValue = 0x3F << ShiftValue;

    void SetType(GameEventType type)
    {
        assert(
            (type == GameEventType::BlockErased) ||
            (type == GameEventType::Chain) ||
            (type == GameEventType::GameOver)
        );

        mData &= ~MaskType;
        mData |= ((static_cast<std::uint16_t>(type)) << ShiftType) & MaskType;
    }
    void SetX(int x)
    {
        assert((x >= 0) && (x < WIDTH));

        mData &= ~MaskX;
        mData |= ((static_cast<std::uint16_t>(x)) << ShiftX) & MaskX;
    }
    void SetY(int y)
    {
        assert((y >= 0) && (y < HEIGHT));
        mData &= ~MaskY;
        mData |= ((static_cast<std::uint16_t>(y)) << ShiftY) & MaskY;
    }
    void SetValue(int value)
    {
        assert((value >= 0) && (value <= static_cast<int>(MaskValue >> ShiftValue)));

        mData &= ~MaskValue;
        mData |= ((static_cast<std::uint16_t>(value)) << ShiftValue) & MaskValue;
    }
    GameEventType GetType() const
    {
        return static_cast<GameEventType>((mData & MaskType) >> ShiftType);
    }
    int GetX() const
    {
        return static_cast<int>((mData & MaskX) >> ShiftX);
    }
    int GetY() const
    {
        return static_cast<int>((mData & MaskY) >> ShiftY);
    }
    int GetValue() const
    {
        return static_cast<int>((mData & MaskValue) >> ShiftValue);
    }
};

class GameEventManager {
private:
    static constexpr std::size_t MaxEvents = WIDTH * HEIGHT + 2;
    std::array<GameEvent, MaxEvents> mGameEvents;
    std::size_t mTop = 0;
    std::size_t mEnd = 0;
public:
    [[nodiscard]] bool Push(GameEventType type, int x, int y, std::uint16_t value)
    {
        std::size_t next = (mEnd + 1) % MaxEvents;
        //次の書き込み先が先頭なら満杯
        if (next == mTop)
        {
            //満杯：上書きせず捨てる
            return false;
        }
        mGameEvents[mEnd].Set(type, x, y, value);
        mEnd = next;
        return true;
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

    std::size_t GetEnd()const
    {
        return mEnd;
    }
};

