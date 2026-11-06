#include "Effect.h"

#include <cassert>
#include <algorithm>
#include <array>
#include <string>

#include "Renderer.h"

#include "PcConfig.h"


std::uint32_t Effect::GetLifeTimeMs()const
{
    return mLifeTimeMs;
}

std::uint32_t Effect::GetStartTimeMs()const
{
    return mStartTimeMs;
}

// エフェクト開始からの経過率を0.0〜1.0で返す
float Effect::CalcRate(std::uint32_t systemTimeMs) const
{
    std::uint32_t elapsedMs = systemTimeMs - mStartTimeMs;

    float rate = static_cast<float>(elapsedMs) / static_cast<float>(mLifeTimeMs);
    if (rate < 0.0f) {
        return 0.0f;
    }
    if (rate > 1.0f) {
        return 1.0f;
    }
    return rate;
}

bool Effect::IsPaused() const
{
    return mIsPaused;
}

void Effect::SetPaused(bool pause)
{
    mIsPaused = pause;
}

void Effect::SetPauseRate(float rate)
{
    mPauseRate = rate;
}

float Effect::GetPauseRate() const
{
    return mPauseRate;
}

void Effect::SetStartTimeMs(std::uint32_t startTimeMs)
{
    mStartTimeMs = startTimeMs;
}

void EffectManager::AddEffect(std::unique_ptr<Effect> effect)
{
    mEffects.push_back(std::move(effect));
}

void EffectManager::Execute(std::uint32_t systemTimeMs, bool isPause)
{
    for (auto& e : mEffects)
    {
        if (isPause)
        {
            if (e->IsPaused())
            {
                float rate = e->GetPauseRate();
                std::uint32_t newStartTimeMs = static_cast<std::uint32_t>(systemTimeMs - e->GetLifeTimeMs() * rate);
                e->SetStartTimeMs(newStartTimeMs);
            }
            else
            {
                e->SetPaused(true);
                float rate = e->CalcRate(systemTimeMs);
                e->SetPauseRate(rate);
            }
        }
        else {
            if (e->IsPaused())
            {
                e->SetPaused(false);
            }
        }

        e->Execute(systemTimeMs);
    }

    //終了処理
    if (!isPause) {
        auto it = std::remove_if(mEffects.begin(), mEffects.end(), [systemTimeMs](const std::unique_ptr<Effect>& e)
            {
                return (systemTimeMs - e->GetStartTimeMs()) > e->GetLifeTimeMs();
            });
        mEffects.erase(it, mEffects.end());

    }
}

void EffectManager::Draw(SDL_Renderer* renderer) const
{
    for (const auto& e : mEffects)
    {
        e->Draw(renderer);
    }
}

EffectBlockErase::EffectBlockErase(
    int x,
    int y,
    BlockColor blockColor,
    std::uint32_t lifeTimeMs,
    std::uint32_t startTimeMs
)
    : Effect(lifeTimeMs, startTimeMs), mX(x), mY(y), mBlockColor(blockColor)
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
void EffectBlockErase::Execute(std::uint32_t systemTimeMs)
{
    float rate = CalcRate(systemTimeMs);
    float alpha = 255.0f - 255.0f * rate;
    mAlpha = static_cast<std::uint32_t>(alpha) & 0xFF;
    mSize = BlockSize - static_cast<int>(static_cast<float>(BlockSize) * rate);
}

void EffectBlockErase::Draw(SDL_Renderer* renderer) const
{
    SDL_SetRenderDrawColor(renderer, (mColor >> 16) & 0xFF, (mColor >> 8) & 0xFF, mColor & 0xFF, static_cast<std::uint8_t>(mAlpha));
    int x = mX - mSize / 2;
    int y = mY - mSize / 2;
    SDL_Rect rect{
        x,y,mSize,mSize
    };
    SDL_RenderFillRect(renderer, &rect);
}

EffectChain::EffectChain(int x, int y, std::uint32_t lifeTimeMs, std::uint32_t startTimeMs, std::uint32_t chainNum)
    : Effect(lifeTimeMs, startTimeMs), mX(x), mY(y), mChainCount(chainNum)
{
    mOffsetY = 0;
    mAlpha = 255;
    mColor = 0xFFFFFF;
}

void EffectChain::Execute(std::uint32_t systemTimeMs)
{
    float rate = CalcRate(systemTimeMs);

    mAlpha = static_cast<std::uint32_t>(255 - 255 * rate) & 0xFF;
    mOffsetY = static_cast<int>(32 - 32 * rate);
    static const std::array<std::uint32_t, 5> colorTbl = {
        0xFF0000,
        0xFFFF00,
        0xFFFFFF,
        0x00FF00,
        0xFF00FF,
    };
    std::size_t index = 0;
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
    assert(index < colorTbl.size());
    mColor = colorTbl[index];

}
void EffectChain::Draw(SDL_Renderer* renderer) const
{
    std::string str = std::to_string(mChainCount) + " CHAIN";
    DrawString(renderer, mX, mY + mOffsetY, str, 1, ((mAlpha & 0xFF) << 24) | mColor, false);
}
