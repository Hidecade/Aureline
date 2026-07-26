# Classic 8-Bit Expansion 音色ガイド

Aureline用のオリジナル50音色ライブラリです。特定製品のコードや
ROMデータは使用せず、パルス波、量子化波形、ウェーブテーブル、
ノイズなどの一般的なデジタル合成方式を基に作成しています。

## 重要事項

- FM方式の音色は、現在のAurelineの合成方式とは異なるため収録していません。
- 8BIT COREのTriangle系音色には、32段階のWave Memoryを使用しています。
- DISK WAVE、TABLE CHIP、EXP CHIPの一部にも32ステップのWave Memoryを使用しています。
- Envelopeは特定ハードウェアの動作再現ではなく、演奏しやすさを優先して調整しています。

## 音源別構成

### 8BIT CORE（16音色）

パルス2ch、32段階Triangle、Noise、量子化サンプル風の波形を使用。

### DISK WAVE（8音色）

波形メモリと変調による音色を32ステップWave Memoryで表現。

### CART PULSE（6音色）

矩形波2chとPCM機能を、パルス波および段階的Wave Memoryで近似。

### WAVETABLE（8音色）

4-bit可変ウェーブテーブルを32ステップWave Memoryで近似。

### PSG（6音色）

矩形波、Noise、短いEnvelopeを組み合わせたPSG風の音色。

### EXPANSION CHIP（6音色）

デューティ比可変パルス2chと7段階Sawを近似。

## 全50音色

| Slot | 音色名 | 系統 | 波形構成 | Attack | Decay | Sustain | Release |
|---:|---|---|---|---:|---:|---:|---:|
| 01 | CORE PULSE 12.5 | 8BIT CORE | Pulse 12.5% | 0.002s | 0.120s | 0.82 | 0.060s |
| 02 | CORE PULSE 25 | 8BIT CORE | Pulse 25% | 0.002s | 0.120s | 0.82 | 0.060s |
| 03 | CORE SQUARE 50 | 8BIT CORE | Pulse 50% | 0.002s | 0.120s | 0.82 | 0.060s |
| 04 | CORE PULSE 75 | 8BIT CORE | Pulse 75% | 0.002s | 0.120s | 0.82 | 0.060s |
| 05 | CORE TRI BASS | 8BIT CORE | 32-step Wave Memory | 0.002s | 0.120s | 0.90 | 0.060s |
| 06 | CORE NOISE HIT | 8BIT CORE | Noise | 0.002s | 0.130s | 0.00 | 0.030s |
| 07 | CORE SAMPLE BITE | 8BIT CORE | 32-step Wave Memory | 0.002s | 0.200s | 0.45 | 0.060s |
| 08 | CORE DUAL LEAD | 8BIT CORE | Pulse 25% + Pulse B（+12半音） | 0.002s | 0.120s | 0.82 | 0.060s |
| 09 | CORE BOSS BASS | 8BIT CORE | Pulse 25% + Pulse B（-12半音） | 0.002s | 0.120s | 0.82 | 0.060s |
| 10 | CORE FANFARE | 8BIT CORE | Pulse 12.5% + Pulse B（+12半音） | 0.002s | 0.120s | 0.82 | 0.060s |
| 11 | CORE OCT LEAD | 8BIT CORE | Pulse 25% + Pulse B（+12半音） | 0.002s | 0.180s | 0.76 | 0.080s |
| 12 | CORE SEMI ALERT | 8BIT CORE | Pulse 25% + Pulse B（+1半音） | 0.002s | 0.160s | 0.78 | 0.070s |
| 13 | CORE DUTY DUO | 8BIT CORE | Pulse 12.5% + Pulse B（+0半音） | 0.002s | 0.140s | 0.82 | 0.050s |
| 14 | CORE DETUNE DUO | 8BIT CORE | Pulse 25% + Pulse B（+0.07半音） | 0.002s | 0.200s | 0.80 | 0.120s |
| 15 | CORE SLOW ATTACK | 8BIT CORE | Pulse 25% + Pulse B（+12半音） | 0.180s | 0.450s | 0.74 | 0.420s |
| 16 | CORE REL LEAD | 8BIT CORE | Pulse 12.5% + Pulse B（+12半音） | 0.003s | 0.200s | 0.72 | 0.720s |
| 17 | DISK PURE WAVE | DISK WAVE | 32-step Wave Memory | 0.002s | 0.120s | 0.90 | 0.060s |
| 18 | DISK HOLLOW LEAD | DISK WAVE | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |
| 19 | DISK BRASS | DISK WAVE | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |
| 20 | DISK BELL MOD | DISK WAVE | 32-step Wave Memory | 0.002s | 0.520s | 0.12 | 0.650s |
| 21 | DISK WOOD BASS | DISK WAVE | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |
| 22 | DISK VOCAL WAVE | DISK WAVE | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |
| 23 | DISK MOD SWELL | DISK WAVE | 32-step Wave Memory | 0.320s | 0.800s | 0.75 | 0.900s |
| 24 | DISK CRYSTAL | DISK WAVE | 32-step Wave Memory | 0.002s | 0.400s | 0.24 | 0.550s |
| 25 | CART PULSE LEAD | CART PULSE | Pulse 25% | 0.002s | 0.120s | 0.82 | 0.060s |
| 26 | CART SQUARE LEAD | CART PULSE | Pulse 50% | 0.002s | 0.120s | 0.82 | 0.060s |
| 27 | CART DUAL PULSE | CART PULSE | Pulse 12.5% + Pulse B（+12半音） | 0.002s | 0.120s | 0.82 | 0.060s |
| 28 | CART PULSE BASS | CART PULSE | Pulse 25% | 0.002s | 0.120s | 0.82 | 0.060s |
| 29 | CART PCM STEP | CART PULSE | 32-step Wave Memory | 0.002s | 0.280s | 0.58 | 0.060s |
| 30 | CART PCM BRASS | CART PULSE | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |
| 31 | TABLE SINE 4BIT | WAVETABLE | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |
| 32 | TABLE MAZE | WAVETABLE | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |
| 33 | TABLE HOLLOW | WAVETABLE | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |
| 34 | TABLE SAW | WAVETABLE | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |
| 35 | TABLE PULSE | WAVETABLE | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |
| 36 | TABLE BASS | WAVETABLE | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |
| 37 | TABLE CHOIR | WAVETABLE | 32-step Wave Memory | 0.180s | 0.120s | 0.82 | 0.550s |
| 38 | TABLE BRIGHT | WAVETABLE | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |
| 39 | PSG SQUARE LEAD | PSG | Pulse 50% | 0.002s | 0.120s | 0.82 | 0.060s |
| 40 | PSG OCT STACK | PSG | Pulse 50% + Pulse B（+12半音） | 0.002s | 0.120s | 0.82 | 0.060s |
| 41 | PSG BASS | PSG | Pulse 50% | 0.002s | 0.120s | 0.82 | 0.060s |
| 42 | PSG NOISE TONE | PSG | Pulse 50% | 0.002s | 0.120s | 0.82 | 0.060s |
| 43 | PSG ENV PLUCK | PSG | Pulse 50% | 0.002s | 0.160s | 0.00 | 0.040s |
| 44 | PSG METAL NOISE | PSG | Pulse 50% + Pulse B（+0.07半音） | 0.002s | 0.220s | 0.18 | 0.060s |
| 45 | EXP PULSE 1/16 | EXPANSION CHIP | Pulse 6.25% | 0.002s | 0.120s | 0.82 | 0.060s |
| 46 | EXP PULSE 2/16 | EXPANSION CHIP | Pulse 12.5% | 0.002s | 0.120s | 0.82 | 0.060s |
| 47 | EXP PULSE 3/16 | EXPANSION CHIP | Pulse 18.75% | 0.002s | 0.120s | 0.82 | 0.060s |
| 48 | EXP PULSE 4/16 | EXPANSION CHIP | Pulse 25% | 0.002s | 0.120s | 0.82 | 0.060s |
| 49 | EXP 7STEP SAW | EXPANSION CHIP | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |
| 50 | EXP SAW BASS | EXPANSION CHIP | 32-step Wave Memory | 0.002s | 0.120s | 0.82 | 0.060s |

## 追加した矩形波2ch音色

- **CORE OCT LEAD**：同じデューティ比の2chを1オクターブで重ねた定番リード。
- **CORE SEMI ALERT**：第2パルスを半音上に配置した警告音・ボス場面向け音色。
- **CORE DUTY DUO**：12.5%と50%を重ね、細い成分と芯を両立。
- **CORE DETUNE DUO**：同じ25%パルスを7 centずらした厚い近似音色。
- **CORE SLOW ATTACK**：矩形波2chを緩やかに立ち上げるパッド／エンディング向け。
- **CORE REL LEAD**：鍵盤を離した後に長く残る、Aureline独自の演奏向け音色。

## 読み込み方法

`8-Bit.aurelinelibrary.xml`はAurelineのインストール時に
プリセットライブラリとして用意されます。Aurelineの **LOAD** からこの
ファイルを選択すると、確認後に現在の50スロットがすべて置き換わります。
必要なライブラリは先にSAVE ALL LIBRARYでバックアップしてください。
