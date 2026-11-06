#pragma once

#include <cstdint>

static const int FontSize = 8;
static const int WindowWidth = 320;
static const int WindowHeight = 240;

static const int BlockSize = 16;
static const int BoardOffsetX = 8;
static const int BoardOffsetY = WindowHeight - 8 - BlockSize;

static const std::uint32_t ChainEffectDisplayTimeMs = 1500;
static const std::uint32_t BlockEraseEffectTimeMs = 1500;
