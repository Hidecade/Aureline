# Aureline App Store metadata

This file is the source copy for App Store Connect. It is not bundled into the
app and does not affect runtime behavior.

## Version

- Version: `1.0.9`
- Build: `10`
- Primary category: `Music`
- Secondary category: `Entertainment`
- Copyright: `2026 Hideki Konishi / Hidecade Instruments`

## URLs

- Support URL: `https://hidecade.github.io/Aureline/support/`
- Privacy Policy URL: `https://hidecade.github.io/Aureline/privacy/`

## Japanese localization

### Subtitle

8ボイス・アナログシンセ

### Promotional text

アナログモデリング、Wave Memory、POLY MODを一台に。128音色を収録し、スタンドアロンでもAUv3音源としても演奏できます。

### Description

Aurelineは、クラシックなアナログシンセの操作感とWave Memoryの個性を組み合わせた、8ボイス・ポリフォニックシンセサイザーです。

2基のオシレーター、4段OTAローパスフィルター、2基のエンベロープ、LFO、POLY MOD、アルペジエーターを備え、温かいパッドや力強いベースから、金属的なクロスモジュレーション、デジタルなゲームサウンド、効果音まで幅広く作れます。

主な機能

・8ボイス・ポリフォニー
・2オシレーター構成
・アナログモデリング波形と32ステップWave Memory
・波形を指で描いて保存できるWAVE EDIT
・オシレーターBとフィルターエンベロープを使うPOLY MOD
・自己発振対応4段OTAローパスフィルター
・フィルター／アンプ独立エンベロープ
・LFO、アルペジエーター、コード、ホールド
・4バンク、合計128音色
・音色、32音色バンク、Wave Memoryの保存／読み込み
・外部MIDI入力
・AUv3 Instrument対応

スタンドアロンアプリとして画面鍵盤や外部MIDI機器から演奏できるほか、GarageBandなどの対応ホストではAUv3 Instrumentとして使用できます。音源エンジンと音色ファイルはMac／Windows版Aurelineと共通です。

### Keywords

シンセ,音源,楽器,MIDI,AUv3,アナログ,波形,作曲,鍵盤,LFO

### What’s New

・ライブラリ読込時にバンク名を反映
・TR-808に着想を得た32音色ライブラリを追加
・打楽器向けのPoly Mod、ノイズ、金属波形音色を追加

## English localization

### Subtitle

8-Voice Analog Synthesizer

### Promotional text

Analog modeling, Wave Memory, and POLY MOD in one instrument, with 128 sounds. Play standalone or as an AUv3 Instrument.

### Description

Aureline is an eight-voice polyphonic synthesizer combining the hands-on character of a classic analog instrument with distinctive Wave Memory sounds.

Its two oscillators, four-stage OTA low-pass filter, envelopes, LFO, POLY MOD, and arpeggiator cover warm pads, powerful basses, metallic cross-modulation, digital game tones, and sound effects.

Features:

• Eight-voice polyphony
• Two oscillators
• Analog-modeled waveforms and 32-step Wave Memory
• WAVE EDIT with drawable, savable waveforms
• POLY MOD using Oscillator B and the filter envelope
• Self-oscillating four-stage OTA low-pass filter
• Independent filter and amplifier envelopes
• LFO, arpeggiator, chord, and hold controls
• Four banks with 128 sounds
• Voice, 32-sound bank, and Wave Memory import/export
• External MIDI input
• AUv3 Instrument support

Play Aureline from its on-screen keyboard or an external MIDI controller as a standalone app, or load it as an AUv3 Instrument in a compatible host such as GarageBand. The synthesis engine and voice files are shared with Aureline for Mac and Windows.

### Keywords

synth,instrument,MIDI,AUv3,analog,waveform,music,keyboard,LFO,polyphonic

### What’s New

• Added bank names that follow imported voice libraries
• Added a 32-voice library inspired by classic TR-808 percussion
• Added percussion voices using Poly Mod, noise, and metallic waveforms

## App Review information

### Sign-in

No sign-in or demo account is required.

### Review notes

Aureline is a standalone synthesizer and also includes an AUv3 Instrument extension.

Standalone test:
1. Launch Aureline.
2. Tap the on-screen keyboard; audio should play immediately.
3. Change a voice from one of the four banks in the voice selector.
4. Open EDIT WAVE to draw and audition a waveform-memory sound.
5. Tap SAVE or SAVE BANK to open the standard document export interface.

AUv3 test in GarageBand:
1. Create or open a song.
2. Add an Audio Unit Extension instrument track.
3. Select Aureline.
4. Play it from GarageBand’s keyboard.

The app has no account, advertising, analytics, in-app purchases, subscriptions, or network-dependent features. Voice, bank, and Wave Memory files are stored locally and are imported or exported only through Apple’s document interfaces at the user’s request.

The app requests no microphone, camera, photo-library, location, contacts, or
tracking permission. External MIDI input is handled through Core MIDI.

## Export compliance

Aureline does not implement proprietary or non-exempt encryption. It only uses encryption supplied by Apple’s operating system for standard platform services.

## Screenshot plan

Use landscape JPEG or PNG screenshots without transparency. Upload between one
and ten images. For the 6.9-inch iPhone screenshot slot, use one consistent
accepted landscape size:

- `2736 × 1260`
- `2796 × 1290`
- `2868 × 1320`

Recommended sequence:

1. PLAY screen with the keyboard and oscillator waveforms.
2. EDIT screen with synthesis controls.
3. WAVE EDIT modal with a custom waveform.
4. Four-bank voice selection/library screen.
5. POLY MOD and FINAL MIX waveform display.
6. AUv3 running inside a compatible host, if the host UI can be shown clearly.

## App Store Connect field checks

- App name: 2–30 characters.
- Subtitle: maximum 30 characters.
- Promotional text: maximum 170 characters.
- Description: maximum 4,000 characters, plain text only.
- Keywords: maximum 100 bytes, comma-separated.
- Screenshots: 1–10 per device size and localization.
- Privacy Policy URL is required for iOS.
- Select the uploaded build whose `CFBundleShortVersionString` is `1.0.9` and
  whose `CFBundleVersion` is `10`.

Before submission, recheck these values against the processed build shown in
App Store Connect. This document is a submission worksheet; changing it does
not update App Store Connect automatically.
