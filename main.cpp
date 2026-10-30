#include <iostream>
#include <sstream>
#include <cassert>
#include <array>
#include <chrono>
#include <functional>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

#include <SDL.h>

#include "font8x8.h"

#include "GameTypes.h"
#include "FallingPair.h"
#include "Board.h"
#include "GameEvent.h"
#include "InputManager.h"
#include "PlayingManager.h"

#include "GameConfig.h"

// #define NUM_OF_ARRAY(x) (sizeof(x)/sizeof(x[0]))



int CalcBoardX(int ix) { return BoardOffsetX + ix * BlockSize; }
int CalcBoardY(int iy) { return BoardOffsetY - iy * BlockSize; }





//--------------------------------------

class GameManager
{
private:
    GameState mGameState = GameState::Title;

    InputManager mInputManager;

    PlayingManager mPlayingManager;

    std::uint32_t mSystemTimeMs = 0;

    GameEventManager mGameEventManager;

public:
    void Execute(std::uint32_t systemTimeMs, std::uint32_t held);

    //
    void SetState(GameState next);

    GameState GetState()const { return mGameState; };

    void Title();
    void Playing(std::uint32_t systemTime);
    void GameOver();
    void GameClear();

    const Board& GetBoard() const
    {
        return mPlayingManager.GetBoard();
    }

    const FallingPair& GetFallingPair() const
    {
        return mPlayingManager.GetFallingPair();
    }
    const FallingPair& GetNextPair() const
    {
        return mPlayingManager.GetNextPair();
    }

    PlayState GetPlayState()const { return mPlayingManager.GetState(); }

    void SetRandomSeed(std::uint32_t randomSeed)
    {
        mPlayingManager.SetRandomSeed(randomSeed);
    }

    std::uint32_t GetRandomSeed()const { return mPlayingManager.GetRandomSeed(); }

    int GetChainCount() const { return mPlayingManager.GetChainCount(); }
    int GetScore() const { return mPlayingManager.GetScore(); }

    bool IsGameEventEmpty()const { return mGameEventManager.IsEmpty(); }
    GameEvent PopGameEvent() { return mGameEventManager.Pop(); }

    std::uint32_t GetAutoFallIntervalMs()const { return mPlayingManager.GetAutoFallIntervalMs(); }
    std::uint32_t GetTotalEraseBlockCount()const { return mPlayingManager.GetTotalEraseBlockCount(); }

};

void GameManager::SetState(GameState next)
{
    if (next == GameState::Playing)
    {
        mPlayingManager.Start(mSystemTimeMs, mGameEventManager);
    }
    mGameState = next;
}

void GameManager::Execute(std::uint32_t systemTimeMs, std::uint32_t held)
{
    mSystemTimeMs = systemTimeMs;

    mInputManager.Execute(mSystemTimeMs, held);

    switch (mGameState)
    {
    case GameState::Title:
        Title();
        break;

    case GameState::Playing:
        Playing(mSystemTimeMs);
        break;

    case GameState::GameOver:
        GameOver();
        break;

    case GameState::GameClear:
        GameClear();
        break;
    }
}

// タイトル
void GameManager::Title()
{
    if (mInputManager.IsPressed(Button::Rotate))
    {
        SetState(GameState::Playing);
    }
}
// ゲーム中
void GameManager::Playing(std::uint32_t systemTimeMs)
{
    mPlayingManager.Execute(mInputManager, systemTimeMs, mGameEventManager);

    GameResult result = mPlayingManager.GetGameResult();
    switch (result)
    {
    case GameResult::Playing:
        break;

    case GameResult::GameOver:
        SetState(GameState::GameOver);
        break;

    case GameResult::GameClear:
        SetState(GameState::GameClear);
        break;
    }
}
// ゲームオーバー
void GameManager::GameOver()
{
    if (mInputManager.IsPressed(Button::Rotate))
    {
        // タイトルへ
        SetState(GameState::Title);
    }
}
// ゲームクリア
void GameManager::GameClear()
{
    if (mInputManager.IsPressed(Button::Rotate))
    {
        // タイトルへ
        SetState(GameState::Title);
    }
}

class Effect {
private:
    std::uint32_t mLifeTimeMs = 0;
    std::uint32_t mStartTimeMs = 0;
public:
    Effect(std::uint32_t lifeTimeMs, std::uint32_t startTimeMs) :mLifeTimeMs(lifeTimeMs), mStartTimeMs(startTimeMs) {

    }
    virtual ~Effect() = default;

    virtual void Execute(std::uint32_t) = 0;
    virtual void Draw(SDL_Renderer*) = 0;

    std::uint32_t GetLifeTimeMs()const { return mLifeTimeMs; }
    std::uint32_t GetStartTimeMs()const { return mStartTimeMs; }

    float CalcRate(std::uint32_t systemTimeMs) const {
        float nowMs = static_cast<float>(systemTimeMs) - static_cast<float>(mStartTimeMs);
        float rate = nowMs / static_cast<float>(mLifeTimeMs);
        if (rate < 0.0f) {
            return 0.0f;
        }
        if (rate > 1.0f) {
            return 1.0f;
        }
        return rate;
    }
};


class EffectManager {
private:
    std::vector<std::unique_ptr<Effect>> mEffects;
public:
    void AddEffect(std::unique_ptr<Effect> effect) {
        mEffects.push_back(std::move(effect));
    }

    void Execute(std::uint32_t systemTimeMs)
    {

        for (auto& e : mEffects) {
            e->Execute(systemTimeMs);
        }

        //終了処理
        auto it = std::remove_if(mEffects.begin(), mEffects.end(), [systemTimeMs](std::unique_ptr<Effect>& e)
            {
                return (systemTimeMs - e->GetStartTimeMs()) > e->GetLifeTimeMs();
            });
        mEffects.erase(it, mEffects.end());
    }

    void Draw(SDL_Renderer* renderer)const
    {
        for (const auto& e : mEffects) {
            e->Draw(renderer);
        }
    }
};


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
//ブロック消去エフェクト
class EffectBlockErase : public Effect
{
private:
    int mX = 0;
    int mY = 0;
    BlockColor mBlockColor;
    std::uint32_t mAlpha = 0;
    std::uint32_t mColor = 0;
    int mSize = 0;
public:
    EffectBlockErase(int x, int y, BlockColor blockColor, std::uint32_t lifeTimeMs, std::uint32_t startTimeMs)
        :Effect(lifeTimeMs, startTimeMs), mX(x), mY(y), mBlockColor(blockColor)
    {
        mAlpha = 255;
        mSize = 0;
        switch (mBlockColor) {
        case BlockColor::Red: mColor = 0xFF0000; break;
        case BlockColor::Blue: mColor = 0x0000FF; break;
        case BlockColor::Yellow: mColor = 0xFFFF00; break;
        case BlockColor::Green: mColor = 0x00FF00; break;
        case BlockColor::Empty: mColor = 0x202020; break;
        }
    }

    void Execute(std::uint32_t systemTimeMs) override {
        float rate = CalcRate(systemTimeMs);
        float alpha = 255.0f - 255.0f * rate;
        mAlpha = static_cast<std::uint32_t>(alpha) & 0xFF;
        mSize = BlockSize - static_cast<int>(static_cast<float>(BlockSize) * rate);
    }
    void Draw(SDL_Renderer* renderer) override
    {
        SDL_SetRenderDrawColor(renderer, (mColor >> 16) & 0xFF, (mColor >> 8) & 0xFF, mColor & 0xFF, static_cast<std::uint8_t>(mAlpha));
        int x = mX - mSize / 2;
        int y = mY - mSize / 2;
        SDL_Rect rect{
            x,y,mSize,mSize
        };
        SDL_RenderFillRect(renderer, &rect);
    }
};
//連鎖数エフェクト
class EffectChain : public Effect
{
private:
    int mX = 0;
    int mY = 0;
    int mOffsetY = 0;
    std::uint32_t mAlpha = 255;
    std::uint32_t mColor = 0xFFFFFF;
    int mChainNum = 0;
public:
    EffectChain(int x, int y, std::uint32_t lifeTimeMs, std::uint32_t startTimeMs, int chainNum)
        : Effect(lifeTimeMs, startTimeMs), mX(x), mY(y), mChainNum(chainNum) {
        mOffsetY = 0;
        mAlpha = 255;
        mColor = 0xFFFFFF;
    }
    void Execute(std::uint32_t systemTimeMs) override {
        float rate = CalcRate(systemTimeMs);
        mAlpha = static_cast<std::uint32_t>(255 - 255 * rate) & 0xFF;
        mOffsetY = static_cast<int>(32 - 32 * rate);
        static const std::array<std::uint32_t, 5> colorTbl = {
            0xFF0000,
            0xFFFF00,
            0xFFFFFF,
            0x00FF00,
            0xFF00FF,
        };
        int index = 0;
        if (rate < 0.2f) {
            index = 0;
        }
        else if (rate < 0.4f) {
            index = 1;
        }
        else if (rate < 0.6f) {
            index = 2;
        }
        else if (rate < 0.8f) {
            index = 3;
        }
        else {
            index = 4;
        }
        assert(index < colorTbl.size());
        mColor = colorTbl[index];

    }
    void Draw(SDL_Renderer* renderer) override {
        std::string str = std::to_string(mChainNum) + " CHAIN";
        DrawString(renderer, mX, mY + mOffsetY, str, 1, ((mAlpha & 0xFF) << 24) | mColor, false);
    }
};

int main(int, char* [])
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << SDL_GetError() << '\n';
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "cpp-puyo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (window == nullptr)
    {
        std::cout << "SDL_CreateWindow Error : " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr)
    {
        std::cout << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    GameManager gameManager;
    EffectManager effectManager;

    auto startTime = std::chrono::steady_clock::now();
    {
        auto now = std::chrono::steady_clock::now();

        std::uint32_t randomSeed = static_cast<std::uint32_t>(now.time_since_epoch().count());
        gameManager.SetRandomSeed(randomSeed);
    }

    gameManager.SetState(GameState::Title);

    bool running = true;

    int frameCount = 0;
    int count = 0;
    std::uint32_t frameCountSystemTimeMs = 0;

    while (true) {

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }

            if (!running) {
                break;
            }
        }

        if (!running) {
            break;
        }

        //今押されているか？
        std::uint32_t held = 0;
        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        if (keyState[SDL_SCANCODE_LEFT]) {
            held |= ButtonBit(Button::Left);
        }
        if (keyState[SDL_SCANCODE_RIGHT]) {
            held |= ButtonBit(Button::Right);
        }
        if (keyState[SDL_SCANCODE_DOWN]) {
            held |= ButtonBit(Button::Down);
        }
        if (keyState[SDL_SCANCODE_SPACE]) {
            held |= ButtonBit(Button::Rotate);
        }

        if (keyState[SDL_SCANCODE_ESCAPE]) {
            break;
        }

        ClearScreen(renderer);

        //FPS
        {
            std::ostringstream oss;
            oss << "FPS " << frameCount;
            std::string str = oss.str();
            DrawString(renderer, 240, 2, str, 1, 0xFFFFFFFF, false);
        }

        {
            PlayState state = gameManager.GetPlayState();
            std::string str;
            switch (state) {
            case PlayState::Control: str += "Control";break;
            case PlayState::DropBlocks: str += "DropBlocks";break;
            case PlayState::Erase: str += "Erase";break;
            case PlayState::EraseCheck: str += "EraseCheck";break;
            case PlayState::Result: str += "Result";break;
            case PlayState::Spawn: str += "Spawn";break;
            }
            DrawString(renderer, 80, 2, str, 1, 0xFFFFFFFF, false);
        }

        auto now = std::chrono::steady_clock::now();

        std::uint32_t systemTimeMs = static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count());

        if ((systemTimeMs - frameCountSystemTimeMs) >= 1000) {
            frameCount = count;
            count = 0;
            frameCountSystemTimeMs = systemTimeMs;
        }

        count++;

        gameManager.Execute(systemTimeMs, held);

        //GameEvent処理
        {
            while (!gameManager.IsGameEventEmpty()) {
                auto ge = gameManager.PopGameEvent();
                switch (ge.mEventType) {
                case GameEventType::Chain:
                    {
                        int x = CalcBoardX(ge.mX);
                        int y = CalcBoardY(ge.mY);
                        std::unique_ptr<EffectChain> effectChain = std::make_unique<EffectChain>(x, y, ChainEffectDispTimeMs, systemTimeMs, ge.mChainCount);
                        effectManager.AddEffect(std::move(effectChain));
                        break;
                    }
                case GameEventType::BlockErased:
                    {
                        int x = CalcBoardX(ge.mX) + BlockSize / 2;
                        int y = CalcBoardY(ge.mY) + BlockSize / 2;
                        std::unique_ptr<EffectBlockErase> effectEraseBlock = std::make_unique<EffectBlockErase>(x, y, ge.mBlockColor, BlockEraseEffectTimeMs, systemTimeMs);
                        effectManager.AddEffect(std::move(effectEraseBlock));
                        break;
                    }
                case GameEventType::GameOver:
                    {
                        break;
                    }
                }
            }
        }

        effectManager.Execute(systemTimeMs);

        DrawScreen(renderer, gameManager, effectManager);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;

}

