# cpp-puyo

## 概要

`cpp-puyo` は、落ちものパズルゲームを題材にした C++17 のポートフォリオ作品です。

ゲームロジックをプラットフォーム非依存の `core` として分離し、同じロジックを **Mac上のPC版（SDL2）** と **Arduino UNO R4 Minima実機** の両方で動作させています。

組み込み開発を意識し、メモリ使用量、固定長データ構造、LCD描画負荷、入力処理、時間管理、自動テストまで含めて設計しています。

## 制作目的

長年の C/C++ ゲーム開発経験を、組み込み・産業系ソフトウェア開発でも活用できる形で示すことを目的に制作しています。

特に以下を重視しています。

- PC / Arduinoで共通利用できるゲームロジック設計
- プラットフォーム依存処理と `core` の分離
- Arduinoのメモリ制約を意識したデータ構造
- LCD・ゲームパッドなど実機I/Oの利用
- CMake / CTestによるビルド・自動テスト
- 状態遷移とPauseを含む時間管理

同じロジックを異なる実行環境で動かすことで、移植性・保守性を意識した設計を実践しています。

## 特徴

- **PC / Arduino共通core**  
  ゲーム進行、盤面、入力、消去、連鎖などを `core` に集約し、描画や実機I/Oだけを各プラットフォーム側へ分離しています。

- **組み込み向け省メモリ設計**  
  固定長データ構造を中心に使用し、ゲームイベントは16bitへパックしています。

- **Arduino LCDの差分描画**  
  前回表示との差分だけを描画し、SPI通信量と描画負荷を抑えています。

- **入力処理の共通化**  
  `InputManager` が `Held / Pressed / Released / Repeat` を生成し、PCとArduinoで操作ロジックを共通化しています。

- **ゲーム内時間を考慮したPause**  
  Pause中の実時間を各タイマーから除外し、再開時に処理が飛ばないよう補正しています。

- **PC上での自動テスト**  
  `Board`、`InputManager`、`GameEvent` などの共通ロジックをCTestで検証しています。

---

## システム構成

## システム構成

ゲームロジックとプラットフォーム依存処理を分離しています。

```text
                +----------------------+
                |        core          |
                |----------------------|
                | Board                |
                | GameManager          |
                | PlayingManager       |
                | InputManager         |
                | FallingPair          |
                | GameEvent            |
                +----------+-----------+
                           |
             +-------------+-------------+
             |                           |
             v                           v
+------------------------+   +----------------------------+
|     platform_pc        |   |     platform_arduino       |
|------------------------|   |----------------------------|
| SDL2                   |   | ST7789 LCD                 |
| キーボード入力         |   | seesawゲームパッド        |
| PC向け描画・演出       |   | Arduino向け描画・入力    |
+------------------------+   +----------------------------+
             |                           |
             v                           v
          macOS                    Arduino UNO R4 Minima
```

### `core`

PC版とArduino版で共通利用する、プラットフォーム非依存のゲームロジックです。

主に以下を担当します。

- 盤面管理
- 落下中ブロックの管理
- 移動・回転・落下
- 接地処理
- 消去判定
- 連鎖処理
- スコア計算
- 入力状態管理
- ゲームイベント通知
- Pauseを含むゲーム内時間管理

`core` 内ではSDL2やArduino固有APIを使用しません。

### `platform_pc`

macOS上で動作するPC版固有の処理です。

- SDL2による描画
- キーボード入力
- 消去・連鎖エフェクト
- Pause表示
- PC向けメインループ

### `platform_arduino`

Arduino UNO R4 Minima向けの処理です。

- ST7789 LCDへの描画
- seesawゲームパッドからの入力
- Arduino向けメインループ
- LCDの差分描画

---

## 使用技術

### 言語

- C++17

### PC版

- macOS
- SDL2
- CMake
- Clang

### Arduino版

- Arduino UNO R4 Minima
- Arduino IDE
- ST7789搭載 240×320 LCD
- Adafruit GFX Library
- Adafruit ST7735 and ST7789 Library
- Adafruit seesaw Library
- SPI
- I2C

### テスト

- CTest
- `assert` を利用した単体テスト

### 動作確認環境

#### macOS

- PC版のビルド・実行
- Arduino UNO R4 Minimaへの書き込み
- Arduino実機での動作確認

#### Windows

- PC版のビルド確認
- Arduino UNO R4 Minima向けVerify
- CTestによる自動テスト

---

## ディレクトリ構成

```text
cpp-puyo/
├─ core/
│  ├─ Board.cpp
│  ├─ Board.h
│  ├─ FallingPair.h
│  ├─ GameConfig.h
│  ├─ GameEvent.h
│  ├─ GameManager.cpp
│  ├─ GameManager.h
│  ├─ GameTypes.h
│  ├─ InputManager.cpp
│  ├─ InputManager.h
│  ├─ PlayingManager.cpp
│  └─ PlayingManager.h
│
├─ platform_pc/
│  ├─ main.cpp
│  ├─ Renderer.*
│  ├─ Effect.*
│  ├─ PcConfig.*
│  └─ font8x8.*
│
├─ platform_arduino/
│  └─ cpp_puyo_arduino/
│     ├─ cpp_puyo_arduino.ino
│     ├─ ArduinoRenderer.h
│     └─ ArduinoRenderer.cpp
│
├─ tests/
│  ├─ test_board.cpp
│  ├─ test_game_event.cpp
│  └─ test_input.cpp
│
└─ CMakeLists.txt
```

### 主なクラス

| クラス | 主な役割 |
|---|---|
| `GameManager` | ゲーム全体の状態管理 |
| `PlayingManager` | プレイ中の状態遷移・ゲーム進行 |
| `Board` | 盤面、配置、消去判定、落下処理 |
| `FallingPair` | 操作中の2個1組のブロック |
| `InputManager` | Held / Pressed / Released / Repeat の生成 |
| `GameEvent` | coreからプラットフォーム側へのイベント通知 |

`PlayingManager` では主に以下の状態を管理しています。

```text
Spawn
  ↓
Control
  ↓
EraseCheck
  ├─ 消去なし → Spawn
  └─ 消去あり → Erase
                   ↓
              DropBlocks
                   ↓
              EraseCheck
```

ゲーム終了時は `Result` へ遷移します。

---

## 実装上の工夫

### 1. PC版とArduino版でゲームロジックを共通化

PC版とArduino版でゲームルールを別々に実装すると、同じ処理を二重管理することになり、修正漏れや動作差異が発生しやすくなります。

そこで、

- ゲームルール
- 盤面
- 入力状態
- 状態遷移
- スコア
- イベント

などを `core` に集約しました。

PC版とArduino版では、描画やデバイス入力などのプラットフォーム依存部分だけを個別に実装しています。

これにより、ゲームロジックの修正箇所を一か所に集約し、PC上で確認したロジックをArduinoでもそのまま利用できます。

---

### 2. ゲームイベントを16bitにパック

ArduinoのRAM使用量を抑えるため、ゲーム中に発生するイベント情報を16bitにパックしています。

```text
bit:  FEDCBA9876543210
      EEEXXXYYYYVVVVVV

E : GameEventType  3bit
X : X座標          3bit
Y : Y座標          4bit
V : value          6bit
```

`value` の意味はイベント種別によって変わります。

- `BlockErased` : ブロック色
- `Chain` : 連鎖数
- `GameOver` : 未使用

マスクとビットシフトを使って値を設定・取得します。

1イベントを2byteにすることで、固定長イベントキューのRAM使用量を抑えています。

---

### 3. Arduino LCDを差分描画

Arduino版では、毎フレーム盤面全体を再描画していません。

まず、

1. 固定済みの `Board` をコピー
2. その上に操作中の `FallingPair` を重ねる
3. 最終的に表示したい盤面を作る
4. 前回表示した盤面と比較する
5. 変化したマスだけLCDへ描画する

という流れで更新しています。

これにより、

- 移動
- 回転
- 接地
- 消去

を個別の描画処理として扱う必要が減り、同じ差分更新の仕組みで処理できます。

また、SPI通信量とLCD描画負荷の削減にもつながります。

---

### 4. 入力処理をPC / Arduinoで共通化

PCのキーボードとArduinoのゲームパッドでは、入力を取得する方法が異なります。

プラットフォーム側では「現在押されているボタン」のビット状態だけを `core` へ渡します。

`InputManager` が前回状態と比較し、

- `Held` : 押されている
- `Pressed` : 押した瞬間
- `Released` : 離した瞬間
- `Repeat` : 長押しリピート

を生成します。

これにより、入力デバイスが変わってもゲーム側の操作ロジックを共通化できます。

---

### 5. Pause中はゲーム処理だけでなくゲーム内時間も停止

ゲーム進行には複数のタイマーがあります。

- 自動落下
- 新しいブロックの出現待ち
- 浮遊ブロックの落下
- 消去判定待ち
- 消去演出

単純にPause中の処理を `return` するだけでは、実時間は進み続けます。

例えば消去処理の途中で数秒Pauseすると、再開した瞬間に消去待ち時間がすでに経過したものとして扱われてしまいます。

そのため、

1. Pause開始時刻を保存
2. Pause解除時にPauseしていた時間を算出
3. 各処理の開始時刻へPause時間を加算

することで、Pause中の時間をゲーム進行時間から除外しています。

これにより、Pause前の残り時間を維持したまま自然にゲームを再開できます。

PC版の描画エフェクトもPauseに対応し、途中の進行状態を維持したまま停止・再開します。

---

### 6. 固定長データ構造を優先

Arduinoでの動作を考慮し、最大サイズをあらかじめ決められるデータには固定長データ構造を使用しています。

例：

- 盤面
- 消去フラグ
- ゲームイベントキュー
- 入力状態

必要以上の動的メモリ確保を避け、RAM使用量を予測しやすい構成を目指しています。

---

## 自動テスト

ゲームロジックを `core` として分離しているため、Arduino実機を使わずPC上でロジックをテストできます。

CMake / CTestを利用し、現在は主に以下を確認しています。

### `Board`

- 指定位置へのブロック配置と取得
- `Reset()`
- 同色3個では消えない
- 同色4個で消える
- 5個以上の場合もすべて消去対象になる
- 横方向・縦方向の連結
- L字・T字の連結
- 同じ色でも離れている場合は別グループとして扱う
- `EraseBlocks()`
- 浮遊ブロック検出
- 1段ずつの落下
- 下にブロックがある場合、その直上で停止する

### `InputManager`

- Held
- Pressed
- Released
- Repeat
- 初回Repeatまでの待ち時間
- 2回目以降のRepeat間隔
- ボタンを離して再度入力した場合の状態

### `GameEvent`

- イベント種別
- X座標
- Y座標
- value
- 16bitパック後の値の復元
- イベントキューへのPush / Pop

### テスト実行

Windowsの例：

```powershell
cmake --build build-win
ctest --test-dir build-win -C Debug
```

成功時：

```text
100% tests passed, 0 tests failed
```

ゲーム画面を操作して確認するだけではなく、内部実装を変更した際に既存動作を壊していないことを自動テストで確認できるようにしています。

特に今後、盤面の内部表現やメモリ使用方法を変更する場合にも、外部から見た動作が維持されていることを確認するために利用します。

---

## PC版のビルド・実行

### 必要なもの

- macOS
- CMake
- C++17対応コンパイラ
- SDL2

### リポジトリ取得

```bash
git clone https://github.com/Numa-alt/cpp-puyo.git
cd cpp-puyo
```

### ビルド

```bash
cmake -S . -B build
cmake --build build
```

生成された `cpp_puyo` を実行します。

```bash
./build/cpp_puyo
```

環境やCMakeジェネレータによって、実行ファイルの出力先が異なる場合があります。

PC版では `core` を独立したライブラリとしてビルドし、`platform_pc` からリンクしています。

---

## Arduino版のビルド

### 必要なもの

- Arduino UNO R4 Minima
- Arduino IDE 2.x
- ST7789搭載 240×320 LCD
- Adafruit seesaw対応ゲームパッド

### Arduinoライブラリ

Arduino IDEのLibrary Managerから以下をインストールします。

- Adafruit GFX Library
- Adafruit ST7735 and ST7789 Library
- Adafruit seesaw Library

UNO R4用ボードパッケージとして、

```text
Arduino UNO R4 Boards
```

もインストールします。

### ボード設定

Arduino IDEで、

```text
Tools
→ Board
→ Arduino UNO R4 Boards
→ Arduino UNO R4 Minima
```

を選択します。

---

### `PuyoCore` の設定

Arduino版からもリポジトリ内の `core` を直接利用します。

Arduino Sketchbook内に以下を用意します。

```text
<Sketchbook location>/
└─ libraries/
   └─ PuyoCore/
      ├─ library.properties
      └─ src/
```

`library.properties`：

```text
name=PuyoCore
version=1.0.0
author=Numashita
maintainer=Numashita
sentence=Platform independent core for cpp-puyo
paragraph=Shared game logic for PC and Arduino.
category=Other
architectures=*
```

`src` はリポジトリ内の、

```text
cpp-puyo/core
```

を参照するようにします。

これにより、PC版とArduino版で `core` のソースをコピーせず、同じコードを利用できます。

#### macOS

シンボリックリンクを利用できます。

```bash
ln -s /path/to/cpp-puyo/core \
      ~/Documents/Arduino/libraries/PuyoCore/src
```

Sketchbook locationが異なる場合は、その場所に合わせて変更します。

#### Windows

ジャンクションを利用できます。

```powershell
cmd /c mklink /J src C:\path\to\cpp-puyo\core
```

このコマンドは、

```text
<Sketchbook location>\libraries\PuyoCore
```

で実行します。

---

### スケッチ

Arduino IDEで以下を開きます。

```text
platform_arduino/cpp_puyo_arduino/cpp_puyo_arduino.ino
```

UNO R4 Minimaを選択して `Verify` を実行します。

実機へ書き込む場合は、UNO R4 Minimaのポートを選択して `Upload` を実行します。

---

## 現在の動作確認

### PC版

- タイトル画面
- ブロック移動
- 回転
- 自動落下
- 接地
- 消去
- 連鎖
- スコア
- NEXT表示
- 消去数表示
- Game Over
- Pause
- 消去・連鎖エフェクト

### Arduino版

- ST7789 LCD表示
- ゲームパッド入力
- 移動
- 回転
- 自動落下
- 接地
- 消去
- 連鎖
- NEXT表示
- SCORE表示
- TOTAL表示
- LCD差分描画

---

## 今後の予定

### ゲーム機能

- 表示・操作感の調整
- ゲームバランスの調整
- 必要に応じたルール追加
- COMによる自動プレイの検討

COM自動プレイでは、対戦機能ではなく、現在の盤面から置き場所と回転方向を選択して自動操作する仕組みを検討しています。

### Arduino版

- Pause操作・表示への対応
- NEXT / SCORE / TOTALの不要な再描画削減
- RAM / Flash使用量の継続確認
- Arduino向け文字列処理の見直し

### 自動テスト

- `GameEventManager` のリングバッファ境界テスト
- Pause時間管理のテスト
- 状態遷移のテスト
- 必要に応じた `PlayingManager` のテスト

### メモリ最適化

現在の固定長データ構造に加え、盤面の内部表現についても省メモリ化を検討しています。

ただし、可読性・保守性とのバランスを確認し、削減効果が小さい場合には過度なビットパックを行わない方針です。

### 設計資料

以下を追加予定です。

- クラス図
- ゲーム状態遷移図
- PC / Arduino共通化構成図
- Arduino配線図

### ポートフォリオ資料

- PC版スクリーンショット
- Arduino実機写真
- Arduino実機動作動画
- RAM / Flash使用量