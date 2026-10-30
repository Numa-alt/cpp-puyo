#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <cassert>

#include <SDL.h>

#include "GameManager.h"
#include "Effect.h"
#include "Renderer.h"
#include "GameConfig.h"
#include "PcConfig.h"

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

    std::uint32_t frameCount = 0;
    std::uint32_t count = 0;
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
#ifdef DEBUG_OVERLAY
        DrawFps(renderer, frameCount);
#endif

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

