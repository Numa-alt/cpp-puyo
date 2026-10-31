#include "ArduinoRenderer.h"

extern Adafruit_ST7789 tft;

void DrawBoard( const Board& board )
{
    constexpr int blockSize = 16;
    constexpr int offsetX = 8;
    constexpr int offsetY = 8;
    
    for(int y = 0; y < HEIGHT; ++y )
    {
        for(int x = 0; x < WIDTH; ++x )
        {
            std::uint16_t color = ST77XX_BLACK;

            switch(board.GetColor(x,y))
            {
            case BlockColor::Red:       color = ST77XX_RED;     break;
            case BlockColor::Blue:      color = ST77XX_BLUE;    break;
            case BlockColor::Green:     color = ST77XX_GREEN;   break;
            case BlockColor::Yellow:    color = ST77XX_YELLOW;  break;
            case BlockColor::Empty:     color = ST77XX_BLACK;   break;
            }

            int px = offsetX + x *blockSize;
            int py = offsetY + (HEIGHT-1-y) * blockSize;

            tft.fillRect(px,py,blockSize-1, blockSize-1, color);
        }
    }
}

void DrawFallingPair(const FallingPair& pair)
{
    constexpr int blockSize = 16;
    constexpr int offsetX = 8;
    constexpr int offsetY = 8;

    auto drawOne =[&](int x, int y, BlockColor blockColor)
    {
        std::uint16_t color = ST77XX_BLACK;

        switch(blockColor)
        {
            case BlockColor::Red:       color = ST77XX_RED;     break;
            case BlockColor::Blue:      color = ST77XX_BLUE;    break;
            case BlockColor::Green:     color = ST77XX_GREEN;   break;
            case BlockColor::Yellow:    color = ST77XX_YELLOW;  break;
            case BlockColor::Empty:     color = ST77XX_BLACK;   break;
        }
        int px = offsetX + x * blockSize;
        int py = offsetY + (HEIGHT-1-y) * blockSize;

        tft.fillRect(px,py,blockSize-1,blockSize-1,color);
    };

    int x = pair.GetX();
    int y = pair.GetY();

    drawOne(x,y,pair.GetColor(0));

    switch(pair.GetDirection())
    {
        case Direction::Up:    ++y; break;
        case Direction::Down:  --y; break;
        case Direction::Left:  --x; break;
        case Direction::Right: ++x; break;
    }

    drawOne(x,y,pair.GetColor(1));

}
