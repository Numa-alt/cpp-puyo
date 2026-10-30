#pragma once

#include <cstdint>

static const int FONT_SIZE = 8;
static const int WINDOW_WIDTH = 320;
static const int WINDOW_HEIGHT = 240;

static const int BlockSize = 16;
static const int BoardOffsetX = 8;
static const int BoardOffsetY = 240 - 8 - BlockSize;

static const std::uint32_t ChainEffectDispTimeMs = 1500;
static const std::uint32_t BlockEraseEffectTimeMs = 1500;
