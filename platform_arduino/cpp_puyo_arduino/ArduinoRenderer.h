#pragma once

#include <GameManager.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// 座標変換
int CalcBoardX(int ix);
int CalcBoardY(int iy);

// 画面単位の描画
void ClearScreen();
void DrawTitle();
void DrawGameOver();
void DrawGameClear();

// ゲーム描画
void DrawBoard(const Board& board, const Board& boardBefore);
void DrawBoardFull(const Board& board);
void DrawFallingPair(const FallingPair& pair);
void DrawNextPair(const FallingPair& pair);
void DrawBlock(int ix, int iy, BlockColor color);

// 共通描画
void DrawText(const char* text, int x, int y, std::uint16_t color, std::uint16_t backColor, int size);

// デバッグ描画
void DrawBoardDebug(int ofsx, const Board& board);



