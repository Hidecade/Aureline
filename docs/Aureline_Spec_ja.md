# Aureline 製品仕様

- 文書バージョン：1.2
- 対象製品バージョン：Desktop 1.0.12／iPhone 1.0.11 (Build 13)
- ブランド：Hidecade Instruments
- ステータス：実装済み仕様

## 1. 製品概要

Aurelineは、クラシックなポリフォニック・アナログシンセサイザーの直接的な
操作感を現代のソフトウェア音源として再構成した、8音ポリフォニック・
アナログモデリングシンセサイザーである。

2 Oscillator、Noise、4段OTA Low-pass Filter、Filter / Amplifier ADSR、LFO、
Poly Modという簡潔な構成に、32ステップWave Memory、Arpeggiator、Chord、
8バンクの音色ライブラリを統合する。

Aurelineは特定機種の完全なクローンではない。他製品のソースコード、実装定数、
回路定数、プリセット、波形ROM、外観を複製せず、Hidecade独自の音響特性、
パラメーター伝達、音色、UIを使用する。

## 2. 対応プラットフォーム

| プラットフォーム | 形式 |
|---|---|
| macOS | Standalone、VST3、Audio Unit |
| Windows | Standalone、VST3 |
| iPhone | Standalone App、AUv3 Instrument |

全形式で共通のC++音源エンジン、音色パラメーター、ステレオ出力を使用する。
デスクトップUIはJUCE、iPhone / AUv3 UIはSwiftUIとApple Audio Unit APIを使用する。

デスクトップ版はSINGLE／DUAL／MULTIルーティングを選択できる。MULTIでは
4パート・マルチティンバー、各パート最大8ボイス／全体最大16音の動的割り当てとする。
iPhone Standalone／AUv3はシングルティンバー、最大8ボイスの固定仕様とする。

iPhone / AUv3のビルド構成と現行実装は
[`iOS/AurelineMobile/README.md`](../iOS/AurelineMobile/README.md)を基準とする。

## 3. 製品境界

- Opaline FMとは別アプリ、別プラグイン、別リポジトリとする。
- FM音源、SysEx、MPE、Microtuningには対応しない。
- Opaline FMの音色、ライブラリ、状態ファイルとは互換性を持たない。
- 汎用Modulation Matrix、Step Sequencer、内蔵Effectsは搭載しない。
- 既存ハードウェアのSysExおよびプリセットとは互換性を持たない。

## 4. 信号経路

```text
Oscillator A ─┐
Oscillator B ─┼─ Mixer / Saturation ─ 4-stage OTA LPF ─ VCA
Noise ────────┘                                      │
                                                     └─ Voice Pan ─ Master ─ Output

LFO ───────── Oscillator A/B Pitch、Pulse Width A/B、Filter Cutoff

Poly Mod ──── Oscillator B + Filter Envelope
               └─ Oscillator A Frequency / Phase
               └─ Oscillator A Pulse Width
               └─ Filter Cutoff
```

Poly Modは各ボイス内で処理し、Filter、VCA、Voice Panを含む最終結果を
ステレオ出力へ送る。内蔵Effects段は持たない。

## 5. Voice Architecture

### 5.1 Polyphony

- デスクトップ：SINGLE／DUAL／MULTI、最大4パート、各パート最大8音、全体最大16音
- iPhone Standalone／AUv3：シングルティンバー、最大8音
- Voice Mode：`POLY`、`MONO`、`UNISON`、`KIT`
- KIT：専用BANK 8「DRUM KIT」の32音色をGM系MIDIノートへ割り当て、最大8音同時発音
- Hi-Hat Choke：Closed／Pedal HatでOpen Hatをリリース
- Sustain Pedal、All Notes Off、Panicに対応
- MIDI Note範囲：0〜127

デスクトップの4パートは独立した音源エンジンと音色状態を持つ。全体が16音を
超えた場合は、次の順序で1音ずつ整理する。

1. 全パート中、Release中で出力レベルが最も小さい音
2. Part 4の最も古いドラム音
3. 新しい発音を要求したパートの最も古い音
4. 最も多く発音している通常パートの最も古い音

無音パートは発音枠を消費しない。各パート内で8音を超えた場合は、そのパート内の
Release音または最古音を再利用する。

Poly時のボイス割り当ては次の優先順を使用する。

1. 未使用ボイス
2. Release中で出力レベルが小さいボイス
3. 最も古いボイス

Mono時はラストノート優先とし、複数キーを押している間は直近の保持ノートへ
戻る。Legato時はEnvelopeを不要に再トリガーせず、Glide設定を反映する。

Unison時は5ボイスを使用し、中央、左右のDetune、Pan、微小な開始遅延、
独立した開始位相を与える。出力ゲインはボイス数に応じて補正し、最終段を
ソフトサチュレーションする。

### 5.2 KIT

KITはBANK 8の32音色をノート別の独立した`AnalogPatch`として読み込み、
1つのドラムセットとして演奏する。サンプル再生は使用しない。ノートオン時に
対応パッチを割当ボイスへコピーするため、異なるドラム音の余韻を同時に保持できる。

- 最大8音、各ドラム音は1ボイス、UNISONは使用しない
- 配置キーに関係なく内部基準ノートMIDI 60で発音
- Closed Hat（42）／Metal Hat（44）でOpen Hat（46）をRelease
- KITオン時はArpeggiatorとChordをオフ
- BANK 8のSTORE／ライブラリ読込後にKITを再構築
- BANK 8の表示名は`DRUM KIT`固定

| Slot | Sound | MIDI | Slot | Sound | MIDI |
|---:|---|---:|---:|---|---:|
| 1 | DEEP KICK | 36 | 17 | ACCENT KICK | 52 |
| 2 | RIM SHOT | 37 | 18 | SHORT KICK | 53 |
| 3 | CLASSIC SNARE | 38 | 19 | BOOM KICK | 54 |
| 4 | HAND CLAP | 39 | 20 | TUNED KICK | 55 |
| 5 | TIGHT SNARE | 40 | 21 | COWBELL | 56 |
| 6 | LOW TOM | 41 | 22 | CLICK KICK | 57 |
| 7 | CLOSED HAT | 42 | 23 | SUB DROP | 58 |
| 8 | DISCO TOM | 43 | 24 | NOISE SNARE | 59 |
| 9 | METAL HAT | 44 | 25 | HIGH CONGA | 62 |
| 10 | MID TOM | 45 | 26 | MID CONGA | 63 |
| 11 | OPEN HAT | 46 | 27 | LOW CONGA | 64 |
| 12 | ELECTRO SNARE | 47 | 28 | CLAVES | 65 |
| 13 | HIGH TOM | 48 | 29 | SUB BASS | 66 |
| 14 | SHORT CYMBAL | 49 | 30 | MUTED COWBELL | 67 |
| 15 | HAT PULSE | 50 | 31 | MARACAS | 70 |
| 16 | METAL CYMBAL | 51 | 32 | TRIGGER FX | 72 |

MIDI 60、61、68、69、71および範囲外ノートは発音しない。KIT状態は通常の
プラグイン状態へ保存する。独立したKITファイルは持たず、BANK 8を
`.aurelinelibrary.xml`として保存・移行する。キー割り当て編集、複数Choke Group、
KIT専用Sequencerは持たない。

### 5.3 Velocity

VelocityはVCA Gainへ反映する。Filterの`VELOCITY`設定に応じて、
Filter Cutoffにも加算する。

## 6. Oscillators

Oscillator A / Bは独立した位相を持ち、次の波形を同時選択できる。

- Saw
- Triangle
- Pulse
- Wave Memory

複数波形を選択した場合は、有効波形数の平方根でゲインを補正する。

### 6.1 共通仕様

- Range：32′、16′、8′、4′、2′
- Pulse Width：0.02〜0.98
- Oscillator Level：0〜1
- Saw / Pulseの不連続点にPolyBLEP処理を適用
- 発振周波数をSample Rateの45%以下へ制限
- パラメーター変更時のLevel / Pulse Widthを平滑化

### 6.2 Oscillator A

- 主Oscillator
- Poly ModのFrequency / PhaseおよびPulse Width Destination
- Hard Sync時のSlave

### 6.3 Oscillator B

- 第2音源
- Fine Tune：-100〜+100 cents
- `LF`：Low Frequency Mode
- `KB`：Keyboard Tracking On / Off
- Poly Modのオーディオ信号Source
- Hard Sync時のMaster

Oscillator BのMixer LevelはPoly Mod量へ影響しない。Mixer Levelが0でも、
Oscillator BはPoly ModとHard SyncのSourceとして動作する。

### 6.4 Hard Sync

`SYNC`がオンの場合、Oscillator Bの位相が一周するたびにOscillator Aの位相を
リセットする。Oscillator BがMaster、Oscillator AがSlaveである。

Filter EnvelopeからPoly Mod `FREQ A`へ送ることで、Oscillator A側の周波数を
時間変化させたSync Sweepを作成できる。

## 7. Wave Memory

Oscillator A / Bは、それぞれ独立した32ステップの単周期波形を保持する。

| 項目 | 仕様 |
|---|---|
| ステップ数 | 32 |
| 保存値 | 0〜31 |
| Factory波形 | 16種類 |
| Character | `5-BIT`、`4-BIT`、`SMOOTH` |

- `5-BIT`：32段階の値をZero-order Holdで再生
- `4-BIT`：16段階へ再量子化してZero-order Holdで再生
- `SMOOTH`：隣接ステップ間を線形補間

Wave MemoryはSaw / Triangle / Pulseと同時使用できる。Range、Detune、Pitch
Modulation、Hard Sync、Oscillator BのLF / KB設定を通常波形と同様に反映する。
Pulse Width ModulationはWave Memory自体の形状には作用しない。

Wave Editorは描画、ステップ選択、値編集、Copy / Paste、Init、Audition、
`.aurelinewave`のLoad / Saveに対応する。編集した32値は音色へ保存する。

## 8. Mixer

- Oscillator A Level
- Oscillator B Level
- Noise Level

OscillatorとNoiseを加算後、`tanh`による連続的な入力サチュレーションを適用して
Filterへ送る。Noiseはボイス単位で生成する。

## 9. Filter

Aurelineは、4段OTA積分器とグローバルなResonance Feedbackを持つ
24 dB/oct Low-pass Filterを各ボイスに搭載する。

- Cutoff：20 Hz〜20 kHz相当
- Resonance：0〜1
- Filter Envelope Amount：正負
- Keyboard Tracking：連続量
- Velocity Amount：連続量
- 最大付近のResonanceで自己発振
- Filter内部：2倍Oversampling

各OTA段の差動入力をSoft Saturationし、Ladder型とは異なるOverdrive特性を持つ。
微小な決定論的Noise Floorにより、高Resonance時のFeedback発振を開始する。
高Resonanceでは入力を緩やかに補償するが、共振型Low-pass特有の低域減少は残す。

Cutoff、Resonanceを平滑化し、内部状態と出力を有限範囲に制限する。
Rev 1/2、Rev 3などのモデル切替は持たず、Aureline独自の単一OTAモデルとする。

Filter Cutoffには次をOctave領域で加算する。

- Filter Envelope
- LFO
- Keyboard Tracking
- Velocity
- Poly Mod
- Vintage Voice Variation

## 10. Envelopes

各ボイスにFilter ADSRとAmplifier ADSRを持つ。

- Attack：0.0001〜30 seconds
- Decay：0.0001〜30 seconds
- Sustain：0〜1
- Release：0.0001〜30 seconds

指数的な時間変化を使用する。Amplifier Envelopeは、パネル上の最短設定を維持しつつ
不連続なGain変化を防ぐため、実レンダー時のAttack / Release Transitionを
最低6 msへ制限する。これにより、短いAttackの音を連打した場合のClickを抑える。

## 11. LFO

波形：

- Triangle
- Saw Up
- Saw Down
- Square
- Sample & Hold

パラメーター：

- Rate：0.01〜30 Hz
- Initial Amount
- Mod Wheel Amount
- Delay：0〜10 seconds
- Fade：0〜10 seconds
- Retrigger

Destination：

- Oscillator A Frequency
- Oscillator B Frequency
- Pulse Width A
- Pulse Width B
- Filter Cutoff

複数LFO波形を同時に選択できる。LFOはグローバルに生成し、各ボイスの
Destinationへ適用する。Retrigger時はNote Onで位相をリセットする。

## 12. Poly Mod

Poly Modは各ボイス内で、Oscillator BとFilter EnvelopeをSourceとし、
選択されたDestinationへ個別量で送る。

### 12.1 Source

- Oscillator B
- Filter Envelope

### 12.2 Destination

- Oscillator A Frequency / Phase
- Oscillator A Pulse Width
- Filter Cutoff

現行状態では、UIの2つのSourceノブと3つのDestinationボタンを、内部で6つの
Source-to-Destination量へ展開して保存する。

### 12.3 変調方式

Oscillator BとFilter Envelopeを同じPitch式へ単純加算せず、Sourceの性質に
合わせて処理を分けるハイブリッド方式を使用する。

#### Oscillator B → Oscillator A

Oscillator BのBipolar音声波形をOscillator Aの読み出し位相へ加算する。

```text
phaseOffset = OscillatorB × 0.65 × Amount²
```

Amountを二乗することで低い設定の調整幅を確保し、上半分で急激に深くする。
この経路はFrequency Modulationに近い音響効果を持つPhase Modulationであり、
Oscillator Bを可聴周波数で動かすとBell、Metal、Cross-modulation音を生成する。

#### Filter Envelope → Oscillator A

```text
source = FilterEnvelope × Amount² × 4
frequency = BaseFrequency × nonlinearMultiplier(source)
```

Multiplierは中央付近の音程感を保ち、外側で変調幅が急激に広がるAureline独自の
非線形カーブを使用する。入力は±4へ制限する。この経路はSync Sweepや急激な
Pitch Envelopeに使用する。

#### Pulse Width A

Oscillator BとFilter EnvelopeのSource信号を加算し、Pulse Widthへ
`-0.25 × signal`を加える。最終Pulse Widthは0.02〜0.98へ制限する。

#### Filter Cutoff

Oscillator BとFilter EnvelopeのSource信号を加算し、1.5倍したOctave変調として
Filter Cutoffへ加える。Poly Mod成分は±4 octavesへ制限する。

### 12.4 Source波形

Oscillator BのSaw、Triangle、Pulse、Wave Memory、および複数波形を合成した結果を
Sourceとして使用する。LF / KB / Range / Detuneを反映する。

係数、非線形カーブ、Phase Modulation方式はAureline独自仕様であり、
他製品の実装式や固有定数を移植しない。

## 13. Vintage

`VINTAGE`は各ボイスに固定されたVariationを連続的に増加させる。

- Oscillator A / B Tune
- 低速Pitch Drift
- Filter Cutoff
- Envelope Time
- Gain
- Pan

Variationは発音ごとに無秩序に再生成せず、各ボイスの個体差として保持する。

## 14. Performance

- Pitch Bend
- Pitch Bend Range：0〜24 semitones
- Mod Wheel
- Transpose：-24〜+24 semitones
- Master Tune：-100〜+100 cents
- Glide：0〜5 seconds
- Legato Only
- Unison Detune：0〜100 cents
- Stereo Spread
- Master Gain

## 15. Arpeggiator / Chord / Hold

Arpeggiator、Chord、HoldはデスクトップとiPhoneで共通の
`PerformanceSequencer`を使用する。

- Tempo
- Rate
- Direction：Up、Down、Up/Down、Random
- Gate
- Scale Root
- Hold
- Diatonic 3-note Chord

Arpeggiatorを使用しない場合も、Chord設定を通常演奏へ適用できる。

## 16. Stereo Output

各VoiceはEqual-power Panで左右へ配置する。Stereo Spread、Unison Pan、
Vintage Pan Variationを合成して有効Panを決定する。

Master Gain適用後、非有限値を0へ置換し、最終出力を-1〜+1へ制限する。
Unison時のみ専用のSoft SaturationとGain補正を適用する。

## 17. 音色ライブラリ

8つの書換可能バンクを搭載する。

| Bank | Name | 構成 |
|---:|---|---|
| 1 | Analog 1 | Brass、Strings / Pad、Piano / Keys、Bass、SE |
| 2 | Analog 2 | Lead、Poly Mod / Sync、Percussion、Rhythm、SE |
| 3 | Circuit | Analog Drum / Percussion系 |
| 4 | Retro | Vintage Game / Arcade系 |
| 5 | 8-Bit | Pulse、Noise、Wave Memory、PSG系 |
| 6 | Bank 6 | User Bank |
| 7 | Bank 7 | User Bank |
| 8 | DRUM KIT | KIT専用32音色 |

各バンクは32音色、合計256スロットとする。BANK 8はKIT専用である。

操作：

- `LOAD`：単一音色を一時読込、または選択先の32音色バンクを置換
- `SAVE`：現在の単一音色を外部ファイルへ保存
- `COPY` / `PASTE`：音色を一時的に複製
- `INIT`：初期音色を一時適用
- `STORE`：現在の編集内容を選択スロットへ確定保存
- `SAVE BANK`：現在の32音色バンクを保存

Version 2ライブラリのルート`name`属性を最大16文字のバンク表示名として扱う。
ライブラリ読込時に音色とバンク名を読み込み先へ保存し、デスクトップ版の音色
メニュー／LCDおよびiPhone版のBANK選択UIへ反映する。`name`がない場合は
ライブラリのファイル名を使用し、既存バンクでは従来の既定名へフォールバックする。

ユーザーが編集した実行時バンクを、アプリ更新時に無条件で上書きしない。

## 18. ファイル形式

| データ | 拡張子 | 形式 |
|---|---|---|
| 単一音色 | `.aurelinevoice` | 共通JSON |
| 音色バンク | `.aurelinelibrary.xml` | Version 2 XML、32音色 |
| Wave Memory | `.aurelinewave` | 32ステップ波形とCharacter |
| 録音 | `.wav` | 24-bit Stereo PCM |

単一音色にはSynth Parameter、Performance、Name、Wave Memory A / Bを保存する。
未知フィールドは後方互換性のため無視し、欠落値はDefaultで補完する。
読込後は`normalizePatch()`を通して範囲外値を制限する。

## 19. WAV録音

macOS / Windows Standalone版は最終ステレオ出力を録音できる。

- `WAV`で録音開始
- 録音中は`STOP`表示
- `STOP`後に保存先を選択
- 現在のAudio Sample Rateを使用
- 24-bit Stereo WAVとして保存

Audio Threadは事前確保したRing Bufferへ書き込む。データ集約とファイル書き込みは
Audio Thread外で行う。音声フレームが記録されていない場合は空ファイルを作成せず、
`WAV recording is empty`を表示する。

プラグイン版とiPhone版は内蔵WAV録音を持たず、HostまたはOS側の録音機能を使用する。

## 20. MIDI

デスクトップ版は次のMIDIルーティングを使用する。

- SINGLE：Part 1へ全MIDIチャンネルを送る
- DUAL：Part 1／2へ全MIDIチャンネルを送る
- MULTI：次表のチャンネルで4パートへ分配する

MULTIのMIDIパート割り当て：

| Part | MIDI Channel | 用途 |
|---:|---:|---|
| 1 | 1 | 通常音色 |
| 2 | 2 | 通常音色 |
| 3 | 3 | 通常音色 |
| 4 | 10 | BANK 8 DRUM KIT |

上記以外のMIDIチャンネルは発音しない。Pitch Bend、CC1、CC64、
All Notes Offは受信チャンネルのパートだけへ送る。

Part 1〜3はCC0値0〜6をBANK 1〜7、Program Change値0〜31をVOICE 1〜32として
受信する。CC0を省略したProgram Changeは各パートで最後に選択したバンクを使う。
Part 4はBANK 8固定で、CC0とProgram Changeを無視する。音色は事前にメモリへ
読み込み、MIDI受信時にAudio ThreadからファイルI/Oを行わない。

iPhone Standalone／AUv3ではMIDIチャンネル別パート分離、Bank Selectによる
マルチティンバー制御を行わない。

| MIDI | 動作 |
|---|---|
| Note On / Off | 発音 / 消音 |
| Velocity | VCAおよび設定に応じてFilter |
| Pitch Bend | 設定範囲内のPitch変化 |
| CC 1 | Mod Wheel |
| CC 64 | Sustain Pedal |
| CC 120 / 123 | Panic / All Notes Off |

Standalone版は選択されたMIDI入力を表示する。複数入力が有効な場合は入力名を列挙し、
全入力使用時は`MIDI: all inputs`と表示する。プラグイン / AUv3版のMIDI経路は
Hostが管理する。

## 21. UI

### Desktop

- 主要音源Parameterを1画面へ配置
- 信号経路に沿った左から右の構成
- 暗色Panel、Amber / Gold Accent
- FINAL MIX、Oscillator A / B Wave Monitor
- Audio Device / MIDI Input Status
- WAV録音、Bank保存
- Wave EditorはModal Panel
- SINGLE／DUAL／MULTI選択、Part選択UI、パート別MIDI Activity LED

`FINAL MIX`は、演奏されたNote OnをTriggerとして表示区間を整列し、
Poly Mod、Filter、VCAを含む最終波形を表示する。視認性のため表示Gainを持ち、
音量Meterとしては使用しない。

### iPhone

- Landscape専用
- Play / Edit切替
- 固定された画面鍵盤
- Section単位の編集画面
- タッチ操作用Wave Editor
- Safe Areaを考慮したレイアウト

デスクトップとiPhoneで音源Parameterの意味と保存形式を共通化し、
画面密度と入力方式のみを各Platformへ最適化する。

デスクトップ状態には4パートそれぞれの音色、音色名、バンク、スロット、
選択中パート、ルーティングモードを保存する。旧単音色状態はPart 1へ復元する。iPhone／AUv3は
単一音色状態を使用する。

## 22. リアルタイム要件

- Audio Threadで動的メモリ確保を行わない。
- Audio Threadで待機Lock、File I/O、UI操作を行わない。
- UI / MIDIからの変更はRealtime-safeな経路でEngineへ渡す。
- Level、Pulse Width、Cutoff、Resonance、Master Gainを平滑化する。
- NaN、Inf、発散した出力を生成しない。
- 44.1、48、88.2、96 kHzと可変Block Sizeへ対応する。
- Offline Renderでも同じEngineとParameter処理を使用する。

## 23. 実装基準

現行仕様とコードが不一致の場合は、Desktop v1.0.12／iPhone v1.0.11の実装を基準とする。

- Patchと値域：`Source/Engine/AnalogPatch.*`
- Voice信号処理：`Source/Engine/AnalogVoice.*`
- Voice割当とStereo出力：`Source/Engine/AnalogEngine.*`
- Oscillator：`Source/DSP/Oscillator.*`
- Filter：`Source/DSP/ProphetOtaFilter.*`
- LFO / Envelope：`Source/DSP/Lfo.*`、`Source/DSP/Envelope.*`
- Arpeggiator / Chord：`Source/Engine/PerformanceSequencer.*`
- Desktop UI：`Source/App/MainComponent.*`
- iPhone / AUv3：`iOS/AurelineMobile/`

ユーザー操作は[`Aureline_Manual_ja.md`](Aureline_Manual_ja.md)を参照する。
