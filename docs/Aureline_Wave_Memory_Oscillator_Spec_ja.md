# Aureline WAVE MEMORY OSCILLATOR仕様

- 文書バージョン: 0.1
- 対象: Aureline Mac版 / iPhone版 / AUv3版
- ステータス: 実装前仕様
- 基準実装: Aureline共有C++音源エンジン

## 1. 目的

AurelineのOSC A / OSC Bへ、32ステップの単周期波形を再生する`WAVE MEMORY`を追加する。

PC Engine系の明るく粗いデジタル波形音、および『ドルアーガの塔』の時代を想起させるNamco WSG系の素朴なウェーブテーブル音を、Aurelineのアナログ・フィルター、エンベロープ、LFO、Poly Modと組み合わせて使用できるようにする。

特定機種の完全なエミュレーションやゲーム音源ROMの複製は目的としない。Aureline独自の波形、編集機能、音色バンクを提供する。

## 2. 技術的な参照点

- PC EngineのHuC6280系PSGで一般的に使われる波形再生は、32サンプル・5bitの波形メモリーを基礎とする。
- 初期Namco WSG系は、32ステップ・4bitの単周期波形を基礎とする。
- 32ステップという短い周期と低い振幅分解能によって、滑らかなアナログ波形とは異なる倍音と量子化感が生まれる。

参考:

- [HuC6280の概要と32サンプル・5bit波形](https://en.wikipedia.org/wiki/Hudson_Soft_HuC6280)
- [Namco WSG: 32ステップ・4bit波形](https://vgmrips.net/wiki/Namco_WSG)
- [Namco WSGの波形テーブル構造](https://walkofmind.com/programming/pie/wsg3.htm)

## 3. 製品上の位置付け

WAVE MEMORYはサンプラーではなく、OSCの1波形として動作する。

```text
SAW ───────┐
TRIANGLE ──┤
PULSE ─────┼─ OSC MIX ─ LEVEL ─ FILTER ─ AMP ENV
WAVE MEM ──┘
```

- OSC AとOSC Bがそれぞれ独立した32ステップ波形を保持する。
- SAW / TRIANGLE / PULSEと同時選択できる。
- 既存波形との合成時は、現在と同じく有効波形数に応じてゲインを補正する。
- OSC RANGE、DETUNE、SYNC、OSC B LF、OSC B KBは既存OSCと同じように作用する。
- Mac版、iPhone版、AUv3版は同じ共有エンジンとパラメータを使用する。

## 4. 波形データ

### 4.1 内部形式

各OSCにつき次のデータを保持する。

| 項目 | 仕様 |
|---|---|
| ステップ数 | 32固定 |
| 保存形式 | 符号なし5bit相当、`0...31` |
| DSP内部値 | `-1.0...+1.0`へ正規化 |
| 初期値 | 独自の丸みを持つSaw系波形 |
| DC補正 | 波形読み込み時に平均値を除去 |
| 正規化 | 最大絶対値が1.0を超えないよう補正 |

パッチにはOSC A用32値とOSC B用32値を個別に保存する。

### 4.2 CHARACTER

再生時の振幅分解能を切り替える`CHARACTER`を設ける。

| 表示 | 内部処理 | 用途 |
|---|---|---|
| `5-BIT` | 0〜31の32段階 | PC Engine系を想起させる細かい波形 |
| `4-BIT` | 0〜15の16段階へ再量子化 | 初期Namco WSG系を想起させる粗い波形 |
| `SMOOTH` | 5bitデータを線形補間 | Aureline独自の現代的な音色 |

デフォルトは`5-BIT`とする。

### 4.3 補間

- `5-BIT`および`4-BIT`はゼロ次ホールドを基本とする。
- `SMOOTH`のみ隣接ステップ間を線形補間する。
- 高音域の強いエイリアシングはキャラクターの一部として残す。
- 安全性のため、ナイキスト周波数付近では既存OSCと同じ周波数上限を適用する。
- 将来、`CLEAN / RAW`切替やオーバーサンプリングを追加できる構造にするが、初期実装には含めない。

## 5. OSCパラメータ

### 5.1 新規パラメータ

| ID | 表示名 | 範囲 | 初期値 |
|---|---|---:|---:|
| `waveMemoryAEnabled` | WAVE MEM | OFF / ON | OFF |
| `waveMemoryBEnabled` | WAVE MEM | OFF / ON | OFF |
| `waveMemoryAIndex` | MEMORY | 0〜15 | 0 |
| `waveMemoryBIndex` | MEMORY | 0〜15 | 0 |
| `waveMemoryACharacter` | CHARACTER | 5-BIT / 4-BIT / SMOOTH | 5-BIT |
| `waveMemoryBCharacter` | CHARACTER | 5-BIT / 4-BIT / SMOOTH | 5-BIT |
| `waveMemoryAData` | WAVE DATA A | 32×0〜31 | 初期波形 |
| `waveMemoryBData` | WAVE DATA B | 32×0〜31 | 初期波形 |

`waveMemoryAIndex`と`waveMemoryBIndex`はファクトリーバンク選択用である。ユーザーが波形を編集した時点で表示を`USER`へ切り替え、編集済み32値を音色へ保存する。

### 5.2 既存パラメータとの関係

- `RANGE`: WAVE MEMORYにも適用する。
- `DETUNE`: OSC BのWAVE MEMORYにも適用する。
- `PW`: WAVE MEMORYには適用しない。
- `SYNC`: OSC Bの位相リセット時にWAVE MEMORYの読み出し位相もリセットする。
- `OSC B LF`: WAVE MEMORYを低周波の周期波形として使用できる。
- `OSC B KB`: OFFの場合は既存OSC Bと同じ固定周波数動作とする。
- LFO Pitch、Poly Mod Pitch: 既存OSCと同じ周波数変調を受ける。
- LFO PW、Poly Mod PW: WAVE MEMORYには作用しない。

## 6. ファクトリー波形バンク

初期バンクは16波形とし、すべてAureline用に新規作成する。

| No. | 名称 | 性格 |
|---:|---|---|
| 01 | SOFT SINE | 基音中心 |
| 02 | HOLLOW | 奇数倍音中心 |
| 03 | BRIGHT 5 | 明るい5bit波形 |
| 04 | REED | リード向け |
| 05 | ORGAN | オルガン系 |
| 06 | BELL | 非対称倍音 |
| 07 | METAL | 硬い高域 |
| 08 | VOCAL A | 母音的な山形 |
| 09 | VOCAL O | 低域の太い母音感 |
| 10 | BASS STEP | 低音向け |
| 11 | ARCADE 1 | 初期ゲーム音源を想起させる波形 |
| 12 | ARCADE 2 | 鋭いアタック向け |
| 13 | MAZE | 4bitで輪郭が出る波形 |
| 14 | TOWER | 素朴なリード向け |
| 15 | PULSE MIX | 複数パルス構造 |
| 16 | NOISY EDGE | 不規則な段差を持つ波形 |

既存ゲームの波形ROM、波形PROM、楽曲データを収録しない。名称も特定タイトルや企業の公式素材と誤認されない一般的な名称を使用する。

## 7. Mac版UI

### 7.1 OSCパネル

- SAW / TRIANGLE / PULSEの右側へ`WAVE MEM`ロッカースイッチを追加する。
- OSC A、OSC Bとも同じ並びにする。
- 横幅が不足する場合は波形スイッチを4個横一列に詰めず、2×2配置を許可する。
- `WAVE MEM`を有効にすると、OSCパネル内または専用ポップアップから`MEMORY`と`CHARACTER`を選択できる。

### 7.2 波形エディター

VOICEセレクタとは別の`WAVE`ボタンでエディターを開く。

```text
┌ WAVE MEMORY A ───────────────────────────────┐
│ MEMORY [03 BRIGHT 5 ▼]  CHARACTER [5-BIT ▼] │
│                                             │
│ ▃▅▇█▇▅▃▂▃▅▇█▇▅▃▂ ... 32 STEPS             │
│                                             │
│ [DRAW] [SMOOTH] [NORMALIZE] [COPY] [PASTE] │
│ [INIT]                              [CLOSE] │
└─────────────────────────────────────────────┘
```

- 32本の縦バーをドラッグして編集する。
- 横ドラッグ時は通過したステップを補間して、値の飛びを防ぐ。
- 5-BIT時は0〜31、4-BIT表示時は0〜15へスナップする。
- ダブルクリックで該当ステップを中央値へ戻す。

## 8. iPhone版UI

- OSCセクションの波形スイッチへ`WAVE MEM`を追加する。
- 画面内へ32ステップを常時表示せず、`WAVE`ボタンから全画面シートを開く。
- エディターは横画面の左側に32ステップ、右側にMEMORY / CHARACTER / COPY / PASTE / INITを配置する。
- 1本指ドラッグで波形を描画する。
- Mac版と同じ波形番号、名称、量子化、初期値を使用する。
- 編集中も鍵盤、外部MIDI、AUv3ホストからの演奏を継続できる。

## 9. 音色保存と互換性

### 9.1 保存対象

- WAVE MEMのON/OFF
- MEMORY番号またはUSER状態
- CHARACTER
- OSC Aの32ステップ
- OSC Bの32ステップ

### 9.2 旧音色

- 新しい保存形式のバージョンを1段階上げる。
- WAVE MEMORY項目が存在しない旧音色は、WAVE MEMをOFFとして読み込む。
- 既存SAW / TRIANGLE / PULSE、DETUNE、PWなどの値は変更しない。
- COPY / PASTE / LOAD / SAVE / STOREの全経路で32ステップを保持する。
- Mac版とiPhone版で同一のWAVE MEMORYデータを交換できる形式にする。

## 10. DSP実装方針

### 10.1 リアルタイム処理

- オーディオスレッドでメモリ確保、ロック、ファイルI/Oを行わない。
- 32ステップは`AnalogPatch`内の固定長配列として保持する。
- UI編集値は既存パッチ更新経路で音源へ渡す。
- 波形データ更新時のクリックを避けるため、旧波形から新波形へ2〜5msでクロスフェードする。
- 複数波形同時選択時は、既存の`1 / sqrt(有効波形数)`補正を維持する。

### 10.2 位相

- 既存Oscillatorの0.0〜1.0位相を共有する。
- インデックスは`floor(phase × 32)`で求める。
- SYNC、ノートON、UNISONのランダム開始位相は既存仕様を継承する。
- UNISONの5 Voiceはそれぞれ同じ波形データを使用し、現在のデチューン、発音遅延、パン、位相差を維持する。

## 11. AUv3

- WAVE MEM A/B、MEMORY、CHARACTERをAUParameterTreeへ公開する。
- 32ステップ個別値はホストオートメーション対象外とし、fullStateへ保存する。
- 波形エディター操作中は、32値をまとめて1回の編集操作としてUndo対象にする。
- GarageBandで音色を保存・再読み込みした場合もUSER波形を復元する。

## 12. テスト項目

### DSP

- 44.1 / 48 / 88.2 / 96kHzでピッチが一致する。
- 32ステップの全インデックスが周期内で正しく再生される。
- 4-BIT、5-BIT、SMOOTHの出力差を確認する。
- 波形更新中にNaN、Inf、過大出力を発生しない。
- SYNC、LF、KB、LFO Pitch、Poly Mod Pitchとの組み合わせを確認する。
- 8 Voice Polyおよび5 Voice Unisonで安定動作する。

### 保存

- Macで作成したUSER波形をiPhoneで読み込める。
- iPhoneで作成したUSER波形をMacで読み込める。
- AUv3ホストのプロジェクト再読み込みで波形が復元される。
- 旧形式の50ファクトリー音色が変化せず読み込める。

### UI

- iPhone横画面で波形スイッチやエディターがはみ出さない。
- 32ステップすべてを指で編集できる。
- Voice切替時にOSC A/Bの表示波形が正しく更新される。
- COPY / PASTE / INIT / STORE後に波形表示と実音が一致する。

## 13. 初期実装の完了条件

1. OSC A/BでWAVE MEMを既存波形と同時使用できる。
2. 16種類の独自ファクトリー波形を選択できる。
3. 4-BIT / 5-BIT / SMOOTHを切り替えられる。
4. MacとiPhoneで32ステップを編集できる。
5. Mac / iPhone / AUv3間で音色とUSER波形を保持できる。
6. 既存50音色の音が変化しない。
7. 共有エンジンテストと各プラットフォームのビルドが成功する。

