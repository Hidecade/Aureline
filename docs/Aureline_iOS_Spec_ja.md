# Aureline iOS対応仕様

- 文書バージョン: 0.1
- 対象製品バージョン: 1.0.1以降
- 対象プラットフォーム: iOS 16.0以降
- 参照実装: `OpalineFM/iOS/OpalineFMMobile`
- ステータス: 実装前仕様

## 1. 目的

AurelineのC++音源エンジンを共用し、iPhone向けスタンドアロンアプリとAUv3 Instrument Extensionを提供する。

Opaline FM Mobileと同じく、デスクトップ版のJUCE UIをiOSへ移植するのではなく、SwiftUIによるモバイル専用UI、AVAudioEngineによるスタンドアロン音声出力、Objective-C++ブリッジ、ネイティブAUv3を組み合わせる。

## 2. 製品境界

- 製品名は「Aureline」とし、デスクトップ版と同一製品ファミリーとして扱う。
- Opaline FM MobileとはBundle ID、Audio Unit識別子、保存形式、アセットを共有しない。
- `Source/Engine`および`Source/DSP`の音源実装を共用し、JUCEベースの`Source/App`と`Source/Plugin`には依存しない。
- 音色パラメータの意味と初期値はデスクトップ版と一致させる。
- iOS固有の画面状態、オーディオ設定、MIDI入力選択は音色データに含めない。

## 3. 初回リリース範囲

### 3.1 必須

- iPhone用スタンドアロンアプリ
- AUv3 Instrument Extension（`aumu`）
- 8音ポリフォニックのステレオ出力
- 画面鍵盤、Pitch Bend、Mod Wheel、Sustain
- Core MIDI入力（ノート、ベロシティ、Pitch Bend、CC 1、CC 64）
- Poly、Mono、Unison
- 全音源パラメータの編集
- Factory Presetの選択
- ユーザープリセットの保存、読込、共有
- AUv3ホストからのMIDI入力、パラメータ操作、状態保存／復元、オフラインレンダー
- オーディオセッションの中断、ルート変更、Media Services Resetからの復帰

### 3.2 初回リリース対象外

- iPad対応。初回リリースはiPhone専用とする。
- デスクトップ版プリセットファイルとの完全な相互運用。共通スキーマを確定後に追加する。
- Ableton Link、Bluetooth MIDI機器の独自設定画面、MPE、Microtuning
- Inter-App Audio（AUv3を使用する）
- iCloud同期、アカウント、分析SDK、広告、アプリ内課金
- デスクトップUIに実装されているArpeggiator、Chord、Hold。これらは現在`AnalogEngine`の外側にあるため、第2段階で共通のリアルタイム処理層へ分離してから対応する。

## 4. 対応端末と画面

- Deployment Target: iOS 16.0
- Device Family: iPhone（`TARGETED_DEVICE_FAMILY = 1`）
- Orientation: Landscape Left / Landscape Right
- 全画面表示を基本とする。
- Safe Area、Dynamic Type、VoiceOverラベル、十分なタッチ領域を考慮する。
- 小さい画面でも鍵盤を常時演奏できるよう、主要ナビゲーションと鍵盤を固定し、編集領域を切り替える。

## 5. アプリ構成

配置先は`Aureline/iOS/AurelineMobile`とする。

```text
iOS/AurelineMobile/
├── project.yml
├── AurelineMobile.xcodeproj/
├── Sources/
│   ├── App/
│   │   ├── AurelineMobileApp.swift
│   │   ├── MobileAudioEngine.swift
│   │   ├── MobileMIDIInput.swift
│   │   └── MobileSynthModel.swift
│   ├── Native/
│   │   ├── AurelineMobileEngineBridge.h
│   │   ├── AurelineMobileEngineBridge.mm
│   │   └── AurelineMobile-Bridging-Header.h
│   ├── UI/
│   │   ├── RootView.swift
│   │   ├── PlayView.swift
│   │   ├── EditView.swift
│   │   ├── PresetView.swift
│   │   └── Components/
│   └── Resources/
│       ├── Assets.xcassets/
│       └── Info.plist
└── AUv3Extension/
    ├── AurelineAUAudioUnit.h
    ├── AurelineAUAudioUnit.mm
    ├── AudioUnitViewController.swift
    ├── AurelineAUv3Extension-Bridging-Header.h
    └── Info.plist
```

XcodeプロジェクトはOpaline FM Mobileと同様にXcodeGenの`project.yml`を正とし、生成済み`.xcodeproj`もリポジトリへ含める。

## 6. ターゲット識別子

| 項目 | 値 |
|---|---|
| App target | `AurelineMobile` |
| App Bundle ID | `com.hidekikonishi.aureline.mobile` |
| AUv3 target | `AurelineAUv3Extension` |
| AUv3 Bundle ID | `com.hidekikonishi.aureline.mobile.auv3` |
| Audio Unit type | `aumu` |
| Audio Unit subtype | `Aurl` |
| Audio Unit manufacturer | `Hcde` |
| Audio Unit表示名 | `Hidecade: Aureline` |

Bundle IDと署名チームはApp Store Connect登録前に最終確認する。Audio Unitのtype、subtype、manufacturerの組み合わせは公開後に変更しない。

## 7. 共有コードとブリッジ

### 7.1 共有対象

- `Source/Engine/AnalogEngine.*`
- `Source/Engine/AnalogPatch.*`
- `Source/Engine/AnalogVoice.*`
- `Source/DSP/*`
- Factory Preset定義。現在`MainComponent.cpp`にある定義は、JUCE非依存の共有ファイルへ移動する。

### 7.2 Objective-C++ブリッジ

`AurelineMobileEngineBridge`はSwiftからC++型を直接参照させず、次の機能を提供する。

- `prepare(sampleRate:)`
- `render(left:right:frames:)`および`render(to:frames:)`
- `noteOn(_:velocity:)`、`noteOff(_:)`、`panic()`
- `setPitchBend(_:)`、`setPitchBendRange(_:)`
- `setModWheel(_:)`、`setSustainPedal(_:)`
- 音源パラメータの取得、変更、初期化
- Factory Preset一覧と選択
- プリセットのエンコード／デコード

UIスレッドから`AnalogEngine`を直接変更しない。ノートイベントと演奏操作は固定長のロックフリーコマンドキューへ投入し、音色の一括変更は世代番号付きのダブルバッファまたは同等のメールボックスでオーディオスレッドへ渡す。

## 8. スタンドアロン音声処理

- `AVAudioSession` category: `.playback`
- option: `.mixWithOthers`
- 希望サンプルレート: 44.1 kHz
- 希望I/Oバッファ時間: 5 ms
- 実際のサンプルレートは`AVAudioSession.sampleRate`を使用する。
- `AVAudioEngine`へステレオの`AVAudioSourceNode`を接続し、コールバックからブリッジをレンダーする。
- ハードウェア出力音量とは別にアプリ内Master Volumeを持つ。
- バックグラウンドでの継続再生は初回リリースでは要求しない。非アクティブ化時はPanicを実行し、安全に停止する。

通知監視対象はInterruption、Route Change、Media Services Were Resetとする。復帰時はエンジンを現在のサンプルレートで再prepareし、現在の音色を再適用する。

## 9. MIDI仕様

スタンドアロンはCore MIDIの全入力ソースを既定で受信する。最低限、次を処理する。

| MIDI | 動作 |
|---|---|
| Note On / Off | 発音／消音、Velocity対応 |
| Pitch Bend | `-1.0...1.0`へ正規化 |
| CC 1 | Mod Wheel |
| CC 64 | Sustain Pedal（64以上でOn） |
| CC 120 / 123 | Panic / All Notes Off |

AUv3は`AURenderEvent`を処理し、イベント時刻に合わせてレンダーブロックを分割する。MIDI 2.0とMPEは初回対象外とする。

## 10. 画面仕様

### 10.1 共通

- 上部: 製品名、音色名、INIT、Play/Edit切替、MIDI／Audio状態
- 下部: Pitch Wheel、Mod Wheel、オクターブ切替付き鍵盤
- 配色はデスクトップ版の暗色パネルとアンバー／ゴールド系アクセントを継承する。
- 木目素材を使用する場合は既存のAurelineアセットを使用し、Opaline FMのアセットを流用しない。

### 10.2 Play

- Preset前後選択
- Voice Mode: Poly / Mono / Unison
- Transpose、Pitch Bend Range、Master Tune
- Glide、Legato Only、Unison Detune、Stereo Spread、Vintage、Master Volume
- 画面鍵盤とホールド中ノート表示

### 10.3 Edit

編集領域を次のページへ分割する。

1. OSC/MIX: VCO A/B Range、波形、Level、Fine、Pulse Width、Sync、LF、Keyboard Tracking、Noise
2. FILTER/ENV: Cutoff、Resonance、Envelope Amount、Keyboard Tracking、Velocity、Filter ADSR、Amp ADSR
3. MOD: LFO Waveform、Rate、Initial Amount、Delay、Fade、Retrigger、各Destination、Poly Mod Source/各Destination

連続値はノブ、離散値はセグメントまたはボタンで操作する。操作中の値を数値表示し、ダブルタップで初期値へ戻せるようにする。

### 10.4 音色管理

- Factory Preset一覧（読込専用）
- INIT ANALOG
- STOREは選択中の音色へ編集内容を上書きする。
- SAVEは現在の音色を`.aurelinevoice`ファイルへ書き出す。
- LOADはMac／iPhone共通の`.aurelinevoice`を読み込み、選択中の音色を置き換える。
- 独立したPreset画面、User Preset一覧、Rename、Deleteは設けない。

## 11. 音色パラメータ

モバイルの音色モデルは`aureline::AnalogPatch`の全フィールドを保持する。加えてデスクトップ版と一致させるため、Transpose、Pitch Bend Rangeなどの演奏設定をPreset Stateとして保持する。

### 11.1 音色コア

- Voice Mode
- VCO A/Bの波形マスク、Range、Fine、Pulse Width、Level、LF、Keyboard Tracking
- Oscillator Sync、Noise Level
- Filter Cutoff、Resonance、Envelope Amount、Keyboard Tracking、Velocity
- Filter ADSR、Amplifier ADSR
- LFO Waveform Mask、Rate、Initial Amount、Delay、Fade、Retrigger、5系統のDestination
- Poly Modの2 Source量と3 Destination
- Glide、Legato Only、Master Tune、Unison Detune、Stereo Spread、Vintage、Master Gain

### 11.2 演奏設定

- Transpose: -24...+24 semitones
- Pitch Bend Range: 0...24 semitones
- Pitch BendとMod Wheelの現在位置は一時的な演奏状態とし、保存しない。

値域は`normalizePatch()`およびデスクトップUIの定義を基準とし、Swift、ブリッジ、AUParameterの三層で同じ範囲を使う。

## 12. Preset保存形式

単一音色の拡張子は`.aurelinevoice`、ライブラリ書き出しは`.aurelinelibrary.xml`とする。単一音色の初回実装はUTF-8 JSONを採用する。

```json
{
  "format": "com.hidecade.aureline.voice",
  "version": 1,
  "name": "INIT ANALOG",
  "author": "Hidecade",
  "category": "Init",
  "patch": {},
  "performance": {
    "transpose": 0,
    "pitchBendRange": 2
  }
}
```

- 未知フィールドは無視し、欠落フィールドは現行の初期値で補う。
- 読込後は必ず正規化し、NaN、Inf、範囲外の値を拒否または補正する。
- STOREによる上書きデータはDocuments/Aureline配下へ保存する。
- Import/ExportはSwiftUIのdocument importer/exporterを使い、Open In Placeに対応する。
- Factory PresetはアプリBundle内の読込専用リソースとする。
- Mac／iPhoneは同じJSONスキーマと共通パラメータ名を使用する。

## 13. AUv3仕様

### 13.1 バスとレンダー

- Music Device / GeneratorではなくInstrument（`aumu`）として登録する。
- 入力バスなし、ステレオ出力バス1系統。
- 44.1、48、88.2、96 kHzと可変ブロックサイズに対応する。
- `maximumFramesToRender`に合わせてprepare時に作業領域を確保し、レンダー中に再確保しない。
- Non-interleaved／interleavedのホスト出力を安全に処理する。
- オフラインレンダーでもリアルタイムと同じ音色とイベントタイミングを得る。

### 13.2 AUParameterTree

AUv3ではPreset選択だけでなく、11章の保存対象パラメータをすべて公開する。識別子はPreset JSONおよびデスクトップStateの名称に合わせ、公開後は変更しない。

ホストAutomationからの変更はatomicな保留値へ書き込み、レンダー開始時にまとめて`AnalogPatch`へ反映する。UIからの操作も必ず`AUParameter`を経由させる。

### 13.3 State

- `fullState`および`fullStateForDocument`へ`version = 1`と全パラメータを保存する。
- 復元時は未知キーを無視し、範囲を正規化する。
- ホストプロジェクト再読込後、音色、Preset名、演奏設定が一致すること。
- MIDIノート、Sustain、Pitch Bendなど瞬間的な演奏状態は復元しない。

### 13.4 Extension UI

初回版はPreset選択、Voice Mode、Glide、Master、主要フィルター操作を表示するコンパクトUIとする。全パラメータはホストAutomationから利用可能にし、将来フル編集UIへ拡張できる構造にする。

## 14. リアルタイム安全性

レンダーコールバックでは次を禁止する。

- 動的メモリ確保とコンテナの拡張
- mutexその他の待機ロック
- Objective-Cオブジェクト生成、ファイルI/O、ログ出力
- SwiftUI／UIKitへのアクセス
- Presetの解析、文字列処理

コマンドキューが満杯の場合はフラグを立て、次回安全なタイミングでPanicと最新状態の再同期を行う。出力は常にfiniteであり、異常値は0へ置換する。

## 15. ビルドと署名

必要環境はFull XcodeとXcodeGenとする。

```bash
cd Aureline/iOS/AurelineMobile
xcodegen generate
xcodebuild \
  -project AurelineMobile.xcodeproj \
  -scheme AurelineMobile \
  -destination 'platform=iOS Simulator,name=iPhone 16' \
  -derivedDataPath ../../build/ios-mobile \
  build
```

- Swift 5、C++17、libc++を使用する。
- App targetがAUv3 Extensionをembedする。
- AppとExtensionのMarketing Version、Build Numberを一致させる。
- Automatic Signingを使用し、Development Teamはリリース設定時に確定する。
- `ITSAppUsesNonExemptEncryption`は`false`とする。

## 16. テストと受け入れ条件

### 16.1 自動テスト

- 既存のC++ engine testsがmacOS上で継続して成功する。
- iOS Simulator向けAppとAUv3 Extensionが警告なしでビルドできる。
- Preset JSONのround-trip、欠落キー、未知キー、不正値をテストする。
- AUv3 stateのround-tripと全AUParameterの範囲をテストする。
- 44.1、48、88.2、96 kHz、異なるブロックサイズで出力がfiniteであることをテストする。

### 16.2 実機テスト

- iPhone内蔵スピーカー、ヘッドホン、USBオーディオで発音する。
- 画面鍵盤と外部MIDI鍵盤でNote On/Off、Velocity、Pitch Bend、Mod Wheel、Sustainが動作する。
- 8音を超える入力、連打、Preset切替、Panicでハングや発音残りがない。
- Bluetooth／USB接続変更、電話等の割込み、画面ロック、Media Services Reset後に安全に復帰する。
- GarageBand等のAUv3ホストで認識、発音、Automation、状態復元、Freeze/Bounceが動作する。
- CPUピークやドロップアウトがなく、InstrumentsのAudio Thread Checkerで重大な違反がない。
- VoiceOverで主要操作の名称と値を読み上げられる。

## 17. 実装フェーズ

1. 共有層整理: Factory Presetと保存モデルをJUCE非依存化し、iOS向けC++ビルドを確認
2. Scaffold: XcodeGen、App、SwiftUIナビゲーション、Objective-C++ブリッジ
3. Playable App: AVAudioEngine、画面鍵盤、Core MIDI、セッション復帰
4. Editing/Voice: 全音色編集、Factory音色、STORE、LOAD/SAVE
5. AUv3: Audio Unit、MIDIイベント、全AUParameter、State、コンパクトUI
6. Hardening: ロックフリー化、実機性能、割込み、Accessibility、TestFlight
7. Future: iPadレイアウト、追加ホスト互換性検証

## 18. App Store方針

- 初回価格: 無料
- 広告なし、アプリ内課金なし、アカウントなし
- 個人情報を意図的に収集しない。
- Privacy Policy、Support URL、iPhone横画面スクリーンショットを用意する。
- App PrivacyとExport Complianceは使用SDK追加時を含め、提出ごとに再確認する。

## 19. 既存文書への反映

実装開始時に`docs/Aureline_Spec_ja.md`の「iOSおよびAUv3は将来対応」を本仕様への参照へ変更する。READMEの対応形式は、実機およびAUv3ホストで受け入れ条件を満たした時点で更新する。
