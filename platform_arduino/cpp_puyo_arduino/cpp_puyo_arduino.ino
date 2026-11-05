#include <GameManager.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include <Adafruit_seesaw.h>

#include "ArduinoRenderer.h"

GameManager gameManager;
GameState gPreviousGameState = GameState::Title;
PlayState gPreviousPlayState = PlayState::Result;
Board gPreviousBoard{};

int gDisplayedTotalEraseBlockCount = -1;
int gDisplayedScore = -1;
FallingPair gDisplayedNextPair{};
bool gIsPauseDisplayed = false;

Adafruit_seesaw ss;

constexpr int TFT_CS = 10;
constexpr int TFT_DC = 7;
constexpr int TFT_RST = 8;

Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);

// seesawゲームパッドで使用するボタンGPIOのビットマスク
#define BUTTON_X 6
#define BUTTON_Y 2
#define BUTTON_A 5
#define BUTTON_B 1
#define BUTTON_SELECT 0
#define BUTTON_START 16
std::uint32_t gButtonMask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) | (1UL << BUTTON_START) | (1UL << BUTTON_A) | (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);

#define BIT(_x) (1UL<<(_x))


static constexpr std::uint16_t gGameBgColor = 8 | (8 << 5) | (8 << 10);

//TOTAL表示
static constexpr int gMaxDisplayedTotalEraseBlockCount = 99999999;
static constexpr int gTotalTextX = 120;
static constexpr int gTotalTextY = 24;
static constexpr std::uint16_t gTotalTextColor = 0x7FFF;
static constexpr std::uint16_t gTotalTextBackColor = gGameBgColor;

//SCORE表示
static constexpr int gMaxDisplayedScore = 99999999;
static constexpr int gScoreTextX = 8;
static constexpr int gScoreTextY = 24;
static constexpr std::uint16_t gScoreTextColor = 0x7FFF;
static constexpr std::uint16_t gScoreTextBackColor = gGameBgColor;

//NEXT表示
static constexpr int gNextPairTextX = 120;
static constexpr int gNextPairTextY = 50;
static constexpr std::uint16_t gNextPairTextColor = 0x7FFF;
static constexpr std::uint16_t gNextPairTextBackColor = gGameBgColor;

//PAUSE表示
static constexpr int gPauseTextX = 120;
static constexpr int gPauseTextY = 100;
static constexpr std::uint16_t gPauseTextColor = 0x7FFF;
static constexpr std::uint16_t gPauseTextBackColor = gGameBgColor;

void setup()
{
    Serial.begin(115200);

    while (!Serial)
    {
        delay(10);
    }

    // 動作確認を再現しやすくするため、乱数シードを固定
    gameManager.SetRandomSeed(12345);
    gameManager.SetState(GameState::Title);

    Serial.println("start");

    // seesawゲームパッドを初期化
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

    ss.pinModeBulk(gButtonMask, INPUT_PULLUP);
    ss.setGPIOInterrupts(gButtonMask, 1);

    // ST7789 LCDを初期化
    tft.init(240, 320);
    tft.setRotation(0);

    tft.fillScreen(gGameBgColor);

}

struct InputState
{
    bool left;
    bool right;
    bool down;
    bool up;
    bool rotate;
};


InputState ReadInput()
{
    constexpr std::uint32_t JoystickCenter = 512;
    // 中央付近の入力を無視するデッドゾーンを設ける
    constexpr std::uint32_t JoystickDeadZone = 120;

    // ジョイスティックのX軸は実際の操作方向と値の増減が逆なので反転する
    std::uint32_t x = 1024 - ss.analogRead(14);
    std::uint32_t y = ss.analogRead(15);
    InputState inputState{};

    if (x < JoystickCenter - JoystickDeadZone)
    {
        inputState.left = true;
    }
    else if (x > JoystickCenter + JoystickDeadZone)
    {
        inputState.right = true;
    }
    if (y > JoystickCenter + JoystickDeadZone)
    {
        inputState.down = true;
    }
    if (y < JoystickCenter - JoystickDeadZone)
    {
        inputState.up = true;
    }

    std::uint32_t buttons = ss.digitalReadBulk(gButtonMask);

    // INPUT_PULLUPのため、ボタン押下時は対応bitが0になる
    inputState.rotate = !(buttons & BIT(BUTTON_A));

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
    Board target{};

    target = gameManager.GetBoard();

    auto fp = gameManager.GetFallingPair();
    if (fp.GetColor(0) != BlockColor::Empty || fp.GetColor(1) != BlockColor::Empty)
    {
        target.PutOne(fp.GetX(), fp.GetY(), fp.GetColor(0));
        target.PutOne(fp.GetPairX(), fp.GetPairY(), fp.GetColor(1));
    }

    // 前回表示との差分セルだけLCDへ描画する
    DrawBoard(target, gPreviousBoard);
    gPreviousBoard = target;

    // SPI描画負荷を減らすため、UIは値が変化した場合だけ更新する

    if (IsDifferentPair(gameManager.GetNextPair(), gDisplayedNextPair))
    {
        gDisplayedNextPair = gameManager.GetNextPair();
        DrawNextPair(gameManager.GetNextPair());
    }

    if (gameManager.GetTotalEraseBlockCount() != gDisplayedTotalEraseBlockCount)
    {
        gDisplayedTotalEraseBlockCount = gameManager.GetTotalEraseBlockCount();
        // 表示桁数を超えないよう上限値でクランプする
        if (gDisplayedTotalEraseBlockCount > gMaxDisplayedTotalEraseBlockCount)
        {
            gDisplayedTotalEraseBlockCount = gMaxDisplayedTotalEraseBlockCount;
        }
        char totalText[9]{};
        snprintf(totalText, sizeof(totalText), "%8d", gDisplayedTotalEraseBlockCount);
        DrawText(totalText, gTotalTextX, gTotalTextY, gTotalTextColor, gTotalTextBackColor, 1);
    }

    if (gameManager.GetScore() != gDisplayedScore)
    {
        gDisplayedScore = gameManager.GetScore();
        // 表示桁数を超えないよう上限値でクランプする
        if (gDisplayedScore > gMaxDisplayedScore)
        {
            gDisplayedScore = gMaxDisplayedScore;
        }
        char scoreText[9]{};
        snprintf(scoreText, sizeof(scoreText), "%8d", gDisplayedScore);
        DrawText(scoreText, gScoreTextX, gScoreTextY + 8, gScoreTextColor, gScoreTextBackColor, 1);
    }

    // Pause状態が変化した時だけ表示を更新する
    if (gameManager.IsPause())
    {
        if (!gIsPauseDisplayed)
        {
            gIsPauseDisplayed = true;
            DrawText("PAUSE", gPauseTextX, gPauseTextY + 8, gPauseTextColor, gPauseTextBackColor, 1);
        }
    }
    else
    {
        if (gIsPauseDisplayed)
        {
            gIsPauseDisplayed = false;
            DrawText("     ", gPauseTextX, gPauseTextY + 8, gPauseTextColor, gPauseTextBackColor, 1);
        }
    }

    // Control以外では操作中ぷよを前回盤面に残さず、固定盤面を描画基準にする
    if (gameManager.GetPlayState() != PlayState::Control)
    {
        gPreviousBoard = gameManager.GetBoard();
    }
    gPreviousPlayState = gameManager.GetPlayState();

    // Arduino版では未使用のイベントも必ずPopし、固定長キューが詰まるのを防ぐ
    {
        while (!gameManager.IsGameEventEmpty())
        {
            auto ge = gameManager.PopGameEvent();
            switch (ge.GetType())
            {
            case GameEventType::Chain:
                break;
            case GameEventType::BlockErased:
                break;
            case GameEventType::GameOver:
                break;
            }
        }
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
    gPreviousBoard.Fill(BlockColor::Red);
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

    if (gameManager.GetState() != gPreviousGameState)
    {
        gPreviousGameState = gameManager.GetState();

        // GameStateが切り替わった時だけ画面全体を初期化する
        ClearScreen(gGameBgColor);

        if (gameManager.GetState() == GameState::Playing)
        {
            InitializePlayingDisplay();
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
