#include "ArduinoRenderer.h"

extern Adafruit_ST7789 tft;

static constexpr int gBoardOffsetX = 8;
static constexpr int gBoardOffsetY = 48;
static constexpr int gBlockSize = 16;

int CalcBoardX(int ix)
{
    return gBoardOffsetX + ix * gBlockSize;
}
int CalcBoardY(int iy)
{
    return gBoardOffsetY - iy * gBlockSize;
}


void ClearScreen()
{
    std::uint16_t dark_glay = 8 | (8 << 5) | (8 << 10);
    tft.fillScreen(dark_glay);
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

void DrawBoardFix(const Board& board)
{
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            DrawBlock(x, y, board.GetColor(x, y));
        }
    }
}

void DrawBoardDebug(int ofsx, const Board& board)
{
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            DrawBlock(x + ofsx, y, board.GetColor(x, y));
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

//Next
void DrawNextPair(int x, int y, const FallingPair& pair)
{
    //軸のブロック
    DrawBlock(WIDTH + 1, HEIGHT - 2, pair.GetColor(0));

    //相方のブロック
    switch (pair.GetDirection())
    {
    case Direction::Up:    ++y; break;
    case Direction::Down:  --y; break;
    case Direction::Left:  --x; break;
    case Direction::Right: ++x; break;
    }
    DrawBlock(WIDTH + 1, HEIGHT - 3, pair.GetColor(1));
}

//Text
void DrawText(const char* text, int x, int y, std::uint16_t color, std::uint16_t backColor, int size)
{
    tft.setCursor(x, y);
    tft.setTextColor(color, backColor);
    tft.setTextSize(size);
    tft.print(text);
}


void DrawPause(int x, int y)
{
    std::uint16_t color = 0x7FFF;
    std::uint16_t backColor = 0x0000;
    int size = 1;

    tft.setCursor(x, y);
    tft.setTextColor(color, backColor);
    tft.setTextSize(size);
    tft.print("PAUSE");
}
