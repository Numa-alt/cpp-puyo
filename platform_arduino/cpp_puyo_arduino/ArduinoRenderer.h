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
void DrawTotal(int x, int y, std::uint32_t totalEraseBlockCount);
void DrawScore(int x, int y, std::uint32_t score);
void DrawPause(int x, int y);

void DrawBlock(int ix, int iy, BlockColor color);

void DrawBoardDebug(int ofsx, const Board& board);



void ClearScreen();
