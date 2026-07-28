# 8-Bit バンク音色ガイド

Aureline v1.0.8の`8-Bit`バンクに収録する、32音色の一覧です。

特定製品のコード、波形ROM、楽曲データは使用せず、Pulse、Noise、
32ステップWave Memory、Poly Modなど、一般的な合成方式を使って
Aureline用に作成しています。

## 音色構成

### 8BIT CORE

| Slot | 音色名 |
|---:|---|
| 01 | CORE PULSE 12.5 |
| 02 | CORE PULSE 25 |
| 03 | CORE SQUARE 50 |
| 04 | CORE PULSE 75 |
| 05 | CORE TRI BASS |
| 06 | CORE NOISE HIT |
| 07 | CORE SAMPLE BITE |
| 08 | CORE DUAL LEAD |
| 09 | CORE BOSS BASS |
| 10 | CORE FANFARE |
| 11 | CORE OCT LEAD |
| 12 | CORE SEMI ALERT |
| 13 | CORE DUTY DUO |
| 14 | CORE DETUNE DUO |
| 15 | CORE SLOW ATTACK |
| 16 | CORE REL LEAD |

### DISK WAVE

| Slot | 音色名 |
|---:|---|
| 17 | DISK PURE WAVE |
| 18 | DISK HOLLOW LEAD |
| 19 | DISK BELL MOD |
| 20 | DISK VOCAL WAVE |
| 21 | DISK CRYSTAL |

### CART PULSE

| Slot | 音色名 |
|---:|---|
| 22 | CART PULSE LEAD |
| 23 | CART DUAL PULSE |
| 24 | CART PULSE BASS |
| 25 | CART PCM BRASS |

### WAVETABLE

| Slot | 音色名 |
|---:|---|
| 26 | TABLE SINE 4BIT |
| 27 | TABLE MAZE |
| 28 | TABLE HOLLOW |

### 効果音

各バンク共通の構成に合わせ、最後の4スロットを効果音にしています。

| Slot | 音色名 |
|---:|---|
| 29 | COIN DROP |
| 30 | POWER UP FX |
| 31 | PSG LASER |
| 32 | NOISE BOOM |

## 保存と読み込み

`8-Bit`はAurelineの4番目の内蔵バンクです。

- `STORE`：選択中のスロットへ編集内容を保存
- `SAVE`：現在の単一音色を`.aurelinevoice`へ書き出す
- `SAVE BANK`：32音色を`.aurelinelibrary.xml`へ書き出す
- `LOAD`：単一音色または32音色バンクを読み込む

バンクを読み込むと、選択した保存先の32音色が置き換わります。必要な音色は
先に`SAVE BANK`でバックアップしてください。
