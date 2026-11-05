#pragma once

#include<cstdint>

// 盤面のサイズ
static constexpr int HEIGHT = 12;
static constexpr int WIDTH = 6;


static constexpr std::uint32_t SpawnDelayMs = 300;
static constexpr std::uint32_t EraseCheckDelayMs = 10;
static constexpr std::uint32_t EraseDurationMs = 300;
static constexpr std::uint32_t DropStepMs = 200;
static constexpr std::uint32_t InitialAutoFallIntervalMs = 1000;

// この個数を消すごとに落下速度が速くなる
static constexpr std::uint32_t EraseCountForSpeedUp = 4;
// どのくらい速くするか
static constexpr std::uint32_t AutoFallDecreaseMs = 50;
// 落下速度はここまで減る
static constexpr std::uint32_t MinAutoFallIntervalMs = 100;

