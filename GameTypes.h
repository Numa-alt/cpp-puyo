// enum class GameState;
// enum class PlayState;
// enum class GameResult;
// enum class BlockColor;
// enum class Direction;
// enum class Button;

#pragma once

#include <cstdint>

#define BIT(x) (1u << (x))

#include "GameConfig.h"

enum class GameState
{
    Title,     // タイトル画面
    Playing,   // ゲーム中
    GameOver,  // ゲームオーバー
    GameClear, // ゲームクリア
};

enum class PlayState
{
    Spawn,      // 新しいブロック出現
    Control,    // ブロックを操作中
    EraseCheck, // 消えるかチェック
    Erase,      // 消去中
    DropBlocks, // 落下中
    Result,     // 結果表示中
};

enum class GameResult
{
    Playing,
    GameOver,
    GameClear,
};

enum class BlockColor
{
    Empty, // 無し
    Red,
    Blue,
    Green,
    Yellow,
};

// 相方のブロックの位置
enum class Direction
{
    Up,
    Right,
    Down,
    Left,
};

//ボタン番号
enum class Button
{
    Rotate = 0,
    Left,
    Right,
    Down,
    Max
};

//ボタン番号をビットに変換
inline std::uint32_t ButtonBit(Button button)
{
    return 1u << static_cast<std::uint32_t>(button);
}

