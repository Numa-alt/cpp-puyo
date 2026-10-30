#pragma once

#include<cstdint>

// 盤面のサイズ
static constexpr int HEIGHT = 12;
static constexpr int WIDTH = 6;


static const std::uint32_t SpawnDelayMs = 300;
static const std::uint32_t EraseCheckTimeMs = 10;
static const std::uint32_t EraseDurationMs = 300;
static const std::uint32_t DropTimeMs = 200;
static const std::uint32_t InitialAutoFallIntervalMs = 1000;

//この個数を消すごとに落下速度が早くなる
static const std::uint32_t EraseCountForSpeedUp = 4;
//どのくらい早くなるか
static const std::uint32_t AutoFallDecreaseMs = 50;
//落下速度はここまで減る
static const std::uint32_t MinAutoFallIntervalMs = 100;

