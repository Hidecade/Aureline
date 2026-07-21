# Aureline 製品仕様

- 文書バージョン: 0.1
- 製品バージョン: 0.1.0（開発中）
- ブランド: Hidecade Instruments

## 1. 製品概要

Aurelineは、クラシックなポリフォニック・アナログシンセサイザーの直接的な操作感を現代のソフトウェア音源として再構成する、8音ポリフォニック・アナログモデリングシンセサイザーである。

特定機種の完全なクローンではない。2VCO、4-poleローパスフィルター、2基のADSR、LFO、Poly Modという簡潔な構成を採用し、Hidecade独自の音響特性と外観を持つ製品とする。

## 2. 製品境界

- Opaline FMとは別アプリ、別プラグイン、別Gitリポジトリとする。
- FM音源、SysEx、Opaline音色ライブラリとの互換性は持たせない。
- パッチ形式、Bundle ID、プラグインID、プリセット保存先を分離する。
- 実機固有の製品名、ロゴ、パネル外観、プリセットは複製しない。

## 3. 対応形式

初期リリースではmacOSおよびWindowsのStandalone、VST3、macOS Audio Unitを対象とする。MIDI入力とステレオ出力に対応する。iOSおよびAUv3は将来対応とする。

## 4. 信号経路

```text
VCO A ─┐
       ├─ Mixer ─ 4-pole LPF ─ VCA ─ Voice Pan ─ Effects
VCO B ─┤
Noise ─┘

LFO ───── Pitch / Pulse Width / Filter
Poly Mod ─ VCO B / Filter Envelope → VCO A Pitch / Filter
```

## 5. 音源仕様

### Polyphony

- 最大同時発音数: 8
- Voice Mode: Poly、Mono、Unison
- Poly時は空きボイス、Release中の最小音量、最古ボイスの順で割り当てる。
- Sustain Pedal、All Notes Off、Panicに対応する。

### VCO A

- Range: 32′、16′、8′、4′、2′
- Wave: Saw、Pulse（同時選択可能）
- Pulse Width
- Hard Sync
- Mixer Level

### VCO B

- Range: 32′、16′、8′、4′、2′
- Fine Tune
- Wave: Saw、Triangle、Pulse（同時選択可能）
- Pulse Width
- Low Frequency Mode
- Keyboard Tracking On/Off
- Mixer Level

波形生成にはPolyBLEPまたは同等の帯域制限処理を使用する。高音域およびSync使用時はオーバーサンプリングを検討する。

### Mixer

- VCO A Level
- VCO B Level
- Noise Level
- 入力レベルに応じた連続的なサチュレーション

### Filter

- Resonant 4-pole Low-pass
- Cutoff
- Resonance
- Filter Envelope Amount（正負）
- Keyboard Tracking: Off、Half、Full
- Velocity Sensitivity
- 自己発振

高レゾナンス時に発散せず、44.1〜96 kHzで一貫した動作をすること。Cutoff、Resonance、Envelope Amountを平滑化する。

### Envelopes

Filter ADSRおよびAmplifier ADSRをボイスごとに保持する。時間変化にはアナログ回路を意識した指数カーブを使用し、ノート再トリガー時のクリックを抑制する。

### LFO

- Wave: Triangle、Saw Up、Saw Down、Square、Sample & Hold
- Rate、Delay、Fade In、Retrigger
- Destination: VCO A/B Pitch、Pulse Width、Filter Cutoff

### Poly Mod

- Source: Filter Envelope、VCO B
- Destination: VCO A Frequency、Filter Cutoff
- グローバルではなく各ボイス内で処理する。

### Vintage

単一のVintageパラメータにより、ボイスごとのVCO tuning、drift、filter cutoff、envelope time、gain、panの差を段階的に増加させる。個体差は発音ごとに無秩序に変えず、各ボイスに固有の値として保持する。

## 6. Performance

- Pitch Bendおよび設定可能なBend Range
- Mod Wheel
- Master Tune
- Transpose ±24 semitones
- GlideおよびLegato Only
- Unison DetuneおよびStereo Spread

## 7. Effects

初期版はStereo Chorus、Delay、Reverb、全体Bypassを搭載する。エフェクトなしでも製品の音色が成立することを優先する。

## 8. プリセット

拡張子は `.aurelinepreset`、ライブラリは `.aurelinelibrary` とする。全音源パラメータ、Voice Mode、Vintage、Effects、Name、Author、Category、Format Versionを保存する。

## 9. UI

- 原則1画面1ノブ方式とする。
- 信号経路を左から右へ配置する。
- Oscillator、Mixer、Filter、Envelope、Modulationを明確に区切る。
- 暗色パネルにアンバー／ゴールド系アクセントを用いる。
- 実機固有の外観や木目パネルを直接複製しない。
- 主要音源パラメータは画面遷移なしで操作可能にする。

## 10. リアルタイム要件

- オーディオスレッドで動的メモリ確保を行わない。
- オーディオスレッドでロック、ファイルI/O、UI呼び出しを行わない。
- パラメータ変更を適切に平滑化する。
- NaN、Inf、発散した信号を出力しない。
- 44.1、48、88.2、96 kHzおよび可変ブロックサイズに対応する。
- Offline Renderで実時間再生と同一の結果を生成する。

## 11. 初期リリース対象外

- 実機SysExおよび既存ハードウェアプリセット互換
- MPE
- Microtuning
- Step Sequencer
- Arpeggiator
- 汎用Modulation Matrix
- Wavetable
- 部品単位の完全な回路エミュレーション

## 12. 開発フェーズ

1. Core DSP: VCO、ADSR、Mixer、Filter、ヘッドレステスト
2. Poly Engine: 8 voices、MIDI、Sustain、Poly Mod、Unison
3. Product Shell: JUCE Standalone、VST3、AU、State、Automation、GUI
4. Character: Vintage、Saturation、Filter調整、Factory Presets
5. Release: クロスプラットフォーム検証、Installer、Documentation
