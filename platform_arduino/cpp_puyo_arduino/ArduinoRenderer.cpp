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
    int x = 100;
    int y = 150;
    std::uint16_t color = 0x7FFF;
    std::uint16_t backColor = 0x0000;
    int size = 1;

    tft.setCursor(x, y);
    tft.setTextColor(color, backColor);
    tft.setTextSize(size);
    tft.print("TITLE");

    y += 16;
    tft.setCursor(x, y);
    tft.setTextColor(color, backColor);
    tft.setTextSize(size);
    tft.print("BUTTON");

}

void DrawGameOver()
{
    int x = 100;
    int y = 150;
    std::uint16_t color = 0x7FFF;
    std::uint16_t backColor = 0x0000;
    int size = 1;

    tft.setCursor(x, y);
    tft.setTextColor(color, backColor);
    tft.setTextSize(size);
    tft.print("GAME OVER");

    y += 16;
    tft.setCursor(x, y);
    tft.setTextColor(color, backColor);
    tft.setTextSize(size);
    tft.print("PUSH BUTTON");
}

void DrawGameClear()
{
    int x = 100;
    int y = 150;
    std::uint16_t color = 0x7FFF;
    std::uint16_t backColor = 0x0000;
    int size = 1;

    tft.setCursor(x, y);
    tft.setTextColor(color, backColor);
    tft.setTextSize(size);
    tft.print("GAME CLEAR");

    y += 16;
    tft.setCursor(x, y);
    tft.setTextColor(color, backColor);
    tft.setTextSize(size);
    tft.print("PUSH BUTTON");

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
    std::uint16_t color = 0x7FFF;
    int size = 1;

    std::string nextTxt{ "NEXT" };
    tft.setCursor(x, y);
    tft.setTextColor(color);
    tft.setTextSize(size);
    tft.print(nextTxt.c_str());


    DrawBlock(WIDTH + 1, HEIGHT - 2, pair.GetColor(0));

    switch (pair.GetDirection())
    {
    case Direction::Up:    ++y; break;
    case Direction::Down:  --y; break;
    case Direction::Left:  --x; break;
    case Direction::Right: ++x; break;
    }

    DrawBlock(WIDTH + 1, HEIGHT - 3, pair.GetColor(1));
}

//Total
void DrawTotal(int x, int y, std::uint32_t totalEraseBlockCount)
{
    std::uint16_t color = 0x7FFF;
    std::uint16_t backColor = 0x0000;
    int size = 1;


    std::string totalText{ "TOTAL " };
    totalText += std::to_string(totalEraseBlockCount);
    tft.setCursor(x, y);
    tft.setTextColor(color, backColor);
    tft.setTextSize(size);
    tft.print(totalText.c_str());

}

//Score
void DrawScore(int x, int y, std::uint32_t score)
{
    std::uint16_t color = 0x7FFF;
    std::uint16_t backColor = 0x0000;
    int size = 1;

    std::string scoreText{ "SCORE " };
    scoreText += std::to_string(score);
    tft.setCursor(x, y);
    tft.setTextColor(color, backColor);
    tft.setTextSize(size);
    tft.print(scoreText.c_str());
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
