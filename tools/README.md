# バランス調整ツール

このディレクトリには、ゲームバランスを「人手の試行錯誤」ではなく**自動プレイ×統計**で調整するためのツールが入っています。

---

## 何を解決するの？

ゲームバランスは「主観」になりがちです。例えば：

- 「敵が硬すぎる」→ HP を 1.2 倍に？ 1.5 倍に？
- 「ボスまで持たない」→ プレイヤー HP を増やす？ 敵スポーンを減らす？
- パラメータが 8 個あると、組み合わせは無限。人手で全部試すのは現実的じゃない。

このシステムでは：

1. **ボット AI を実装**して、人間の代わりに 5 分間プレイ
2. **ヘッドレスモード**で 1 試合を ~0.5 秒で完走（300 倍速）
3. **ベイズ最適化**で「N 回プレイしたボットの生存時間中央値を目標値に近づける」パラメータを自動探索

これにより「**難易度ノブを 1 つ動かせば、釣り合いが取れたまま難しさが変わる**」状態を作れます。

---

## チューニング対象（8 個のパラメータ）

すべて**乗数（デフォルト 1.0）**として実装されています。元のゲーム定数に掛け算される形です。

| パラメータ | 意味 | 大きくすると |
|-----------|------|------------|
| `enemy_hp_mult` | 敵の HP 倍率（全敵タイプ共通） | 敵が硬くなる |
| `enemy_spawn_min_mult` | スポーン最短間隔の倍率 | **小**ほど過密（注意：小さいほど難しい） |
| `enemy_speed_bonus_mult` | 時間経過による敵速度ボーナス倍率 | 終盤の高速化が激しくなる |
| `enemy_damage_mult` | 敵接触ダメージの倍率 | 一撃のダメージが増える |
| `spawn_count_mult` | 1 度のスポーン数倍率 | 一気に大量湧き |
| `player_speed_mult` | プレイヤー速度倍率 | プレイヤーが速くなる（簡単に） |
| `player_hp_mult` | プレイヤー最大 HP 倍率 | プレイヤーが硬くなる |
| `player_invincible_mult` | 被弾後無敵時間の倍率 | 連続被弾しにくくなる |

これらは `src/params.c` の `params_from_difficulty()` で **DIFFICULTY (0-100) から自動算出**されます。

---

## 難易度ノブの仕組み

`src/config.h` の `DIFFICULTY` マクロ（0〜100、デフォルト 50）が**唯一の入り口**です。

```c
#define DIFFICULTY 50         // ← ここを書き換えてビルド
#define DIFFICULTY_EASY    20
#define DIFFICULTY_NORMAL  50
#define DIFFICULTY_HARD    75
#define DIFFICULTY_BRUTAL  90
```

### ピボット曲線（params.c）

各乗数は次の `pivot()` 関数で計算されます：

- DIFFICULTY = 0 → 「最弱（プレイヤー有利）」端の値
- DIFFICULTY = 50 → 1.0（vanilla、本来のゲームバランス）
- DIFFICULTY = 100 → 「最強（敵有利）」端の値

例えば `enemy_hp_mult` は `(0.55, 1.0, 1.70)` の 3 点を通る区分線形。難易度 50 を境に直線が折れます。

これにより：
- DIFFICULTY を 1 動かすと、8 つの乗数が**バランスを保ったまま**少しずつ変化
- 5 分耐え切れる難易度が、ユーザーの操作 1 つで決まる

---

## ボット AI の挙動（src/bot.c）

「中程度の人間プレイヤー」を模した単純なベクトル和ベース：

1. **脅威ベクトル**：近場の敵から離れる方向（距離 ²の逆数で重み付け、半径 140px）
2. **ボス回避**：半径 160px でボスから離れる
3. **敵弾回避**：半径 80px で敵弾から離れる
4. **収集行動**（脅威が薄い時のみ）：最寄りのジェム/アイテム/宝箱に向かう
5. **画面端回避**：マージン 40px 内から離れる

すべてを加重和して、正規化した方向ベクトルを返します。

レベルアップ強化は**ランダム選択**します（プレイヤーごとに偏らないように）。

---

## CLI 仕様（ゲーム側）

```sh
# ヘッドレス・ボット試合
./disk_survivor --headless --bot --seed=42 --duration=300 \
    --params=enemy_hp_mult=1.5,player_hp_mult=1.5 \
    --output=run.txt

# 通常起動（ボットの動きを目視）
./disk_survivor --bot

# 通常プレイ
./disk_survivor
```

| 引数 | 説明 |
|------|------|
| `--headless` | ウィンドウ・音声を初期化せず最高速で実行 |
| `--bot` | プレイヤー入力を bot AI に置き換える |
| `--seed=N` | 乱数シードを固定（再現可能） |
| `--duration=N` | 試合の最大秒数（デフォルト 300） |
| `--params=k=v,k=v,...` | パラメータ乗数を上書き（DIFFICULTY より優先） |
| `--output=path` | 統計を key=value 形式でファイル出力 |

### 出力ファイルのフォーマット

```
duration=187.342         # 実際の生存時間（秒）
survived=0               # 生存したか（1=Yes）
ran_to_completion=0      # 5 分を完走したか
boss_defeated=0          # ボス撃破済み（ボーナス指標）
boss_active=1            # 終了時点でボスが残っていた
kills=342                # 総撃破数
level=14                 # 最終レベル
final_hp=0               # 死亡時 HP
max_hp=8                 # 最終 max HP
param_*                  # 使われた 8 つの乗数値
```

---

## ツール一覧

### `runner.py` — 並列バッチ実行

N 回試合を並列で走らせて、中央値・平均値・勝率を集計します。

```sh
# デフォルトパラメータで 20 回
python3 tools/runner.py --runs 20

# カスタムパラメータで 30 回
python3 tools/runner.py --runs 30 --params=enemy_hp_mult=1.3,spawn_count_mult=1.2

# JSON 形式で出力（他スクリプトへの入力用）
python3 tools/runner.py --runs 20 --json
```

### `sweep_difficulty.py` — 難易度カーブ確認

DIFFICULTY 0〜100 を一気に試して、ボット成績の表を出します。

```sh
python3 tools/sweep_difficulty.py --runs 20
```

出力例：

```
 DIFF   median     mean    p25    p75  kills    lv   win%  boss%
----------------------------------------------------------------------
  0      252.4    253.0    250    258    631  19.6   100%   100%
 20 E    250.3    250.2    248    252    869  23.4   100%   100%
 50 N    246.0    222.9    190    250    877  22.9    45%    45%
 75 H     52.0    104.1     34    219    323  10.3     0%     0%
100       54.3     72.3     31    110    138   7.1     0%     0%
```

`E / N / H / B` は EASY / NORMAL / HARD / BRUTAL のプリセット位置を示します。

**注意**：`params.c` の値を変えたら、`sweep_difficulty.py` 内の `pivot()` 関数も合わせて更新してください。両者が独立しているため、ずれると正確な結果になりません。

### `tune.py` — ベイズ最適化

指定した「目標生存時間中央値」に最も近づくパラメータを自動探索します。

```sh
# 「ボットがちょうど 3 分（180 秒）で半数死ぬ」を狙う
python3 tools/tune.py --target 180 --runs 12 --calls 40 --out tuned.json
```

**仕組み：**
- 8 次元のパラメータ空間（各乗数の上下限）を `gp_minimize`（ガウス過程ベイズ最適化）で探索
- 各候補について `runs` 回試合 → 生存時間中央値を計算
- 損失 = `|中央値 - 目標値|` を最小化
- `calls` 回の評価で最良点に収束

| 引数 | 説明 |
|------|------|
| `--target` | 目標生存時間（秒）。例：180 で「3 分中央値」 |
| `--runs` | 各候補で何回試合するか（多いほどノイズに強い、遅い） |
| `--calls` | 全評価回数。8 次元なので 30〜100 推奨 |
| `--out` | 最良パラメータの JSON 出力先 |

---

## ワークフロー：難易度カーブを調整する

### Step 1：現状確認

```sh
make mac
python3 tools/sweep_difficulty.py --runs 20
```

DIFFICULTY ごとのボット成績を表示。理想は：
- DIFF 20 (EASY)：90% 以上が 5 分到達
- DIFF 50 (NORMAL)：50% 前後が 5 分到達
- DIFF 75 (HARD)：10〜25% が 5 分到達
- DIFF 90 (BRUTAL)：ほぼ 0%、ただし序盤は耐える

### Step 2：目標時間ごとにベイズ最適化

```sh
# NORMAL 用パラメータを探す（中央値 180 秒）
python3 tools/tune.py --target 180 --runs 12 --calls 40 --out normal.json

# HARD 用（中央値 90 秒）
python3 tools/tune.py --target 90 --runs 12 --calls 40 --out hard.json
```

### Step 3：結果を曲線に反映

`tune.py` が出した最適値を見ながら、`src/params.c` の `pivot()` 引数の `min_v`/`max_v` を調整します。

```c
// 例：HARD で出てきた enemy_hp_mult が 1.5 だったら
p->enemy_hp_mult = pivot(difficulty, 0.55f, 1.50f);  // 上限を 1.5 に
```

### Step 4：再 sweep で確認

```sh
make mac
python3 tools/sweep_difficulty.py --runs 30
```

カーブが滑らかになったか、各プリセットが目標に近いかを確認。

---

## ボット成績と実プレイヤーの目安

ボット AI は意図的に「中程度」に設計されています（最適プレイをしない）。実プレイヤーの感覚との対応はおよそ：

| ボットの生存時間 | 普通の人間プレイヤー |
|-----------------|--------------------|
| 5 分到達（勝率 100%）| 余裕で勝てる |
| 中央値 180 秒（勝率 30〜50%）| やや手応えあり、慣れれば勝てる |
| 中央値 60 秒（勝率 0%）| 普通プレイで詰む、上手い人なら時々勝てる |
| 即死（10〜20 秒）| 上級者でも厳しい |

DIFFICULTY をいじる際は「ボットの 1.2〜1.5 倍の生存時間が一般プレイヤーの感覚」くらいに見積もると良いです。

---

## セットアップ

```sh
pip3 install scikit-optimize numpy matplotlib
make mac      # macOS
make win      # Windows 用クロスコンパイル
```

ヘッドレスモードでもウィンドウは「非表示で」作成されます（raylib のグラフィックス初期化が必要なため）。GPU は使われませんが、X11 / Wayland / macOS WindowServer は必要です。SSH 経由などで動かす場合は xvfb 等が必要かもしれません。

---

## ファイル構成

```
tools/
  runner.py              バッチ実行 & 統計集計
  sweep_difficulty.py    DIFFICULTY 全範囲のスイープ
  tune.py                ベイズ最適化
  README.md              これ
```

C 側の関連ファイル：

```
src/
  params.c, params.h     ParamSet, CLI パース, DIFFICULTY → 乗数の写像
  bot.c                  ヒューリスティック AI（観察用、無 ML）
  config.h               DIFFICULTY マクロ
  main.c                 --headless / --bot ハンドリング
```

各乗数を使う箇所：

```
src/player.c    player_speed_mult, player_hp_mult, player_invincible_mult,
                enemy_damage_mult（被弾側で適用）
src/enemy.c     enemy_hp_mult, enemy_spawn_min_mult, enemy_speed_bonus_mult,
                spawn_count_mult
```
