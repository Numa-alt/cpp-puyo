#pragma once

#include <cstdint>
#include <string>

#include <SDL.h>

#include "Board.h"
#include "FallingPair.h"
#include "GameManager.h"
#include "Effect.h"

void DrawChar(SDL_Renderer* renderer, int x, int y, char c, int scale, std::uint32_t color);

void DrawString(SDL_Renderer* renderer, int x, int y, const std::string& str, int scale, std::uint32_t color, bool shadow);

void DrawBlock(SDL_Renderer* renderer, int ix, int iy, BlockColor blockColor, std::uint32_t alpha);

void DrawBoard(SDL_Renderer* renderer, const Board& board);

void DrawFallingPair(SDL_Renderer* renderer, const FallingPair& fallingPair);

void ClearScreen(SDL_Renderer* renderer);

void DrawTitle(SDL_Renderer* renderer);

void DrawPlaying(SDL_Renderer* renderer, const GameManager& gameManager);

void DrawGameOver(SDL_Renderer* renderer);

void DrawClear(SDL_Renderer* renderer);

void DrawScreen(SDL_Renderer* renderer, const GameManager& gameManager, const EffectManager& effectManager);

int CalcBoardX(int ix);
int CalcBoardY(int ix);








