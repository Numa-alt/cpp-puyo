#include <GameManager.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include <Adafruit_seesaw.h>

#include "ArduinoRenderer.h"

GameManager gameManager;
GameState gOldGameState = GameState::Title;
PlayState gPlayStateBefore = PlayState::Result;
Board gBoardBefore{};
FallingPair gFallingPairBefore{};

Adafruit_seesaw ss;

constexpr int TFT_CS = 10;
constexpr int TFT_DC = 7;
constexpr int TFT_RST = 8;

Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);

#define BUTTON_X 6
#define BUTTON_Y 2
#define BUTTON_A 5
#define BUTTON_B 1
#define BUTTON_SELECT 0
#define BUTTON_START 16
std::uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) | (1UL << BUTTON_START) |
(1UL << BUTTON_A) | (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);
#define BIT(_x) (1UL<<(_x))


void setup()
{
    Serial.begin(115200);

    while (!Serial)
    {
        delay(10);
    }

    gameManager.SetRandomSeed(12345);
    gameManager.SetState(GameState::Title);

    Serial.println("start");

    if (!ss.begin(0x50)) {
        Serial.println("ERROR! seesaw not found");
        while (1) delay(1);
    }

    Serial.println("seesaw started");

    std::uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
    if (version != 5743) {
        Serial.print("Wrong firmware loaded? ");
        Serial.println(version);
        while (1) delay(10);
    }

    Serial.println("Found Product 5743");

    ss.pinModeBulk(button_mask, INPUT_PULLUP);
    ss.setGPIOInterrupts(button_mask, 1);

    tft.init(240, 320);
    tft.setRotation(0);

    std::uint16_t dark_glay = 8 | (8 << 5) | (8 << 10);
    tft.fillScreen(dark_glay);

}

struct InputState
{
    bool left;
    bool right;
    bool down;
    bool rotate;
};


InputState ReadInput();

InputState ReadInput()
{
    std::uint32_t x = 1024 - ss.analogRead(14);
    std::uint32_t y = ss.analogRead(15);
    InputState is{};

    if (x < 512 - 120)
    {
        is.left = true;
    }
    else if (x > 512 + 120)
    {
        is.right = true;
    }
    if (y > 512 + 120)
    {
        is.down = true;
    }

    std::uint32_t buttons = ss.digitalReadBulk(button_mask);

    is.rotate = !(buttons & BIT(BUTTON_A));

    return is;
}

std::uint32_t GetHeld()
{
    InputState is = ReadInput();
    std::uint32_t held = 0;
    if (is.left)
    {
        held |= ButtonBit(Button::Left);
    }
    if (is.right)
    {
        held |= ButtonBit(Button::Right);
    }
    if (is.down)
    {
        held |= ButtonBit(Button::Down);
    }
    if (is.rotate) {
        held |= ButtonBit(Button::Rotate);
    }
    return held;
}


void loop()
{
    std::uint32_t held = GetHeld();

    std::uint32_t now = millis();

    gameManager.Execute(now, held);

    if (gameManager.GetState() != gOldGameState) {
        gOldGameState = gameManager.GetState();
        ClearScreen();
        if (gameManager.GetState() == GameState::Playing)
        {
            gBoardBefore.Fill(BlockColor::Red);
        }
    }

    switch (gameManager.GetState())
    {
    case GameState::Title:
        {
            DrawTitle();
            break;
        }
    case GameState::Playing:
        {
            if (gameManager.GetPlayState() != gPlayStateBefore)
            {
                if (gameManager.GetPlayState() == PlayState::Control) {
                    gBoardBefore = gameManager.GetBoard();
                }
            }

            //targetに今回描画したい盤面を構築
            Board target{};
            //現在の盤面
            target = gameManager.GetBoard();

            //操作中のブロック
            auto fp = gameManager.GetFallingPair();
            if (fp.GetColor(0) != BlockColor::Empty || fp.GetColor(1) != BlockColor::Empty)
            {
                target.PutOne(fp.GetX(), fp.GetY(), fp.GetColor(0));
                target.PutOne(fp.GetPairX(), fp.GetPairY(), fp.GetColor(1));
            }

            //targetと前回のboardに差異があれば反映
            DrawBoard(target, gBoardBefore);
            gBoardBefore = target;

            //Next
            DrawNextPair(120, 50, gameManager.GetNextPair());

            //Total
            DrawTotal(8, 24, gameManager.GetTotalEraseBlockCount());

            //Score
            DrawScore(8, 2, gameManager.GetScore());

            if (gameManager.GetPlayState() != PlayState::Control)
            {
                gBoardBefore = gameManager.GetBoard();
            }
            gPlayStateBefore = gameManager.GetPlayState();

            //GameEvent処理
            {
                while (!gameManager.IsGameEventEmpty()) {
                    auto ge = gameManager.PopGameEvent();
                    switch (ge.GetType()) {
                    case GameEventType::Chain:
                        {
                            int x = CalcBoardX(ge.GetX());
                            int y = CalcBoardY(ge.GetY());
                            // std::unique_ptr<EffectChain> effectChain = std::make_unique<EffectChain>(x, y, ChainEffectDispTimeMs, systemTimeMs, ge.mChainCount);
                            // effectManager.AddEffect(std::move(effectChain));
                            break;
                        }
                    case GameEventType::BlockErased:
                        {
                            // int x = CalcBoardX(ge.mX) + BlockSize / 2;
                            // int y = CalcBoardY(ge.mY) + BlockSize / 2;
                            // std::unique_ptr<EffectBlockErase> effectEraseBlock = std::make_unique<EffectBlockErase>(x, y, ge.mBlockColor, BlockEraseEffectTimeMs, systemTimeMs);
                            // effectManager.AddEffect(std::move(effectEraseBlock));
                            break;
                        }
                    case GameEventType::GameOver:
                        {
                            break;
                        }
                    }
                }
            }

            break;
        }
    case GameState::GameOver:
        {
            DrawGameOver();
            break;
        }
    case GameState::GameClear:
        {
            DrawGameClear();
            break;
        }
    }
}
