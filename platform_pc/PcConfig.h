#pragma once

#include <cstdint>

static constexpr int FONT_SIZE = 8;
static constexpr int WINDOW_WIDTH = 320;
static constexpr int WINDOW_HEIGHT = 240;

static constexpr int BlockSize = 16;
static constexpr int BoardOffsetX = 8;
static constexpr int BoardOffsetY = WINDOW_HEIGHT - 8 - BlockSize;

static constexpr std::uint32_t ChainEffectDispTimeMs = 1500;
static constexpr std::uint32_t BlockEraseEffectTimeMs = 1500;
