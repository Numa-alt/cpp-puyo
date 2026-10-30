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
    static constexpr std::size_t MaxEvents = WIDTH * HEIGHT + 2;
    std::array<GameEvent, MaxEvents> mGameEvents;
    std::size_t mTop = 0;
    std::size_t mEnd = 0;
public:
    [[nodiscard]] bool Push(GameEventType type, int x, int y, std::uint32_t chainCount, BlockColor blockColor)
    {
        std::size_t next = (mEnd + 1) % MaxEvents;
        //次の書き込み先が先頭なら満杯
        if (next == mTop)
        {
            //満杯：上書きせず捨てる
            return false;
        }
        mGameEvents[mEnd].Set(type, x, y, chainCount, blockColor);
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
};
