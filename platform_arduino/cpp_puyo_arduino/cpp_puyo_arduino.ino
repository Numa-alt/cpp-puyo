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
//表示している消した数合計
int gDispTotalEraseBlockCount = -1;
//表示しているスコア
int gDispScore = -1;
//表示しているNextPair
FallingPair gDispNextPair{};

bool gIsDrawPause = false;

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
std::uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) | (1UL << BUTTON_START) | (1UL << BUTTON_A) | (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);

#define BIT(_x) (1UL<<(_x))

//表示できる最大消した数
static constexpr int gMaxDispTotalEraseBlockCount = 99999999;
//消した数
static constexpr int gTotalTextX = 120;
static constexpr int gTotalTextY = 24;
static constexpr std::uint16_t gTotalTextColor = 0x7FFF;
static constexpr std::uint16_t gTotalTextBackColor = 0x0000;

//表示できる最大得点
static constexpr int gMaxDispScore = 99999999;
//得点
static constexpr int gScoreTextX = 8;
static constexpr int gScoreTextY = 24;
static constexpr std::uint16_t gScoreTextColor = 0x7FFF;
static constexpr std::uint16_t gScoreTextBackColor = 0x0000;

//NEXT PAIR
static constexpr int gNextPairTextX = 120;
static constexpr int gNextPairTextY = 50;
static constexpr std::uint16_t gNextPairTextColor = 0x7FFF;
static constexpr std::uint16_t gNextPairTextBackColor = 0x0000;

//PAUSE
static constexpr int gPauseTextX = 16;
static constexpr int gPauseTextY = 100;
static constexpr std::uint16_t gPauseTextColor = 0x7FFF;
static constexpr std::uint16_t gPauseTextBackColor = 0x0000;

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
    bool up;
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
    if (y < 512 - 120)
    {
        is.up = true;
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
    if (is.up)
    {
        held |= ButtonBit(Button::Up);
    }
    if (is.rotate)
    {
        held |= ButtonBit(Button::Rotate);
    }
    return held;
}

void DrawPlaying()
{
    if (gameManager.GetPlayState() != gPlayStateBefore)
    {
        if (gameManager.GetPlayState() == PlayState::Control)
        {
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

    //NextPair
    if (isDefferentPair(gameManager.GetNextPair(), gDispNextPair))
    {
        //更新された時だけ描画
        gDispNextPair = gameManager.GetNextPair();
        DrawNextPair(gNextPairTextX, gNextPairTextY, gameManager.GetNextPair());
    }

    //Total
    if (gameManager.GetTotalEraseBlockCount() != gDispTotalEraseBlockCount)
    {
        //更新された時だけ描画
        gDispTotalEraseBlockCount = gameManager.GetTotalEraseBlockCount();
        //最大値確認
        if (gDispTotalEraseBlockCount > gMaxDispTotalEraseBlockCount)
        {
            gDispTotalEraseBlockCount = gMaxDispTotalEraseBlockCount;
        }
        //数字ではなく文字列で保存し、桁ごとに数字が変わったら書き換えるようにしたほうがいいかな
        char totalText[9]{};
        snprintf(totalText, sizeof(totalText), "%8d", gDispTotalEraseBlockCount);
        DrawText(totalText, gTotalTextX, gTotalTextY, gTotalTextColor, gTotalTextBackColor, 1);
    }

    //得点
    if (gameManager.GetScore() != gDispScore)
    {
        //更新された時だけ描画
        gDispScore = gameManager.GetScore();
        //最大値確認
        if (gDispScore > gMaxDispScore)
        {
            gDispScore = gMaxDispScore;
        }
        char scoreText[9]{};
        snprintf(scoreText, sizeof(scoreText), "%8d", gDispScore);
        DrawText(scoreText, gScoreTextX, gScoreTextY + 8, gScoreTextColor, gScoreTextBackColor, 1);
    }

    //Pause
    if (gameManager.IsPause())
    {
        if (!gIsDrawPause)
        {
            gIsDrawPause = true;
            DrawText("PAUSE", gPauseTextX, gPauseTextY + 8, gPauseTextColor, gPauseTextBackColor, 1);
        }
    }
    else
    {
        if (gIsDrawPause)
        {
            gIsDrawPause = false;
        }
    }

    if (gameManager.GetPlayState() != PlayState::Control)
    {
        gBoardBefore = gameManager.GetBoard();
    }
    gPlayStateBefore = gameManager.GetPlayState();

    //GameEvent処理
    {
        while (!gameManager.IsGameEventEmpty())
        {
            auto ge = gameManager.PopGameEvent();
            switch (ge.GetType())
            {
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
}
//違うか？
bool isDefferentPair(const FallingPair& a, const FallingPair& b)
{
    if (a.GetColor(0) != b.GetColor(0))
    {
        return false;
    }
    if (a.GetColor(1) != b.GetColor(1))
    {
        return false;
    }
    return true;
}
//プレイ開始時の処理
void StartPlaying()
{
    gBoardBefore.Fill(BlockColor::Red);
    gIsDrawPause = false;

    //NEXT PAIR TEXT
    DrawText("NEXT", gNextPairTextX, gNextPairTextY, gNextPairTextColor, gNextPairTextBackColor, 1);

    //TOTAL TEXT
    DrawText("TOTAL", gTotalTextX, gTotalTextY, gTotalTextColor, gTotalTextBackColor, 1);

    //SCORE TEXT
    DrawText("SCORE", gScoreTextX, gScoreTextY, gScoreTextColor, gScoreTextBackColor, 1);

    //初回は描画するように
    gDispTotalEraseBlockCount = -1;
    gDispScore = -1;
    gDispNextPair.SetColor(0, BlockColor::Empty);
    gDispNextPair.SetColor(1, BlockColor::Empty);
}

void loop()
{
    std::uint32_t held = GetHeld();

    std::uint32_t now = millis();

    gameManager.Execute(now, held);

    if (gameManager.GetState() != gOldGameState)
    {
        gOldGameState = gameManager.GetState();

        ClearScreen();

        if (gameManager.GetState() == GameState::Playing)
        {
            StartPlaying();
        }
    }

    switch (gameManager.GetState())
    {
    case GameState::Title:
        DrawTitle();
        break;
    case GameState::Playing:
        DrawPlaying();
        break;
    case GameState::GameOver:
        DrawGameOver();
        break;
    case GameState::GameClear:
        DrawGameClear();
        break;
    }
}
