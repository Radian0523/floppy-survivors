# 強化システム仕様

DISK SURVIVOR の武器・強化・パッシブの全データ。

レベルアップ時に 3 つの強化候補がランダムで提示され、1 つを選んで取得する。すべての数値は `src/config.h` を一次ソースとし、変更されたらここも更新する。

---

## 0. 全強化クイック一覧（24 種）

レベルアップ画面に出る可能性のあるすべての選択肢。**右上タグ**は UI 上の分類バッジ。

**ピック制限：** 武器解放（NEW）は 1 回のみ取得可能。それ以外はすべて **最大 8 回** まで（`UPGRADE_MAX_PICKS`）。

| 名前 | タグ | 効果 | 最大 | 出現条件 |
|------|------|------|----|---------|
| **RAPID FIRE** | WEAPON+ | **全武器**の発射間隔 × 0.82 | 8 | 常時 |
| **MULTI SHOT** | WEAPON+ | Pulse: 撃つ敵数 +1（近い順）／ Boomerang: 投擲数 +1（扇状） | 8 | 常時 |
| **POWER** | WEAPON+ | **全武器**のダメージ +1 | 8 | 常時 |
| **SPEED** | PASSIVE | 移動速度 × 1.12 | 8 | 常時 |
| **MAGNET** | PASSIVE | ジェム回収範囲 +34 px | 8 | 常時 |
| **VITALITY** | PASSIVE | 最大 HP +1（即時回復） | 8 | 常時 |
| **AREA** | PASSIVE | **全武器の AoE 半径 × 1.15**（Nova/Mine/Whip/Trail/Beam幅） | 8 | 常時 |
| **DURATION** | PASSIVE | **全武器の持続時間 × 1.20**（Trail寿命/Beam発射時間/Whipアニメ/Mine寿命/Chain表示） | 8 | 常時 |
| **ORBITERS** | NEW | 武器解放：周回シールド | 1 | Orbiters 未所持 |
| **BEAM** | NEW | 武器解放：スイープレーザー | 1 | Beam 未所持 |
| **NOVA** | NEW | 武器解放：パルス波 | 1 | Nova 未所持 |
| **MINES** | NEW | 武器解放：地雷設置 | 1 | Mines 未所持 |
| **CHAIN** | NEW | 武器解放：連鎖雷撃 | 1 | Chain 未所持 |
| **BOOMERANG** | NEW | 武器解放：投擲＆帰還 | 1 | Boomerang 未所持 |
| **TRAIL** | NEW | 武器解放：移動軌跡 | 1 | Trail 未所持 |
| **WHIP** | NEW | 武器解放：扇状近接 | 1 | Whip 未所持 |
| **ORB COUNT** | WEAPON+ | オーブ +1 | 8 | Orbiters 所持 ＆ 8 個未満 |
| **BEAM ARC** | WEAPON+ | ビームのスイープ半角 +0.3 rad | 8 | Beam 所持 |
| **NOVA RANGE** | WEAPON+ | ノヴァ範囲 +30 | 8 | Nova 所持 |
| **MINE BLAST** | WEAPON+ | マイン爆発半径 +20 | 8 | Mines 所持 |
| **CHAIN JUMPS** | WEAPON+ | チェイン連鎖数 +1 | 8 | Chain 所持 |
| **BOOMERANG SPIN** | WEAPON+ | ブーメラン当たり判定 +6 | 8 | Boomerang 所持 |
| **TRAIL DURATION** | WEAPON+ | トレイル寿命 +0.8s | 8 | Trail 所持 |
| **WHIP ARC** | WEAPON+ | ウィップ攻撃範囲 +0.5 rad | 8 | Whip 所持 |

> ✓ **RAPID FIRE / MULTI SHOT / POWER** は**所持している全武器に乗算される**グローバル強化。`gs->weapon_rate_mult`, `gs->weapon_damage_bonus`, `gs->weapon_extra_projectiles` で管理。後から手に入れる武器も同じ乗数が適用される。

---

## 1. 武器カタログ（全 9 種）

`武器` カテゴリは「自動で攻撃するもの」。プレイヤーは移動のみ。

### 1.1 PULSE BOLT（パルスボルト）— 初期武器候補

| 項目 | 値 |
|------|----|
| 発射間隔 | 0.56 秒 |
| 弾速 | 370 px/s |
| 初期弾数 | 1 |
| 初期ダメージ | 1 |
| ターゲット | 最寄りの敵 1 体（毎発射） |
| 弾の挙動 | 直線。敵に当たると消滅（貫通なし） |

**特徴：** どんな状況でも安定してダメージを出せる主力。

### 1.2 ORBITERS（オービターズ）— 初期武器候補 / 解放可

| 項目 | 値 |
|------|----|
| 初期個数 | 2 個 |
| 公転半径 | 50 px |
| 公転速度 | 約 3 rad/s |
| ダメージ | 1（接触） |

**特徴：** プレイヤーの周囲を回るオーブ。接触ダメージなので、群がる敵に強い。

### 1.3 BEAM（ビーム）— 初期武器候補 / 解放可

| 項目 | 値 |
|------|----|
| 発射間隔 | 2.5 秒 |
| 発射時間 | 0.8 秒 |
| 射程 | 300 px |
| 幅 | 8 px |
| ダメージ | 2 |
| スイープ角 | ±0.7 rad（約 ±40°） |

**特徴：** プレイヤーの向き方向を中心に±40°を反時計回りにスイープ。一度に大量の敵を貫通。

### 1.4 NOVA（ノヴァ）— 初期武器候補 / 解放可

| 項目 | 値 |
|------|----|
| 発射間隔 | 3.0 秒 |
| 最大半径 | 80 px |
| 展開速度 | 400 px/s |
| ダメージ | 1 |

**特徴：** プレイヤー中心から拡大するリング。近接の群れを一掃する近距離 AoE。

### 1.5 MINES（スパークマイン）— 解放可

| 項目 | 値 |
|------|----|
| 設置間隔 | 2.5 秒 |
| 寿命 | 8 秒 |
| 起爆半径 | 10 px（接触） |
| 爆発半径 | 40 px |
| ダメージ | 2 |
| 最大設置数 | 16 |

**特徴：** プレイヤーの足元に地雷を設置。敵が触れる or 寿命切れで爆発。

### 1.6 CHAIN LIGHTNING（チェインライトニング）— 解放可

| 項目 | 値 |
|------|----|
| 発射間隔 | 2.0 秒 |
| 初撃射程 | 120 px |
| ジャンプ射程 | 80 px |
| 連鎖数 | 3 |
| ダメージ | 1（各段階） |

**特徴：** 最寄りの敵を起点に最大 3 体まで雷が連鎖。各敵に 1 ダメージ。

### 1.7 BOOMERANG（ブーメラン）— 解放可

| 項目 | 値 |
|------|----|
| 投擲間隔 | 1.5 秒 |
| 速度 | 280 px/s |
| 射程 | 180 px |
| ダメージ | 1（往復で 2 回ヒット可能） |

**特徴：** プレイヤーの向き方向に投擲、射程で折り返してプレイヤーへ戻る。往復中ずっと攻撃判定。

### 1.8 TRAIL（トレイル）— 解放可

| 項目 | 値 |
|------|----|
| 設置間隔 | 0.12 秒（移動中のみ） |
| 寿命 | 1.2 秒 |
| 半径 | 8 px |
| ダメージ | 1 |
| 最大数 | 64 |

**特徴：** 移動中、プレイヤーの軌跡にダメージ判定を残す。動き続けるプレイスタイル向き。

### 1.9 WHIP（ウィップ）— 解放可

| 項目 | 値 |
|------|----|
| 発動間隔 | 1.4 秒 |
| 表示時間 | 0.25 秒 |
| 射程 | 70 px |
| 攻撃範囲 | プレイヤーの向き ±約 57° |
| ダメージ | 2 |

**特徴：** プレイヤーの向き方向に扇状の近接攻撃。背後は無防備。

---

## 2. 武器強化（WEAPON+）

特定の武器を所持していると、その武器の強化が選択肢に出現する。

### 全武器グローバル強化（常時選択肢）

これらは「持っている全武器」と「これから取る武器」両方に効く。

| 強化 | 効果 | 内部管理 |
|------|------|---------|
| **RAPID FIRE** | 全武器の発射間隔 × 0.82（複数回取れば 0.82² = 0.67 と乗算） | `weapon_rate_mult` |
| **MULTI SHOT** | 発射物系（Pulse / Boomerang）の同時発射数 +1（扇状に広がる） | `weapon_extra_projectiles` |
| **POWER** | 全武器のダメージ +1（毎回累積） | `weapon_damage_bonus` |

### ORBITERS

| 強化 | 効果 | 条件 |
|------|------|------|
| **ORB COUNT** | オーブ数 +1 | 最大 8 個まで |

### BEAM

| 強化 | 効果 |
|------|------|
| **BEAM ARC** | スイープ半角 +0.3 rad（攻撃範囲が広がる） |

### NOVA

| 強化 | 効果 |
|------|------|
| **NOVA RANGE** | 最大半径 +30 px |

### MINES

| 強化 | 効果 |
|------|------|
| **MINE BLAST** | 爆発半径 +20 px |

### CHAIN

| 強化 | 効果 |
|------|------|
| **CHAIN JUMPS** | 連鎖数 +1 |

### BOOMERANG

| 強化 | 効果 |
|------|------|
| **BOOMERANG SPIN** | 当たり判定半径 +6 px |

### TRAIL

| 強化 | 効果 |
|------|------|
| **TRAIL DURATION** | 残痕の寿命 +0.8s |

### WHIP

| 強化 | 効果 |
|------|------|
| **WHIP ARC** | 攻撃範囲 +0.5 rad（約 +30°） |

> 注：各武器の専用強化は 1 種類のみ。さらに PASSIVE と全武器グローバル（RAPID FIRE / POWER / MULTI SHOT）でも間接的に強化される。

---

## 3. パッシブ強化（PASSIVE）

プレイヤー本体に効果。武器に依らず常に有効。

| 強化 | 効果 | 直接の影響 |
|------|------|----------|
| **SPEED** | 移動速度 × 1.12 | 避け・位置取り |
| **MAGNET** | ジェム回収範囲 +34 px | XP 回収効率 |
| **VITALITY** | 最大 HP +1（即時回復） | 耐久力 |
| **AREA** | 全武器の AoE 半径 × 1.15 | 範囲攻撃強化 |
| **DURATION** | 全武器の持続時間 × 1.20 | 滞在型武器強化 |

---

## 4. パッシブが各武器に与える影響

「移動できる ＝ 戦略が変わる」というスタイルなので、SPEED は実は武器ごとに体感差が大きい。

### SPEED の影響

| 武器 | 影響度 | 理由 |
|------|--------|------|
| PULSE BOLT | ◯ | 攻撃位置の自由度↑ |
| ORBITERS | △ | 公転速度は不変だが、敵に接近・逃走しやすくなる |
| BEAM | ◯ | スイープ方向＝向き なので、向き調整がしやすくなる |
| NOVA | △ | 中心がプレイヤーなので、危険な密集地帯への突入・離脱が楽 |
| MINES | ◎ | 単位時間あたりの設置位置数が増え、面制圧力↑ |
| CHAIN | ◯ | 起点の最寄り敵を選ぶ位置取りが楽 |
| BOOMERANG | ◯ | 投擲方向（向き）の調整・戻りの軌跡管理 |
| **TRAIL** | ★★★ | **移動中だけ Trail が増えるので、SPEED の効果が直接ダメージ密度に転化** |
| WHIP | ◎ | 向きの調整 = 攻撃方向の調整、つまり位置取り次第で命中率激変 |

### MAGNET の影響

| 武器 | 影響 |
|------|------|
| 全武器共通 | 撃破後のジェム回収が速い → レベルアップ加速 → 連鎖的に強くなる |
| 特に効くスタイル | NOVA / CHAIN のような **AoE で大量撃破するビルド** は MAGNET の恩恵が大きい |

MAGNET 単体では火力に影響しないが、レベルアップ回数を増やせるため**長期的に最も強い基盤強化**。

### VITALITY の影響

| 武器 | 影響度 | 理由 |
|------|--------|------|
| 遠距離主体（PULSE / CHAIN / BEAM / BOOMERANG） | ◯ | 距離を取れる分、HP は遊撃用 |
| **近接・AoE主体（NOVA / WHIP / TRAIL / MINES / ORBITERS）** | ★★★ | **敵の至近にいる時間が長いため、被弾が増えがち。HP の恩恵が大きい** |

特に NOVA + WHIP のような近接スタイルでは VITALITY が生存の鍵。

---

## 5. ビルド例（推奨パッシブ）

### 遠距離砲台ビルド
PULSE BOLT + CHAIN + BOOMERANG
- 推奨 PASSIVE: SPEED（位置取り）, MAGNET（XP 効率）
- 武器強化: RAPID FIRE, MULTI SHOT, POWER

### 近接無双ビルド
NOVA + WHIP + ORBITERS
- 推奨 PASSIVE: VITALITY（必須）, SPEED
- 武器強化: NOVA RANGE, NOVA POWER, ORB COUNT

### 機動巻き付けビルド
TRAIL + MINES + BOOMERANG
- 推奨 PASSIVE: SPEED（TRAIL のダメージ密度に直結）, VITALITY
- 武器強化: なし（MINES/TRAIL に強化なし、BOOMERANG にも無し → PASSIVE 偏重）

---

## 6. レベルアップ選択 UI のカテゴリタグ

選択カードの右上に表示されるタグの分類：

| タグ | 色 | 対象 |
|------|----|----|
| **NEW** | 金 | 未取得の新武器（解放） |
| **WEAPON+** | オレンジ | 既存武器の強化、PULSE BOLT 系強化 |
| **PASSIVE** | 青 | SPEED / MAGNET / VITALITY |

---

## 7. その他の入手手段

レベルアップ以外で強化や XP を得る経路：

| 入手元 | 内容 |
|--------|------|
| **緑ジェム（S）** | XP 1（弱敵から） |
| **シアンジェム（M）** | XP 3（中堅敵から） |
| **紫ジェム（L）** | XP 7（強敵・エリート・ボスから） |
| **HP アイテム（緑十字）** | HP +2 |
| **マグネット（緑大ジェム）** | 画面上の全ジェムを高速で引き寄せ（2 秒間） |
| **宝箱（シアン）** | レベルアップと同じ強化選択画面が開く |

---

## 8. コードレイアウト

### 8.1 数値の定義場所

すべての数値・上限は `src/config.h` を一次ソースに。

- 武器パラメータ：`#define PULSE_*`, `#define BEAM_*` などのブロック
- 強化倍率：`#define UPGRADE_*_MULT` などのブロック
- ジェム XP：`#define GEM_XP_S/M/L`
- 難易度カーブ：`src/params.c` の `params_from_difficulty()`

### 8.2 武器ごとのファイル

各武器は独立した 1 ファイル：

```
src/weapons/
  pulse.c       PULSE BOLT
  orbiters.c    ORBITERS
  beam.c        BEAM
  nova.c        NOVA
  mines.c       SPARK MINES
  chain.c       CHAIN LIGHTNING
  boomerang.c   BOOMERANG
  trail.c       TRAIL
  whip.c        WHIP
```

各ファイル内に `xxx_init()` / `xxx_update()` / `xxx_draw()` の 3 関数。

### 8.3 共通ヘルパー

`src/weapon_util.h` / `weapon_util.c` に集約：

| 関数 | 用途 |
|------|------|
| `weapon_hit_enemy()` | 単体ダメージ＋撃破処理。`weapon_damage_bonus` を自動加算 |
| `weapon_hit_boss()` | ボスダメージ。`weapon_damage_bonus` を自動加算 |
| `weapon_aoe_damage()` | 範囲ダメージ（敵 + ボス） |
| `weapon_try_hit_boss_radius()` | ボスが半径内にいたらヒット |
| `weapon_nearest_enemy()` | 最寄り敵インデックス検索 |
| `weapon_nearest_target()` | 最寄りターゲット位置（敵 or ボス） |
| `weapon_kill_enemy()` | 撃破処理（ジェム生成・分裂・爆発・宝箱） |
| `enemy_color_for_type()` | 敵タイプ → 色 |

ダメージ加算は helper 内で行われるので、呼び出し側は `gs->X.damage` を base 値として渡せばよい。

### 8.4 状態（GameState）

各武器の状態は独立した struct にまとめる：

```c
PulseWeapon pulse;
OrbitersWeapon orbiters;
BeamWeapon beam;
NovaWeapon nova;
MinesWeapon mines;
ChainWeapon chain;
BoomerangWeapon boomerang;
TrailWeapon trail;
WhipWeapon whip;
```

参照は `gs->beam.damage` `gs->orbiters.count` のように。

### 8.5 グローバル乗数（全武器に影響）

```c
float weapon_rate_mult;       // RAPID FIRE: interval × 0.82 を累積
int   weapon_damage_bonus;    // POWER: 全武器の damage に +1
int   weapon_extra_projectiles; // MULTI SHOT: Pulse/Boomerang の投擲数 +1
```

これらは `apply_upgrade()` で書き換えられ、各武器の update 関数内で base 値に適用される。

### 8.6 新しい武器を追加する手順

1. `src/config.h` に `#define MY_WEAPON_*` を追加
2. `src/game.h` に `MyWeaponWeapon` struct を定義し `GameState` に追加
3. `src/game.h` の `UpgradeType` enum に `UPGRADE_MYWEAPON` を追加
4. `src/weapons/my_weapon.c` を新規作成（init / update / draw）
5. `Makefile` の `SRCS` に追記
6. `src/upgrade.c` の以下を更新：
   - `upgrade_names[]`、`upgrade_descs[]`、`upgrade_colors[]`
   - `is_upgrade_available()` の case
   - `apply_upgrade()` で `my_weapon_init()` を呼ぶ
   - `upgrade_category()` のカテゴリ判定（NEW タグ）
   - `upgrade_draw_preview()` でアイコン or アニメーション
7. `src/main.c` の `update_game()` で `my_weapon_update()` 呼び出し追加
8. `src/main.c` の `draw_game_world()` で `my_weapon_draw()` 呼び出し追加
