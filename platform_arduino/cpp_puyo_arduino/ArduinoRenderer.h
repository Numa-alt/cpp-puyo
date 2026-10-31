#pragma once

#include <GameManager.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

void DrawBoard( const Board& board );

void DrawFallingPair(const FallingPair& pair);
