# Aureline ドラムキット演奏モード仕様

- 文書バージョン：1.0
- 対象製品バージョン：1.0.9以降
- ステータス：実装済み仕様
- 対応形式：macOS／Windows Standalone、VST3、Audio Unit、iPhone、AUv3

## 1. 概要

ドラムキット演奏モード（以下`KIT`）は、専用BANK 8「DRUM KIT」に保存された
32音色をMIDIノートへ個別に割り当て、1つのドラムセットとして演奏する
Voice Modeである。

サンプル再生は使用しない。Kick、Snare、Tom、Hi-Hat、Cymbalなどは、
AurelineのOscillator、Noise、Wave Memory、Poly Mod、Filter、Envelopeを
使用してリアルタイムに合成する。

## 2. 操作

1. PERFORMANCEまたはPOLY MODEセクションの`KIT`をオンにする。
2. MIDIキーボード、外部MIDI入力、画面鍵盤から割り当て済みノートを演奏する。
3. `KIT`をもう一度押すと`POLY`へ戻る。

`KIT`をオンにすると、通常の旋律演奏向け機能であるARPEGGIOとCHORDは
自動的にオフになる。LCDのVoice Mode表示は`KIT`になる。
デスクトップ版の画面鍵盤は、通常モードとKITモードのどちらも
C2（MIDI 36）からC5までを表示する。

## 3. 音源構造

### 3.1 音色スナップショット

KIT構築時に、BANK 8のスロット1〜32をそれぞれ独立した`AnalogPatch`として
読み込む。ノートオン時には、対応するパッチを割り当て先ボイスへコピーする。

このため、発音後に別のドラムキーを押しても、先に鳴っている音のOscillator、
Filter、Envelopeなどは別の音色へ変化しない。Kickの余韻を残したままSnareや
Hi-Hatを重ねられる。

### 3.2 ポリフォニー

- 最大同時発音数：8
- 未使用ボイスを優先
- 全ボイス使用時はRelease中の小さい音、その後に最も古い音を優先して再利用
- 各ドラム音は通常の1ボイスを使用
- UNISON処理はKIT内では使用しない

### 3.3 基準ピッチ

ドラム音色は、割り当て先MIDIノートの高さではなく、内部基準ノート
MIDI 60で発音する。したがって、低いキーへ配置したKickや高いキーへ配置した
Clavesでも、BANK 8で作成した本来のピッチを維持する。

音色内のOscillator Range、Transpose、Detune、Poly Modなどは通常どおり
基準ピッチへ反映される。

### 3.4 Velocity

受信Velocity 1〜127を各ボイスのVCA Gainへ反映する。音色のVELOCITY設定が
有効な場合はFilter Cutoffにも反映する。

### 3.5 Hi-Hat Choke

MIDI 42（Closed Hat）またはMIDI 44（Metal／Pedal Hat）を発音すると、
発音中のMIDI 46（Open Hat）をReleaseへ移行させる。

Open Hatを瞬時に強制停止するのではなく、音色に設定された最短Release処理を
通すため、不自然な波形切断やクリックを抑える。

## 4. MIDIキーマップ

MIDIノート番号を仕様上の基準とする。オクターブ名はDAWやMIDI機器によって
1オクターブ異なる場合がある。

| DRUM KITスロット | 音色名 | MIDIノート | 音名（MIDI 60=C4） |
|---:|---|---:|---|
| 1 | DEEP KICK | 36 | C2 |
| 2 | RIM SHOT | 37 | C#2 |
| 3 | CLASSIC SNARE | 38 | D2 |
| 4 | HAND CLAP | 39 | D#2 |
| 5 | TIGHT SNARE | 40 | E2 |
| 6 | LOW TOM | 41 | F2 |
| 7 | CLOSED HAT | 42 | F#2 |
| 8 | DISCO TOM | 43 | G2 |
| 9 | METAL HAT | 44 | G#2 |
| 10 | MID TOM | 45 | A2 |
| 11 | OPEN HAT | 46 | A#2 |
| 12 | ELECTRO SNARE | 47 | B2 |
| 13 | HIGH TOM | 48 | C3 |
| 14 | SHORT CYMBAL | 49 | C#3 |
| 15 | HAT PULSE | 50 | D3 |
| 16 | METAL CYMBAL | 51 | D#3 |
| 17 | ACCENT KICK | 52 | E3 |
| 18 | SHORT KICK | 53 | F3 |
| 19 | BOOM KICK | 54 | F#3 |
| 20 | TUNED KICK | 55 | G3 |
| 21 | COWBELL | 56 | G#3 |
| 22 | CLICK KICK | 57 | A3 |
| 23 | SUB DROP | 58 | A#3 |
| 24 | NOISE SNARE | 59 | B3 |
| 25 | HIGH CONGA | 62 | D4 |
| 26 | MID CONGA | 63 | D#4 |
| 27 | LOW CONGA | 64 | E4 |
| 28 | CLAVES | 65 | F4 |
| 29 | SUB BASS | 66 | F#4 |
| 30 | MUTED COWBELL | 67 | G4 |
| 31 | MARACAS | 70 | A#4 |
| 32 | TRIGGER FX | 72 | C5 |

C2〜C5の範囲でGMドラムマップの代表位置を優先する。Aureline固有の
追加音色は同じ範囲の空きキーへ配置する。MIDI 60、61、68、69、71と
範囲外のノートはKITモードでは発音しない。

## 5. BANK 8との関係

KITは専用BANK 8「DRUM KIT」の現在の32スロットを
音源として使用する。

- アプリ起動時にBANK 8からKITを構築
- BANK 8の音色を`STORE`するとKITも再構築
- ライブラリをBANK 8へ読み込むとKITも再構築
- BANK 8の表示名は`DRUM KIT`に固定
- BANK 8へライブラリを読み込んだ場合も、表示名は`DRUM KIT`を維持
- BANK 1〜7の変更はKITへ影響しない

音色の編集は通常のVoice ModeでBANK 8の対象スロットを選択して行い、
`STORE`で確定する。

## 6. パラメーターの扱い

### 音色ごとに保持するもの

- Oscillator A／B設定
- Noise Level
- Wave MemoryデータとCharacter
- Oscillator Sync、Keyboard Tracking、Low Frequency
- Poly Mod設定
- Filter Mode、Cutoff、Resonance、Envelope、Key Track、Velocity
- Amplifier Envelope
- LFO設定
- Stereo Spread、Vintage、Transient Accent

### KIT全体で共有するもの

- 最大8音のボイスプール
- Pitch Bend入力とPitch Bend Range
- Mod Wheel入力
- Sustain Pedal状態
- Master出力段

各ドラムパッチに保存されたVoice Modeは、KIT発音時には`POLY`相当として扱う。

## 7. MIDI制御

- Note On：対応するドラム音をVelocity付きで発音
- Note Off：対応ボイスをReleaseへ移行
- Sustain Pedal：通常のボイスと同じSustain処理
- Pitch Bend：発音中の全KITボイスへ適用
- Mod Wheel：各音色で有効なLFO Destinationへ適用
- Panic／All Sound Off：全KITボイスを停止

MIDIチャンネル10専用には限定しない。Standaloneでは選択されたMIDI入力、
プラグイン／AUv3ではホストから届いたMIDIノートをチャンネルに関係なく処理する。

## 8. 保存と復元

Voice Modeの`KIT`状態は通常のプラグイン状態／音色状態に含まれる。
アプリまたはホストセッションの復元後も、BANK 8から構築済みのKITを使用する。

KITそのものを単独の`.aurelinekit`ファイルとして保存する機能は持たない。
移行やバックアップにはBANK 8を`.aurelinelibrary.xml`として保存する。
内蔵ライブラリはユーザーの音色フォルダへ
`DrumKit.aurelinelibrary.xml`としてインストールする。

## 9. プラットフォーム共通性

デスクトップ版とiPhone／AUv3版は、共通のC++ `AnalogEngine`で次を処理する。

- ノート別パッチ選択
- ボイス割り当て
- 基準ピッチ固定
- Hi-Hat Choke
- 8音ポリフォニー

各UI層はBANK 8の32音色を読み取り、同一順序のMIDIキーマップをエンジンへ設定する。

## 10. 現行制約

- 1ノートへ割り当てられる音色は1つ
- キー割り当てをUIから変更する機能はない
- 音色ごとのPan、Level、TuneをKIT専用値として追加保存する機能はない
- Choke GroupはHi-Hatの1グループのみ
- MIDIチャンネル別のマルチティンバー動作は行わない
- KIT専用シーケンサー、パターン保存、Swingは搭載しない
- 割り当て外ノートの自動Transposeや代替発音は行わない

## 11. 関連文書

- [Aureline取扱説明書](Aureline_Manual_ja.md)
- [Aureline製品仕様](Aureline_Spec_ja.md)
- [DRUM KIT音色説明](../assets/Circuit_音色説明.md)
