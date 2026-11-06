#pragma once

#include <cstdint>

static constexpr int FontSize = 8;
static constexpr int WindowWidth = 320;
static constexpr int WindowHeight = 240;

static constexpr int BlockSize = 16;
static constexpr int BoardOffsetX = 8;
static constexpr int BoardOffsetY = WindowHeight - 8 - BlockSize;

static constexpr std::uint32_t ChainEffectDisplayTimeMs = 1500;
static constexpr std::uint32_t BlockEraseEffectTimeMs = 1500;
