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

// 落下速度を1段階上げるために必要な消去数
static constexpr std::uint32_t EraseCountForSpeedUp = 4;
// 自動落下間隔の減少量
static constexpr std::uint32_t AutoFallDecreaseMs = 50;
// 自動落下間隔の下限
static constexpr std::uint32_t MinAutoFallIntervalMs = 100;

