# 視覚可読性 (Visual Clarity) 改善計画

中盤〜終盤で画面がカオスになる問題への対策。Deep research (2026-06-21) の結果を踏まえた実装手順。

---

## 1. 問題の二層構造

| 層 | 原因 | 該当する症状 |
|---|---|---|
| **(A) 技術的** | 加算合成 (BLEND_ADDITIVE) の累積で輝度 1.0 超 → フラット白に潰れる | 自軍弾＋エフェクトの重なり白飛び |
| **(B) 認知的** | 同サイズ・同色相エンティティで視覚階層が崩壊 | 敵が暖色全域に散らばって "敵内 pop-out" 不能、自軍エフェクトが敵より目立つ |

## 2. 業界既定の 4 つの対策の柱

1. **視覚階層 (相対スケール)** — 重要なほど大きく/濃く/輪郭付き
2. **事前注意特徴 (preattentive)** — プレイヤー/敵/弾/アイテムを単一特徴で区別 (FIT定説)
3. **弾幕慣習** — 敵弾は最前面 z、明コア＋暗縁、色は赤/桃/紫
4. **HDR + トーンマップ** — float バッファ＋ Reinhard で白飛びの根本解

## 3. うちのプロジェクトに適用する実装手順

### Section 1: プレイヤー輪郭リング (即効・小工数)

**目的**：終盤に自機を見失う問題への直接対策。
**実装**：`player_draw` で本体描画前後にシアン外周＋暗縁の二重リングを描く。点滅/脈動でさらに目立たせる。
**ファイル**：`src/player.c`
**工数**：~30 行
**期待効果**：高（VS の Mod でも実装される定番）

### Section 2: z-order ルール強制

**目的**：敵弾・プレイヤーを最前面に固定し情報の遮蔽を防ぐ。
**実装順序**：
```
背景 → 敵 → 自軍弾 → 粒子 → 敵弾 → プレイヤー → UI
```
**ファイル**：`src/main.c` の draw_game_world 内、描画順を入れ替え。
**工数**：~50 行
**期待効果**：高

### Section 3: 敵弾の配色と形状を統一

**目的**：黄/橙の敵弾が爆発・金アイテムと衝突して識別性が落ちる問題を解消。
**実装**：
- 敵弾色を赤/桃/紫系に統一
- 明るいコア（白〜淡色）＋暗い縁取り（黒系）の二重描画
- 既存の `enemy_bullets_draw` を書き換え
**ファイル**：`src/enemy.c` (enemy_bullets_draw)
**工数**：~20 行
**期待効果**：高

### Section 4: パーティクル動的減衰

**目的**：粒子過多による overdraw とチャオス回避。
**実装**：
- 画面上の active 粒子数が閾値（例：256）を超えたら新規生成を間引く
- もしくは閾値超過分の粒子の寿命を強制短縮
**ファイル**：`src/particle.c`
**工数**：~30 行
**期待効果**：中（重複領域の白飛びは多少残る）

### Section 5: 加算合成を自軍光条/粒子に限定

**目的**：累積飽和の発生源を半減。
**実装**：
- 現状 `BeginBlendMode(BLEND_ADDITIVE)` で囲ってる描画ブロックを再点検
- 敵・敵弾・UI は通常 alpha (`BLEND_ALPHA`) に
- 自軍弾・自軍粒子・ハイライトのみ加算合成
**ファイル**：`src/main.c` の draw_game_world
**工数**：1-2 時間
**期待効果**：中〜高

### Section 6: 武器ごとの形・色・運動 3 軸差別化

**目的**：「どの武器が今打ってる？」を即判別可能に。
**実装**：
- PULSE = 細い直線光条（既存）
- BEAM = 持続する太い直線（既存）
- NOVA = 拡大環（既存）
- CHAIN = 折れ線稲妻（既存だが BEAM と紛らわしい？）
- TRAIL = 点群残痕
- ORBITERS = 円形軌道粒
- MINES = 静止円→爆発
- BOOMERANG = 回転 X 字
- WHIP = 扇形アーク
→ グレーゾーンの武器を見直し
**ファイル**：各 `src/weapons/*.c` の draw
**工数**：半日
**期待効果**：中

### Section 7: ダメージサイズ＝脅威度の一致

**目的**：「小ダメージで巨大エフェクト」「大ダメージで地味」を排除。
**実装**：
- popup_spawn のテキストサイズを damage に比例
- ヒットフラッシュの強度も damage 比例
- 小ダメージは控えめ、大ダメージは派手 (Sekiro 危符号の文脈)
**ファイル**：`src/particle.c`, popup 系
**工数**：半日
**期待効果**：中

### Section 8: HDR レンダリング (根本対策)

**目的**：加算飽和の工学的解決。
**実装**：
- `RenderTexture` を `PIXELFORMAT_UNCOMPRESSED_R16G16B16A16` 化
- フラグメントシェーダで Reinhard トーンマップ: `color = mapped / (mapped + 1.0)`
- 既存の bloom shader と統合
**ファイル**：`src/main.c`, shader
**工数**：1〜2 日
**期待効果**：高（ただし他の対策で十分かもしれない）

---

## 4. 反証された通説 (採用しない)

| 通説 | 反証 |
|---|---|
| Gage の "Three Reads" 三層モデル | 0-3 反証。ただし「相対スケール」原則は強固 |
| Brotato が 3 色 palette / 4 種敵に統一 | 0-3, 1-2 反証 |
| Cuphead が低密度背景を意図的に採用 | 1-2 反証 |
| pop-out は distractor 数に依存しない | 1-2 (限定容量並列の見解あり) |

## 5. 採用ソース

- Boghog's bullet hell shmup 101 (shmups.wiki)
- Sparen Danmaku Design Studio Guide
- Cuphead: Creating Clarity (Medium SuperJump)
- Designing for Difficulty: Readability in ARPGs (gamedeveloper.com)
- learnopengl.com Advanced-Lighting/HDR
- Christer Ericson - particle overdraw
- arxiv 2412.12198 (Beukelman & Rodrigues 2024)
- Wikipedia Visual Search, FIT (Treisman & Gelade 1980)

## 6. 進捗トラッキング

- [x] Section 1: プレイヤー輪郭リング
- [x] Section 2: z-order ルール
- [x] Section 3: 敵弾配色統一
- [x] Section 4: パーティクル動的減衰
- [x] Section 5: 加算合成限定
- [x] Section 6: 武器差別化見直し
- [x] Section 7: ダメージサイズ一致
- [x] Section 8: HDR レンダリング
