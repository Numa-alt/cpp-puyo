#include <cassert>
#include <algorithm>
#include <array>
#include <string>

#include "GameConfig.h"
#include "Renderer.h"
#include "Effect.h"

std::uint32_t Effect::GetLifeTimeMs()const
{
    return mLifeTimeMs;
}
std::uint32_t Effect::GetStartTimeMs()const
{
    return mStartTimeMs;
}

float Effect::CalcRate(std::uint32_t systemTimeMs) const
{
    float nowMs = static_cast<float>(systemTimeMs) - static_cast<float>(mStartTimeMs);
    float rate = nowMs / static_cast<float>(mLifeTimeMs);
    if (rate < 0.0f) {
        return 0.0f;
    }
    if (rate > 1.0f) {
        return 1.0f;
    }
    return rate;
}

void EffectManager::AddEffect(std::unique_ptr<Effect> effect)
{
    mEffects.push_back(std::move(effect));
}

void EffectManager::Execute(std::uint32_t systemTimeMs)
{
    for (auto& e : mEffects)
    {
        e->Execute(systemTimeMs);
    }

    //終了処理
    auto it = std::remove_if(mEffects.begin(), mEffects.end(), [systemTimeMs](std::unique_ptr<Effect>& e)
        {
            return (systemTimeMs - e->GetStartTimeMs()) > e->GetLifeTimeMs();
        });
    mEffects.erase(it, mEffects.end());
}

void EffectManager::Draw(SDL_Renderer* renderer)const
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

void EffectBlockErase::Draw(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, (mColor >> 16) & 0xFF, (mColor >> 8) & 0xFF, mColor & 0xFF, static_cast<std::uint8_t>(mAlpha));
    int x = mX - mSize / 2;
    int y = mY - mSize / 2;
    SDL_Rect rect{
        x,y,mSize,mSize
    };
    SDL_RenderFillRect(renderer, &rect);
}

EffectChain::EffectChain(int x, int y, std::uint32_t lifeTimeMs, std::uint32_t startTimeMs, int chainNum)
    : Effect(lifeTimeMs, startTimeMs), mX(x), mY(y), mChainNum(chainNum)
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

void EffectChain::Draw(SDL_Renderer* renderer)
{
    std::string str = std::to_string(mChainNum) + " CHAIN";
    DrawString(renderer, mX, mY + mOffsetY, str, 1, ((mAlpha & 0xFF) << 24) | mColor, false);
}
