#pragma once

#include <GameManager.h>

// 画面単位の描画
void ClearScreen(std::uint16_t color);

// ゲーム描画
void DrawBoard(const Board& board, const Board& previousBoard);
void DrawBoardFull(const Board& board);
void DrawNextPair(const FallingPair& pair);
void DrawBlock(int ix, int iy, BlockColor color);

// 共通描画
void DrawText(const char* text, int x, int y, std::uint16_t color, std::uint16_t backColor, int size);

// デバッグ描画
void DrawBoardDebug(int offsetX, const Board& board);
