#include "ArduinoRenderer.h"

extern Adafruit_ST7789 tft;

// 盤面描画の原点と1マスのピクセルサイズ
static constexpr int gBoardOffsetX = 8;
static constexpr int gBoardOffsetY = 48;
static constexpr int gBlockSize = 16;

int CalcBoardX(int ix)
{
    return gBoardOffsetX + ix * gBlockSize;
}
int CalcBoardY(int iy)
{
    return gBoardOffsetY + (HEIGHT - 1 - iy) * gBlockSize;
}


void ClearScreen(std::uint16_t color)
{
    tft.fillScreen(color);
}

void DrawTitle()
{
    DrawText("TITLE", 100, 150, 0x7FFF, 0x0000, 2);
    DrawText("PUSH BUTTON", 100, 166, 0x7FFF, 0x0000, 2);
}

void DrawGameOver()
{
    DrawText("GAME OVER", 100, 150, 0x7FFF, 0x0000, 2);
    DrawText("PUSH BUTTON", 100, 166, 0x7FFF, 0x0000, 2);
}

void DrawGameClear()
{
    DrawText("GAME CLEAR", 100, 150, 0x7FFF, 0x0000, 2);
    DrawText("PUSH BUTTON", 100, 166, 0x7FFF, 0x0000, 2);
}

void DrawBlock(int ix, int iy, BlockColor blockColor)
{
    std::uint16_t color;
    switch (blockColor)
    {
    case BlockColor::Red:       color = ST77XX_RED;     break;
    case BlockColor::Blue:      color = ST77XX_BLUE;    break;
    case BlockColor::Green:     color = ST77XX_GREEN;   break;
    case BlockColor::Yellow:    color = ST77XX_YELLOW;  break;
    case BlockColor::Empty:     color = ST77XX_BLACK;   break;
    }

    int px = gBoardOffsetX + ix * gBlockSize;
    int py = gBoardOffsetY + (HEIGHT - 1 - iy) * gBlockSize;

    tft.fillRect(px, py, gBlockSize - 1, gBlockSize - 1, color);

}

// 前回盤面と比較し、変化したセルだけをLCDへ描画する
void DrawBoard(const Board& board, const Board& boardBefore)
{
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            if (board.GetColor(x, y) != boardBefore.GetColor(x, y))
            {
                DrawBlock(x, y, board.GetColor(x, y));
            }
        }
    }
}

// 盤面全体を強制的に再描画する
void DrawBoardFull(const Board& board)
{
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            DrawBlock(x, y, board.GetColor(x, y));
        }
    }
}

// デバッグ用：X方向にオフセットして盤面全体を描画する
void DrawBoardDebug(int offsetX, const Board& board)
{
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            DrawBlock(x + offsetX, y, board.GetColor(x, y));
        }
    }
}

void DrawFallingPair(const FallingPair& pair)
{
    int x = pair.GetX();
    int y = pair.GetY();

    DrawBlock(x, y, pair.GetColor(0));

    switch (pair.GetDirection())
    {
    case Direction::Up:    ++y; break;
    case Direction::Down:  --y; break;
    case Direction::Left:  --x; break;
    case Direction::Right: ++x; break;
    }

    DrawBlock(x, y, pair.GetColor(1));
}

void DrawNextPair(const FallingPair& pair)
{
    //軸のブロック
    DrawBlock(WIDTH + 1, HEIGHT - 2, pair.GetColor(0));

    //相方のブロック
    DrawBlock(WIDTH + 1, HEIGHT - 3, pair.GetColor(1));
}

void DrawText(const char* text, int x, int y, std::uint16_t color, std::uint16_t backColor, int size)
{
    tft.setCursor(x, y);
    tft.setTextColor(color, backColor);
    tft.setTextSize(size);
    tft.print(text);
}
