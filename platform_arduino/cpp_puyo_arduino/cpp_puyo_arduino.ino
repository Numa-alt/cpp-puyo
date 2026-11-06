#include <GameManager.h>
#include "InputState.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include <Adafruit_seesaw.h>

#include "ArduinoRenderer.h"

GameManager gameManager;
GameState gPreviousGameState = GameState::Title;
PlayState gPreviousPlayState = PlayState::Result;
Board gPreviousBoard{};

bool gIsGameStateDisplayInitialized = false;
int gDisplayedTotalEraseBlockCount = -1;
int gDisplayedScore = -1;
FallingPair gDisplayedNextPair{};
bool gIsPauseDisplayed = false;

Adafruit_seesaw gSeesaw;

constexpr int TFT_CS = 10;
constexpr int TFT_DC = 7;
constexpr int TFT_RST = 8;

Adafruit_ST7789 gTft(TFT_CS, TFT_DC, TFT_RST);

// seesawゲームパッドで使用するボタンGPIOのビットマスク
static constexpr std::uint8_t kButtonA = 5;
static constexpr std::uint32_t gButtonMask = 1UL << kButtonA;

static constexpr std::uint16_t gGameBgColor = 8 | (8 << 5) | (8 << 11);

// TOTAL表示
static constexpr int gMaxDisplayedTotalEraseBlockCount = 99999999;
static constexpr int gTotalTextX = 120;
static constexpr int gTotalTextY = 24;
static constexpr int gTotalNumX = 120;
static constexpr int gTotalNumY = 36;
static constexpr std::uint16_t gTotalTextColor = ST77XX_WHITE;
static constexpr std::uint16_t gTotalTextBackColor = gGameBgColor;

// SCORE表示
static constexpr int gMaxDisplayedScore = 99999999;
static constexpr int gScoreTextX = 8;
static constexpr int gScoreTextY = 24;
static constexpr int gScoreNumX = 8;
static constexpr int gScoreNumY = 32;
static constexpr std::uint16_t gScoreTextColor = ST77XX_WHITE;
static constexpr std::uint16_t gScoreTextBackColor = gGameBgColor;

// NEXT表示
static constexpr int gNextPairTextX = 120;
static constexpr int gNextPairTextY = 50;
static constexpr std::uint16_t gNextPairTextColor = ST77XX_WHITE;
static constexpr std::uint16_t gNextPairTextBackColor = gGameBgColor;

// PAUSE表示
static constexpr int gPauseTextX = 120;
static constexpr int gPauseTextY = 108;
static constexpr std::uint16_t gPauseTextColor = ST77XX_WHITE;
static constexpr std::uint16_t gPauseTextBackColor = gGameBgColor;

constexpr std::uint32_t kJoystickXPin = 14;
constexpr std::uint32_t kJoystickYPin = 15;


void setup()
{
    Serial.begin(115200);

    Serial.println("start");

    // seesawゲームパッドを初期化
    static constexpr std::uint16_t kGamepadI2cAddress = 0x50;
    if (!gSeesaw.begin(kGamepadI2cAddress)) {
        Serial.println("ERROR! seesaw not found");
        while (1) delay(1);
    }

    Serial.println("seesaw started");
    static constexpr std::uint16_t kExpectedProductId = 5743;
    std::uint32_t productId = ((gSeesaw.getVersion() >> 16) & 0xFFFF);
    if (productId != kExpectedProductId) {
        Serial.print("Wrong firmware loaded? ");
        Serial.println(productId);
        while (1) delay(10);
    }

    Serial.println("Found Product 5743");

    gSeesaw.pinModeBulk(gButtonMask, INPUT_PULLUP);

    // ST7789 LCDを初期化
    gTft.init(240, 320);
    gTft.setRotation(0);

    gTft.fillScreen(gGameBgColor);

    std::uint32_t seed =
        micros()
        ^ (static_cast<std::uint32_t>(gSeesaw.analogRead(kJoystickXPin)) << 10)
        ^ gSeesaw.analogRead(kJoystickYPin);
    gameManager.SetRandomSeed(seed);

    gameManager.SetState(GameState::Title);
}

InputState ReadInput()
{
    constexpr std::uint32_t kJoystickAdcMax = 1023;
    constexpr std::uint32_t kJoystickCenter = 512;
    // 中央付近の入力を無視するデッドゾーンを設ける
    constexpr std::uint32_t kJoystickDeadZone = 120;

    // ジョイスティックのX軸は実際の操作方向と値の増減が逆なので反転する
    std::uint32_t x = kJoystickAdcMax - gSeesaw.analogRead(kJoystickXPin);
    std::uint32_t y = gSeesaw.analogRead(kJoystickYPin);
    InputState inputState{};

    if (x < kJoystickCenter - kJoystickDeadZone)
    {
        inputState.left = true;
    }
    else if (x > kJoystickCenter + kJoystickDeadZone)
    {
        inputState.right = true;
    }
    if (y > kJoystickCenter + kJoystickDeadZone)
    {
        inputState.down = true;
    }
    if (y < kJoystickCenter - kJoystickDeadZone)
    {
        inputState.up = true;
    }

    std::uint32_t buttons = gSeesaw.digitalReadBulk(gButtonMask);

    // INPUT_PULLUPのため、ボタン押下時は対応bitが0になる
    inputState.rotate = (buttons & gButtonMask) == 0;

    return inputState;
}

std::uint32_t ReadHeldButtons()
{
    InputState inputState = ReadInput();
    std::uint32_t held = 0;
    if (inputState.left)
    {
        held |= ButtonBit(Button::Left);
    }
    if (inputState.right)
    {
        held |= ButtonBit(Button::Right);
    }
    if (inputState.down)
    {
        held |= ButtonBit(Button::Down);
    }
    if (inputState.up)
    {
        held |= ButtonBit(Button::Up);
    }
    if (inputState.rotate)
    {
        held |= ButtonBit(Button::Rotate);
    }
    return held;
}

void DrawPlaying()
{
    if (gameManager.GetPlayState() != gPreviousPlayState)
    {
        // Control開始時は固定済み盤面を基準にして差分描画を開始する
        if (gameManager.GetPlayState() == PlayState::Control)
        {
            gPreviousBoard = gameManager.GetBoard();
        }
    }

    // 固定済み盤面に操作中のぷよを重ね、今回表示する盤面を作る
    Board target = gameManager.GetBoard();

    auto fallingPair = gameManager.GetFallingPair();
    if (fallingPair.GetColor(0) != BlockColor::Empty || fallingPair.GetColor(1) != BlockColor::Empty)
    {
        target.PlaceBlock(fallingPair.GetX(), fallingPair.GetY(), fallingPair.GetColor(0));
        target.PlaceBlock(fallingPair.GetPartnerX(), fallingPair.GetPartnerY(), fallingPair.GetColor(1));
    }

    // 前回表示との差分セルだけLCDへ描画する
    DrawBoard(target, gPreviousBoard);
    gPreviousBoard = target;

    // SPI描画負荷を減らすため、UIは値が変化した場合だけ更新する
    const FallingPair nextPair = gameManager.GetNextPair();
    if (IsDifferentPair(nextPair, gDisplayedNextPair))
    {
        gDisplayedNextPair = nextPair;
        DrawNextPair(nextPair);
    }

    int displayedCount = gameManager.GetTotalEraseBlockCount();
    // 表示桁数を超えないよう上限値でクランプする
    if (displayedCount > gMaxDisplayedTotalEraseBlockCount)
    {
        displayedCount = gMaxDisplayedTotalEraseBlockCount;
    }
    if (displayedCount != gDisplayedTotalEraseBlockCount)
    {
        gDisplayedTotalEraseBlockCount = displayedCount;

        char totalText[9]{};
        snprintf(totalText, sizeof(totalText), "%8d", displayedCount);
        DrawText(totalText, gTotalNumX, gTotalNumY, gTotalTextColor, gTotalTextBackColor, 1);
    }

    int displayedScore = gameManager.GetScore();
    // 表示桁数を超えないよう上限値でクランプする
    if (displayedScore > gMaxDisplayedScore)
    {
        displayedScore = gMaxDisplayedScore;
    }
    if (displayedScore != gDisplayedScore)
    {
        gDisplayedScore = displayedScore;
        char scoreText[9]{};
        snprintf(scoreText, sizeof(scoreText), "%8d", displayedScore);
        DrawText(scoreText, gScoreNumX, gScoreNumY, gScoreTextColor, gScoreTextBackColor, 1);
    }

    // Pause状態が変化した時だけ表示を更新する
    if (gameManager.IsPaused())
    {
        if (!gIsPauseDisplayed)
        {
            gIsPauseDisplayed = true;
            DrawText("PAUSE", gPauseTextX, gPauseTextY, gPauseTextColor, gPauseTextBackColor, 1);
        }
    }
    else
    {
        if (gIsPauseDisplayed)
        {
            gIsPauseDisplayed = false;
            DrawText("     ", gPauseTextX, gPauseTextY, gPauseTextColor, gPauseTextBackColor, 1);
        }
    }

    // Control以外では操作中ぷよを前回盤面に残さず、固定盤面を描画基準にする
    if (gameManager.GetPlayState() != PlayState::Control)
    {
        gPreviousBoard = gameManager.GetBoard();
    }
    gPreviousPlayState = gameManager.GetPlayState();

    // Arduino版では未使用のイベントも必ずPopし、固定長キューが詰まるのを防ぐ
    while (!gameManager.IsGameEventEmpty())
    {
        gameManager.PopGameEvent();
    }
}

// NEXT表示に必要な2色だけを比較する
bool IsDifferentPair(const FallingPair& a, const FallingPair& b)
{
    if (a.GetColor(0) != b.GetColor(0))
    {
        return true;
    }
    if (a.GetColor(1) != b.GetColor(1))
    {
        return true;
    }
    return false;
}

void InitializePlayingDisplay()
{
    gPreviousBoard = gameManager.GetBoard();
    DrawBoardFull(gPreviousBoard);
    gIsPauseDisplayed = false;

    DrawText("NEXT", gNextPairTextX, gNextPairTextY, gNextPairTextColor, gNextPairTextBackColor, 1);

    DrawText("TOTAL", gTotalTextX, gTotalTextY, gTotalTextColor, gTotalTextBackColor, 1);

    DrawText("SCORE", gScoreTextX, gScoreTextY, gScoreTextColor, gScoreTextBackColor, 1);

    // 初回更新を必ず発生させるため、表示キャッシュを無効値で初期化する
    gDisplayedTotalEraseBlockCount = -1;
    gDisplayedScore = -1;
    gDisplayedNextPair.SetColor(0, BlockColor::Empty);
    gDisplayedNextPair.SetColor(1, BlockColor::Empty);
}

void loop()
{
    std::uint32_t held = ReadHeldButtons();

    std::uint32_t now = millis();

    gameManager.Execute(now, held);

    if ((gameManager.GetState() != gPreviousGameState) || !gIsGameStateDisplayInitialized)
    {
        gIsGameStateDisplayInitialized = true;
        gPreviousGameState = gameManager.GetState();

        // GameStateが切り替わった時だけ画面全体を初期化する
        ClearScreen(gGameBgColor);

        switch (gameManager.GetState())
        {
        case GameState::Title:
            DrawText("TITLE", 50, 140, ST77XX_WHITE, gGameBgColor, 2);
            DrawText("PUSH BUTTON", 50, 180, ST77XX_WHITE, gGameBgColor, 2);
            break;

        case GameState::Playing:
            InitializePlayingDisplay();
            break;

        case GameState::GameOver:
            DrawText("GAME OVER", 50, 140, ST77XX_WHITE, gGameBgColor, 2);
            DrawText("PUSH BUTTON", 50, 180, ST77XX_WHITE, gGameBgColor, 2);
            break;

        case GameState::GameClear:
            DrawText("GAME CLEAR", 50, 140, ST77XX_WHITE, gGameBgColor, 2);
            DrawText("PUSH BUTTON", 50, 180, ST77XX_WHITE, gGameBgColor, 2);
            break;
        }
    }

    if (gameManager.GetState() == GameState::Playing)
    {
        DrawPlaying();
    }
}
