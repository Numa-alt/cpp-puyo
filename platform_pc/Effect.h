#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include <SDL.h>

#include "GameTypes.h"


class Effect
{
private:
    std::uint32_t mLifeTimeMs = 0;
    std::uint32_t mStartTimeMs = 0;
    bool mIsPaused = false;
    std::uint32_t mPauseTimeMs = 0;
    float mRate = 0.0f;
public:
    Effect(std::uint32_t lifeTimeMs, std::uint32_t startTimeMs) :mLifeTimeMs(lifeTimeMs), mStartTimeMs(startTimeMs)
    {

    }
    virtual ~Effect() = default;

    virtual void Execute(std::uint32_t) = 0;
    virtual void Draw(SDL_Renderer*) = 0;

    std::uint32_t GetLifeTimeMs() const;
    std::uint32_t GetStartTimeMs() const;
    void SetStartTimeMs(std::uint32_t startTimeMs);

    float CalcRate(std::uint32_t systemTimeMs) const;

    bool GetPause()const;
    void SetPause(bool pause);
    void SetPauseTimeMs(std::uint32_t pauseTimeMs);
    std::uint32_t GetPauseTimeMs()const;
    void SetRate(float rate);
    float GetRate() const;
};


class EffectManager
{
private:
    std::vector<std::unique_ptr<Effect>> mEffects;
public:
    void AddEffect(std::unique_ptr<Effect> effect);

    void Execute(std::uint32_t systemTimeMs, bool isPause);

    void Draw(SDL_Renderer* renderer) const;

    std::size_t GetCount() const;
};

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
    EffectBlockErase(
        int x,
        int y,
        BlockColor blockColor,
        std::uint32_t lifeTimeMs,
        std::uint32_t startTimeMs
    );

    void Execute(std::uint32_t systemTimeMs) override;
    void Draw(SDL_Renderer* renderer) override;
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
    std::uint32_t mChainNum = 0;
public:
    EffectChain(int x, int y, std::uint32_t lifeTimeMs, std::uint32_t startTimeMs, std::uint32_t chainNum);
    void Execute(std::uint32_t systemTimeMs) override;
    void Draw(SDL_Renderer* renderer) override;
};
