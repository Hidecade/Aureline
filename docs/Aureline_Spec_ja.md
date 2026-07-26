# Aureline 製品仕様

- 文書バージョン: 0.1
- 製品バージョン: 1.0.1
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

初期リリースではmacOSおよびWindowsのStandalone、VST3、macOS Audio Unitを対象とする。MIDI入力とステレオ出力に対応する。iOSおよびAUv3は追加対応として開発中とし、詳細は`Aureline_iOS_Spec_ja.md`に定める。

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

- Prophet系の構成を意識した、4段OTA積分器とグローバル・レゾナンス帰還による24 dB/octローパスを各ボイスに持つ。
- 各OTA段の差動入力をソフト飽和させ、従来のラダー型モデルとは異なるオーバードライブ特性を持たせる。
- 最大レゾナンスでは内部ノイズフロアをきっかけに自己発振する。高レゾナンス時は入力を緩やかに補償しつつ、アナログフィルターらしい低域の痩せを残す。
- エイリアシングと高域での不安定性を抑えるため、フィルター内部を2倍オーバーサンプリングする。
- Rev 1/2・Rev 3の切替は設けず、両者の実装定数を転用しないAureline独自の単一OTAモデルとする。

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
- Destination: VCO A Frequency、Pulse Width A、Filter Cutoff
- グローバルではなく各ボイス内で処理する。
- VCO B Sourceは`VCO B波形 × Amount × 2.0`とする。
- 2つのSource Amountは0〜1の単方向ノブとし、負Amountは音色仕様として扱わない。
- Filter Envelope Sourceは`Filter Envelope × Amount² × 4.0`とし、下半分では微調整しやすく、上半分で深くなるAureline独自カーブを持つ。
- 2つのSourceを加算した共通Poly Mod信号を各Destinationへ送る。
- VCO A Frequencyは基準周波数へ`2^clamp(Poly Mod, -4, 4)`を乗算し、ピッチ領域で上下対称となる指数変調を行う。
- Pulse Width Aは`-0.25 × Poly Mod`を加算して0.02〜0.98へ制限する。
- Filter Cutoffは通常のEnvelope、LFO、Key Trackingへ`Poly Mod × 1.5 octaves`を加え、Poly Mod成分を±4 octavesへ制限する。
- VCO BのMixer LevelはPoly Mod量へ影響せず、Mixer Levelが0でも変調源として使用できる。
- Filter Envelope Sourceは通常のFilter Envelope Amountから独立し、通常Amountが0でも各Poly Mod Destinationへ作用する。
- VCO BのSaw、Triangle、Pulseおよび複数選択結果を変調波形として使用し、LFおよびKeyboard Tracking設定を反映する。
- WAVE MEMORY選択時にその波形をVCO B Sourceとして使用できることはAureline独自拡張とする。
- Oscillator SyncではVCO BをMaster、VCO AをSlaveとし、Filter EnvelopeからFREQ Aへの経路でSync Sweepを作成できる。
- 係数および伝達カーブはAureline独自仕様とし、他製品の実装式や固有定数を移植しない。

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

単一音色の拡張子は`.aurelinevoice`、全音色ライブラリは`.aurelinelibrary.xml`とする。全音源パラメータ、Voice Mode、Vintage、Effects、Name、Author、Category、Format Versionを保存する。

内蔵音色バンクは`Analog 1 → Analog 2 → Retro → 8-Bit`の順に表示する。各バンクは32音色で、末尾4スロットをそのバンクの音響方式を活かした効果音とする。旧構成から更新する場合は、ユーザーが編集したRetroと8-Bitの各スロットを保持したまま後方のバンクへ移行する。

Analog 1はBrass 8、Strings/Pad 8、Piano/Keys 6、Bass 6、SE 4で構成する。Analog 2はLead 8、Poly Mod/Sync 6、Percussion 6、Rhythm 8、SE 4で構成し、Analog 1の単なる派生音色を置かない。64音色版への初回更新時のみAnalog 1/2を新しい内蔵内容へ置き換え、Retroと8-Bitは保持する。

Mac／Windows版はヘッダーにWAV録音ボタンを持つ。`WAV`で録音を開始して表示を`STOP`へ切り替え、`STOP`で録音を終了して保存先を選択する。最終ステレオ出力を現在のサンプルレート、24-bit WAVで保存する。音声スレッドは事前確保したリングバッファへの書き込みだけを行い、データ集約とファイル書き込みは音声スレッド外で処理する。

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
