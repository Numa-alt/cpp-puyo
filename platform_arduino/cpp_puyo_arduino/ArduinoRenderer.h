#pragma once

#include <GameManager.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>


int CalcBoardX(int ix);
int CalcBoardY(int iy);


void DrawTitle();
void DrawGameOver();
void DrawGameClear();

void DrawBoard(const Board& board, const Board& boardBefore);
void DrawBoardFix(const Board& board);
void DrawFallingPair(const FallingPair& pair);

void DrawNextPair(int x, int y, const FallingPair& pair);
void DrawText(const char* text, int x, int y, std::uint16_t color, std::uint16_t backColor, int size);

void DrawBlock(int ix, int iy, BlockColor color);

void DrawBoardDebug(int ofsx, const Board& board);



void ClearScreen();
