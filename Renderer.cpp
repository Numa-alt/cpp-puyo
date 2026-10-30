#include "Renderer.h"

#include "GameConfig.h"
#include "font8x8.h"

#include "PcConfig.h"


int CalcBoardX(int ix)
{
    return BoardOffsetX + ix * BlockSize;
}
int CalcBoardY(int iy)
{
    return BoardOffsetY - iy * BlockSize;
}


void DrawChar(SDL_Renderer* renderer, int x, int y, char c, int scale, std::uint32_t color)
{
    //char c = 'A';
    if (c >= 0x20 && c <= 0x5F) {
        const std::uint8_t* data = font8x8[c - 0x20];

        SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
        for (int ix = 0; ix < FONT_SIZE; ix++) {
            int px = x + ix * scale;
            for (int iy = 0; iy < FONT_SIZE; iy++) {
                if (data[iy] & BIT((FONT_SIZE - 1) - ix)) {
                    int py = y + iy * scale;
                    SDL_Rect rect{
                        px,py,scale,scale
                    };
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }
    }
}
void DrawString(SDL_Renderer* renderer, int x, int y, const std::string& str, int scale, std::uint32_t color, bool shadow)
{
    //フォントデータは大文字しか用意していないので変換
    std::string strUpper = str;
    for (char& c : strUpper) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }

    //影のためのアルファ
    std::uint32_t alpha = (color & 0xFF000000);

    if (shadow) {
        for (std::size_t i = 0; i < strUpper.size(); i++) {
            char c = strUpper[i];
            DrawChar(renderer, x + scale * 8 * static_cast<int>(i) + scale, y + scale, c, scale, alpha | 0x00000000);
        }
    }

    for (std::size_t i = 0; i < strUpper.size(); i++) {
        char c = strUpper[i];
        DrawChar(renderer, x + scale * 8 * static_cast<int>(i), y, c, scale, color);
    }
}

void DrawBlock(SDL_Renderer* renderer, int ix, int iy, BlockColor blockColor, std::uint32_t alpha)
{
    std::uint32_t color = 0x202020;
    switch (blockColor) {
    case BlockColor::Red: color = 0xFF0000; break;
    case BlockColor::Blue: color = 0x0000FF; break;
    case BlockColor::Green: color = 0x00FF00; break;
    case BlockColor::Yellow: color = 0xFFFF00; break;
    case BlockColor::Empty: color = 0x202020;break;
    }

    int x = CalcBoardX(ix);
    int y = CalcBoardY(iy);
    SDL_Rect rect{
        x,y,BlockSize - 2,BlockSize - 2
    };
    SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, static_cast<std::uint8_t>(alpha));
    SDL_RenderFillRect(renderer, &rect);
}

void DrawBoard(SDL_Renderer* renderer, const Board& board)
{
    for (int ix = 0; ix < WIDTH; ix++) {
        for (int iy = 0; iy < HEIGHT; iy++) {
            BlockColor blockColor = board.GetColor(ix, iy);
            DrawBlock(renderer, ix, iy, blockColor, 0xFF);
        }
    }
}
void DrawFallingPair(SDL_Renderer* renderer, const FallingPair& fallingPair)
{
    int ix = fallingPair.GetX();
    int iy = fallingPair.GetY();
    BlockColor blockColor = fallingPair.GetColor(0);

    DrawBlock(renderer, ix, iy, blockColor, 0xFF);

    //相方
    switch (fallingPair.GetDirection())
    {
    case Direction::Up: ++iy; break;
    case Direction::Down: --iy; break;
    case Direction::Left: --ix; break;
    case Direction::Right: ++ix; break;
    }
    BlockColor pairBlockColor = fallingPair.GetColor(1);
    DrawBlock(renderer, ix, iy, pairBlockColor, 0xFF);
}

void ClearScreen(SDL_Renderer* renderer)
{
    //画面クリア
    {
        SDL_SetRenderDrawColor(renderer, 0, 0x20, 0, 255);
        SDL_RenderClear(renderer);
    }
}

void DrawTitle(SDL_Renderer* renderer)
{
    {
        std::string str = "PUYO";
        int x = WINDOW_WIDTH / 2 - FONT_SIZE * static_cast<int>(str.size()) / 2;
        int y = WINDOW_HEIGHT / 2 - FONT_SIZE / 2;
        DrawString(renderer, x, y, str, 1, 0xFFFFFFFF, false);
    }

    {
        std::string str = "PRESS SPACE";
        int x = WINDOW_WIDTH / 2 - FONT_SIZE * static_cast<int>(str.size()) / 2;
        int y = WINDOW_HEIGHT / 2 + FONT_SIZE / 2 * 3;
        DrawString(renderer, x, y, str, 1, 0xFFFFFFFF, false);
    }
}

void DrawPlaying(SDL_Renderer* renderer, const GameManager& gameManager)
{
    //盤面
    DrawBoard(renderer, gameManager.GetBoard());

    //操作中
    if (gameManager.GetPlayState() == PlayState::Control)
    {
        DrawFallingPair(renderer, gameManager.GetFallingPair());
    }

    //Next
    {
        DrawString(renderer, 112, 10, "NEXT", 1, 0xFFFFFFFF, true);
        FallingPair pair = gameManager.GetNextPair();
        DrawBlock(renderer, WIDTH + 1, HEIGHT + 0, pair.GetColor(0), 0xFF);
        DrawBlock(renderer, WIDTH + 1, HEIGHT - 1, pair.GetColor(1), 0xFF);
    }

    //スコア
    {
        int x = 8;
        int y = 2;
        DrawString(renderer, x, y, "SCORE", 1, 0xFFFFFFFF, false);

        int score = gameManager.GetScore();
        std::string str = std::to_string(score);
        DrawString(renderer, x, y + FONT_SIZE, str, 1, 0xFFFFFFFF, false);
    }

    //消した総数
    {
        std::uint32_t total = gameManager.GetTotalEraseBlockCount();
        int x = 112;
        int y = 120;
        DrawString(renderer, x, y, "TOTAL", 1, 0xFFFFFFFF, false);

        std::string str = std::to_string(total);
        DrawString(renderer, x, y + FONT_SIZE, str, 1, 0xFFFFFFFF, false);
    }

    //落下速度
    {
        std::uint32_t total = gameManager.GetAutoFallIntervalMs();
        int x = 112;
        int y = 144;
        DrawString(renderer, x, y, "INTERVAL", 1, 0xFFFFFFFF, false);

        std::string str = std::to_string(total);
        DrawString(renderer, x, y + FONT_SIZE, str, 1, 0xFFFFFFFF, false);
    }

}

void DrawGameOver(SDL_Renderer* renderer)
{
    {
        std::string str = "GAME OVER";
        int x = WINDOW_WIDTH / 2 - FONT_SIZE * static_cast<int>(str.size()) / 2;
        int y = WINDOW_HEIGHT / 2 - FONT_SIZE / 2;
        DrawString(renderer, x, y, str, 1, 0xFFFFFFFF, false);
    }

    {
        std::string str = "PRESS SPACE";
        int x = WINDOW_WIDTH / 2 - FONT_SIZE * static_cast<int>(str.size()) / 2;
        int y = WINDOW_HEIGHT / 2 + FONT_SIZE / 2 * 3;
        DrawString(renderer, x, y, str, 1, 0xFFFFFFFF, false);
    }

}

void DrawClear(SDL_Renderer* renderer)
{
    std::string str = "CLEAR";
    int x = WINDOW_WIDTH / 2 - FONT_SIZE * static_cast<int>(str.size()) / 2;
    int y = WINDOW_HEIGHT / 2 - FONT_SIZE / 2;
    DrawString(renderer, x, y, str, 1, 0xFFFFFFFF, false);

}

void DrawScreen(SDL_Renderer* renderer, const GameManager& gameManager, const EffectManager& effectManager)
{
    switch (gameManager.GetState()) {
    case GameState::Title:
        {
            DrawTitle(renderer);
            break;
        }
    case GameState::Playing:
        {
            DrawPlaying(renderer, gameManager);
            break;
        }
    case GameState::GameOver:
        {
            DrawGameOver(renderer);
            break;
        }
    case GameState::GameClear:
        {
            DrawClear(renderer);
            break;
        }
    }

    effectManager.Draw(renderer);

    SDL_RenderPresent(renderer);
}
