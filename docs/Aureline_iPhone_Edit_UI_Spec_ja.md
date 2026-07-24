# Aureline iPhone版 EDIT画面仕様

- 文書バージョン: 0.2
- 対象: Aureline iPhoneスタンドアロン版
- 対応OS: iOS 16.0以降
- 対応端末: iPhoneのみ（iPad対象外）
- 画面方向: Landscape Left / Landscape Right
- 参照: Aureline Mac版、OpalineFM iPhone版 EDIT画面
- ステータス: 実装前仕様

## 1. 目的

iPhone横画面の限られた高さと横幅で、Aurelineの全音色パラメータを無理なく編集できるEDIT画面を提供する。

現行の横スクロール式パラメータ列は廃止し、機能ごとの複数セクションへ分割する。ページを切り替えながらも、現在の音色、演奏状態、ノブの値を保持し、音を鳴らしたまま調整できることを優先する。

Mac版Aurelineに存在する音色編集、演奏設定、Arpeggiator、Chord、VOICE管理機能は、プラットフォーム固有機能を除き原則すべてiPhone版へ組み込む。画面が狭いことを理由に機能を削除せず、セクション分割とポップオーバーで操作面積を確保する。

## 2. 基本方針

- EDIT画面を8セクションに分割する。
- セクション切替は画面左側の縦タブで行う。
- パラメータ領域の横スクロールは使用しない。
- 1画面内のノブは原則8個以下とする。
- PLAY / EDIT切替、音色名、試奏用鍵盤はセクション切替の影響を受けない固定領域とする。
- ノブ、ロッカースイッチ、パネル、配色はMac版Aurelineの外観を基準とする。
- ページ構成と操作密度はOpalineFM iPhone版EDIT画面を基準とする。
- パラメータ変更は即時に音源へ反映し、Applyボタンは設けない。

## 3. 画面構成

```text
┌────────────────────────────────────────────────────────────┐
│ PLAY | EDIT   VOICE NAME                    MINI WAVEFORM   │
├──────────┬─────────────────────────────────────────────────┤
│ OSC A    │                                                 │
│ OSC B    │          選択中セクションの編集パネル           │
│ FILTER   │          ノブ／スイッチ／波形選択               │
│ F ENV    │                                                 │
│ A ENV    │                                                 │
│ MOD      │                                                 │
│ PERF     │                                                 │
│ ARP      │                                                 │
├──────────┴─────────────────────────────────────────────────┤
│                   試奏用ミニ鍵盤                            │
└────────────────────────────────────────────────────────────┘
```

### 3.1 上部固定領域

- 左端にPLAY / EDIT切替ボタンを配置する。
- PLAY / EDITの位置、サイズ、デザインはPLAY画面と揃える。
- EDITをアンバー色の選択状態で表示する。
- PLAYを押すと演奏状態を維持したままPLAY画面へ戻る。
- 中央に現在の音色名を1行で表示する。
- 右側に小型波形モニターを配置する。表示専用とし、タッチ操作は持たせない。
- 上部固定領域の高さは56ptを目安とする。

### 3.2 セクションタブ

- 編集パネル左側へ縦1列で配置する。
- 表示名は `OSC A`、`OSC B`、`FILTER`、`F ENV`、`A ENV`、`MOD`、`PERF`、`ARP` とする。
- 選択中はアンバー背景＋黒文字、非選択時は暗色背景＋明色文字とする。
- 画面切替時の初期セクションは、前回選択していたセクションとする。
- アプリ初回起動時は `OSC A` を選択する。
- 各タブの最小高さは34pt、タブ間隔は4ptを目安とする。

### 3.3 編集パネル

- 選択中セクションだけを表示し、ページ間の横スクロールは行わない。
- ノブは上段・下段の最大2行に配置する。
- ノブのラベル、目盛り、現在値が同じ画面内で判読できること。
- スイッチは赤LED付きロッカースイッチを使用し、2行単位で整理する。
- 空き領域を埋めるためだけにノブを拡大しない。PLAY画面と同程度のサイズを基準にする。
- 小型iPhoneでも要素が欠けず、必要な場合は間隔を縮める。編集パネル自体はスクロールさせない。

### 3.4 試奏用鍵盤

- EDIT画面下部へ常時表示する。
- OpalineFM iPhone版EDIT画面と同等の小型試奏鍵盤を採用する。
- オクターブ `+` / `-` を鍵盤左側へ配置する。
- セクション切替中も発音を継続する。
- 外部MIDIおよび画面鍵盤の発音中ノートを表示へ反映する。
- 画面を離れる場合は画面タッチ由来のノートを停止する。

## 4. セクション構成

### 4.1 OSC A

Oscillator Aの基本音色を編集する。

| 種別 | 項目 | パラメータID |
|---|---|---|
| 波形選択 | SAW / PULSE / TRIANGLE等 | 既存Oscillator A waveform ID |
| ノブ | VCO A LEVEL | `oscALevel` |
| ノブ | PULSE WIDTH A | `pulseWidthA` |
| 離散選択 | RANGE | 既存Oscillator A range ID |

- 複数波形を同時選択できる場合はMac版と同じトグル動作にする。
- 波形ボタンは図形表示を優先し、文字だけのコンボボックスにはしない。
- RANGEはMac版と同じ `32'`、`16'`、`8'`、`4'`、`2'` の5段階とする。

### 4.2 OSC B / MIX

Oscillator Bとノイズミックスを編集する。

| 種別 | 項目 | パラメータID |
|---|---|---|
| 波形選択 | SAW / PULSE / TRIANGLE等 | 既存Oscillator B waveform ID |
| ノブ | VCO B LEVEL | `oscBLevel` |
| ノブ | B FINE | `oscBFine` |
| ノブ | PULSE WIDTH B | `pulseWidthB` |
| ノブ | NOISE | `noiseLevel` |
| 離散選択 | RANGE | 既存Oscillator B range ID |
| スイッチ | SYNC | `oscSync` |
| スイッチ | OSC B LF | `oscBLowFrequency` |
| スイッチ | OSC B KB | `oscBKeyTrack` |

- スイッチは最大2行に配置する。
- B FINEのみ表示値をセント実値とし、その他の正規化ノブは0〜10表示を基本とする。

### 4.3 FILTER

フィルター本体と演奏追従を編集する。

| 種別 | 項目 | パラメータID |
|---|---|---|
| ノブ | CUTOFF | `cutoff` |
| ノブ | RESONANCE | `resonance` |
| ノブ | FILTER ENV | `filterEnvAmount` |
| ノブ | KEY TRACK | `filterKeyTrack` |
| ノブ | VELOCITY | `filterVelocity` |

- CUTOFFは対数カーブを使用する。
- FILTER ENVは負値と正値を区別できる中央基準目盛りにする。
- PLAY画面のCUTOFF／RESONANCEと同じ値を共有し、双方の表示を同期する。

### 4.4 FILTER ENV

Filter Envelopeを編集する。

| 種別 | 項目 | パラメータID |
|---|---|---|
| ノブ | ATTACK | `filterAttack` |
| ノブ | DECAY | `filterDecay` |
| ノブ | SUSTAIN | `filterSustain` |
| ノブ | RELEASE | `filterRelease` |

- ADSRは左からA、D、S、Rの順に横一列で配置する。
- 右側の空き領域に簡易エンベロープ形状を表示してよい。
- 簡易表示は操作値に追従するが、直接編集機能は持たせない。

### 4.5 AMP ENV

Amplifier Envelopeを編集する。

| 種別 | 項目 | パラメータID |
|---|---|---|
| ノブ | ATTACK | `ampAttack` |
| ノブ | DECAY | `ampDecay` |
| ノブ | SUSTAIN | `ampSustain` |
| ノブ | RELEASE | `ampRelease` |

- FILTER ENVと同一レイアウトを使用し、ページを切り替えてもノブ位置が変わらないようにする。
- パネル見出しでFILTER ENVとの違いを明確にする。

### 4.6 MOD

LFOとPoly Modを1ページ内の上下2ブロックに分ける。

#### LFOブロック

| 種別 | 項目 | パラメータID |
|---|---|---|
| 波形選択 | LFO WAVE | 既存LFO waveform ID |
| ノブ | LFO RATE | `lfoRate` |
| ノブ | INITIAL AMT | `lfoAmount` |
| ノブ | LFO DELAY | `lfoDelay` |
| ノブ | LFO FADE | `lfoFade` |
| スイッチ | A FREQ | `lfoDestA` |
| スイッチ | B FREQ | `lfoDestB` |
| スイッチ | FILTER | `lfoDestFilter` |
| スイッチ | PW A | 既存LFO Pulse Width A destination ID |
| スイッチ | PW B | 既存LFO Pulse Width B destination ID |
| スイッチ | RETRIG | `lfoRetrigger` |

#### POLY MODブロック

| 種別 | 項目 | パラメータID |
|---|---|---|
| ノブ | POLY F ENV | `polyModFilterEnv` |
| ノブ | POLY OSC B | `polyModOscB` |
| スイッチ | PITCH | `polyDestPitch` |
| スイッチ | PW A | `polyDestPWA` |
| スイッチ | FILTER | `polyDestFilter` |

- LFOとPOLY MODの境界を細いアンバー枠または見出しで明示する。
- ノブが過密になる場合はLFOノブを上段、Poly Modノブと全スイッチを下段に配置する。

### 4.7 PERFORMANCE

Mac版のPERFORMANCE領域およびPLAY画面にある演奏・音響設定を編集する。

| 種別 | 項目 | パラメータID |
|---|---|---|
| ノブ | VOLUME | `masterGain` |
| ノブ | TRANSPOSE | `transpose` |
| ノブ | PITCH RANGE | `pitchBendRange` |
| ノブ | MASTER TUNE | `masterTune` |
| ノブ | GLIDE | `glide` |
| ノブ | UNI DETUNE | `unisonDetune` |
| ノブ | SPREAD | `spread` |
| ノブ | VINTAGE | `vintage` |
| スイッチ | LEGATO | `glideLegatoOnly` |
| 選択 | VOICE MODE | `voiceMode` |

- VOICE MODEはPOLY、MONO、UNISONの排他的選択とする。
- VOLUME、TRANSPOSE、GLIDE、CUTOFF、RESONANCE、TEMPOなどPLAY画面にも存在する値は双方向に同期する。
- PITCH RANGEは0〜24 semitones、TRANSPOSEは-24〜+24 semitones、MASTER TUNEは-100〜+100 centsとする。
- UNI DETUNEはUNISON時以外も編集可能とし、無効化して値を隠さない。
- 8ノブを上段4個・下段4個に配置し、VOICE MODEとLEGATOは右端または下端のスイッチ領域に配置する。

### 4.8 ARP / CHORD

Mac版のArpeggiator、Chord、Holdおよびテンポ／スケール設定を編集する。

| 種別 | 項目 | パラメータID |
|---|---|---|
| ノブ | TEMPO | `tempoBpm` |
| 離散選択 | SCALE ROOT | `scaleRoot` |
| 離散選択 | ARP RATE | `arpRate` |
| 離散選択 | DIRECTION | `arpDirection` |
| ノブ | GATE | `arpGate` |
| スイッチ | ARP | `arpEnabled` |
| スイッチ | CHORD | `chordEnabled` |
| スイッチ | HOLD | `arpHold` |

- TEMPOは40〜240 BPMの実値を表示する。
- SCALE ROOTはC、C#、D、D#、E、F、F#、G、G#、A、A#、Bの12段階とする。
- ARP RATEは `1/8`、`1/16`、`1/32` の3段階とする。
- DIRECTIONは `UP`、`DOWN`、`U/D`、`RND` の4段階とする。
- GATEはMac版と同じ0.1〜0.95の範囲を使用し、画面表示は0〜10とする。
- ARP、CHORD、HOLDはPLAY画面のスイッチと双方向に同期する。
- CHORDは`scaleRoot`を基準にMac版と同じダイアトニック3和音を生成する。
- ARPがOFFでもCHORDは機能し、HOLDはMac版と同じノート保持規則に従う。

## 5. Mac版機能の包含範囲

### 5.1 iPhone版へ組み込む機能

- Oscillator A/Bの全波形、レンジ、レベル、Fine、Pulse Width
- Oscillator Sync、Oscillator B LF、Keyboard Tracking、Noise
- Filter本体、Filter Envelope、Amplifier Envelope、Velocity
- LFOの全波形、Rate、Initial Amount、Delay、Fade、Retrigger
- LFOのA FREQ、B FREQ、PW A、PW B、FILTER送信
- Poly Modの2 Source量とFREQ A、PW A、FILTER送信
- Voice Mode、Glide、Legato、Master Tune、Unison Detune、Spread、Vintage
- Volume、Transpose、Pitch Bend Range、Pitch Wheel、Mod Wheel
- Tempo、Scale Root、Arpeggiator、Chord、Hold
- LCD、波形モニター、画面鍵盤、オクターブ移動
- Factory/User VOICE選択
- LOAD、SAVE、COPY、PASTE、INIT、STORE
- Presetへの全パラメータ保存と復元

### 5.2 プラットフォームに合わせて置き換える機能

- Macのファイル選択画面はiOS Document Picker / Share Sheetへ置き換える。
- MacのPCキーボード演奏はiPhone画面鍵盤とCore MIDI入力へ置き換える。
- Macのウィンドウサイズ変更は対象外とし、iPhone横画面へ最適化する。
- MacのAudio/MIDIデバイス選択はiOSのAVAudioSession/Core MIDI仕様に従う。
- マウスホバー、右クリック、キーボードショートカットはiPhoneでは必須としない。

### 5.3 機能差の管理

- Mac版に音色パラメータまたは演奏機能を追加した場合、iPhone版仕様とパラメータ表も同時に更新する。
- 意図的にiPhone版へ搭載しない機能は、理由と代替操作を本節へ明記する。
- 単に画面が狭いことは機能除外の理由にしない。

## 6. VOICE操作

`LOAD`、`SAVE`、`COPY`、`PASTE`、`INIT`、`STORE`はEDIT画面から利用できる状態を維持する。

- 常時表示すると編集面積を圧迫するため、音色名を押したときに開くVOICE操作パネルへまとめる。
- 操作順と意味はOpalineFMと同じにする。
- `PASTE`はコピー内容がない場合に無効表示する。
- `STORE`は他ボタンより強いアンバー／オレンジで表示する。
- ファイル選択、保存、エラー表示の既存動作は変更しない。
- VOICE操作パネルを閉じても選択中のEDITセクションを維持する。

## 7. 操作仕様

### 7.1 ノブ

- 上下ドラッグで値を変更する。上方向で増加、下方向で減少する。
- 1回の全高ドラッグで概ね全レンジを移動できる感度とする。
- ダブルタップでパラメータ既定値へ戻す。
- 操作中も数値表示を更新する。
- 音源側、Preset読込、PLAY画面から値が変わった場合も表示を同期する。
- VoiceOverではパラメータ名、現在値、増減操作を提供する。

### 7.2 スイッチ

- タップごとにON / OFFを切り替える。
- ONは明るい赤LED、OFFは暗い赤LEDで表示する。
- ラベルを含むスイッチ全体をタッチ領域とする。
- 見た目より広い44×44pt相当の操作領域を可能な範囲で確保する。

### 7.3 ページ切替

- タブ操作によるセクション切替には短いフェードを使用してよい。
- 横スワイプによるページ切替はノブ操作と競合するため採用しない。
- セクション切替で音色値、鍵盤、MIDI状態、Undo対象を初期化しない。

## 8. レイアウト基準

- 基準確認端末はiPhone 17 Proシミュレータの横画面とする。
- Safe Areaを除く領域内に全操作部を収める。
- 編集パネル内で縦方向・横方向ともスクロールを必要としないことを合格条件とする。
- ラベルは原則9〜10pt、現在値は12〜13pt、タブは10〜11ptを基準とする。
- 長いラベルは1行表示し、必要に応じて最小85%まで縮小する。
- 隣接するノブやスイッチのタッチ領域を重ねない。
- 背景、枠、ノブ、ロッカー、文字色はPLAY画面と統一する。

## 9. 状態管理

- 選択セクションはUI状態として保持し、Presetには保存しない。
- 全パラメータ値は`MobileSynthModel`を唯一の表示状態とする。
- EDIT画面固有のローカルコピーを作らず、ノブ操作を即時にモデルへ書き込む。
- Preset読込、INIT、PASTE後は表示中セクションを維持したまま全コントロールを更新する。
- PLAY / EDIT切替時も発音中ノートと演奏パラメータを維持する。

## 10. 実装単位

想定コンポーネントを次のように分離する。

```text
EditView
├── AurelineEditHeader
├── AurelineEditTabBar
├── OscillatorAEditSection
├── OscillatorBEditSection
├── FilterEditSection
├── EnvelopeEditSection (Filter / Amp共用)
├── ModulationEditSection
├── PerformanceEditSection
├── ArpChordEditSection
├── AurelineEditKnob
├── AurelineEditRocker
├── AurelineWaveSelector
├── AurelineVoiceActionSheet
└── AurelineAuditionKeyboard
```

- セクション定義とパラメータ配列をView本体から分離する。
- Filter/Amp ADSRは共通コンポーネントを使用する。
- Mac版とiPhone版で音源パラメータID、範囲、既定値を共通化する。
- 見た目の定数はEDIT専用Theme/Layoutへまとめ、個々のViewへ数値を散在させない。

## 11. 受け入れ条件

1. iPhone横画面でEDITの8セクションを切り替えられる。
2. 編集パネルに横スクロールが存在しない。
3. 各セクションの全コントロールが欠けずに表示される。
4. 全ノブとスイッチが対応する音源パラメータへ即時反映される。
5. FILTER ENVとAMP ENVが同じ位置・操作感で編集できる。
6. EDIT中も画面鍵盤および外部MIDIで試奏できる。
7. PLAY / EDIT切替後も音色値と選択中セクションが維持される。
8. LOAD / SAVE / COPY / PASTE / INIT / STOREが従来どおり動作する。
9. iPhone 17 Proシミュレータでレイアウト崩れ、意図しないスクロール、タッチ競合がない。
10. Mac版とiPhone版で同じPresetを読み込んだ際、対応パラメータ値が一致する。
11. Mac版の全音色パラメータ、PERFORMANCE、ARP、CHORD、HOLDをiPhone版から操作できる。
12. PLAY画面とEDIT画面に重複するパラメータが常に双方向同期する。

## 12. 実装順序

1. `EditPage`を8セクションへ変更し、固定ヘッダーと縦タブを実装する。
2. OSC A、OSC B / MIXを実装する。
3. FILTER、FILTER ENV、AMP ENVを実装する。
4. MODをLFO／POLY MODの2ブロックで実装する。
5. PERFORMANCEを実装し、PLAY画面との双方向同期を検証する。
6. ARP / CHORDを実装し、Mac版と同じ演奏結果になることを検証する。
7. VOICE操作を音色名から開くパネルへ整理する。
8. 試奏用ミニ鍵盤を追加する。
9. Mac版の全機能・全保存パラメータとの対応表をコード上でも一元化する。
10. 全パラメータ同期、Preset操作、画面切替を検証する。
11. iPhone 17 Proシミュレータで視認性とタッチ操作を調整する。
