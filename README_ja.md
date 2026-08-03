# Aureline

[English](README.md)

Aurelineは、Hidecade Instrumentsによる8音ポリフォニック・アナログモデリング
シンセサイザーです。クラシックなポリシンセの直接的な操作感に、Aureline独自の
音源エンジン、UI、Wave Memoryオシレーター、音色ライブラリを組み合わせています。

最新リリース：[v1.0.11](https://github.com/Hidecade/Aureline/releases/tag/v1.0.11)

![Aurelineデスクトップ版シンセサイザー画面](docs/images/aureline-desktop.png)

![Aureline iPhone版シンセサイザー画面](docs/images/aureline-iphone.png)

## 主な機能

- 1パート最大8音、Poly／Mono Legato／Unisonモード
- 1ボイスあたり2基の帯域制限オシレーターとNoise
- Saw、Triangle、Pulse、32ステップWave Memory波形
- Hard Sync、PWM、Oscillator B Detune、Low Frequencyモード
- Oscillator BとFilter Envelopeを音源とするボイス単位のPoly Mod
- Poly ModをOscillator A Frequency、Pulse Width、Filter Cutoffへ選択的に接続
- レゾナンスと自己発振に対応するAureline独自の4段OTAローパスフィルター
- Filter／Amplifier独立ADSR
- 複数波形、Delay、Fade、Retrigger、接続先選択を備えたLFO
- Stereo Spread、Vintage Voice Variation、Glide、Pitch Bend、Mod Wheel
- Arpeggiator、Chord、Hold、Tempo、Direction、Gate、Scale Root
- 32音色×8つの書換可能バンク：BANK 3=Circuit、4=Retro、5=8-Bit、8=DRUM KIT専用
- Mac／Windows／iPhone間で交換できる音色・バンクファイル
- macOS／Windows Standalone版の24-bit Stereo WAV録音
- デスクトップ版は4パート・マルチティンバー、各パート最大8音／全体最大16音の動的割り当て
- Part 1／2／3は通常音色（MIDI CH 1／2／3）、Part 4はDRUM KIT専用（CH 10）
- Part 1〜3はCC0（BANK 1〜7）とProgram Change（VOICE 1〜32）に対応
- ヘッダーでパートを選択し、パート別MIDI LEDで受信を確認

## 対応形式

| プラットフォーム | 形式 |
|---|---|
| macOS | Standalone、VST3、Audio Unit |
| Windows | Standalone、VST3 |
| iPhone | Standalone App、AUv3 Instrument |

デスクトップ版の各形式は、JUCEプラグインプロセッサ、音源エンジン、MIDI経路、
状態形式、演奏画面を共用します。iPhone AppとAUv3はSwiftUIのモバイル専用UIと
AppleのAudio機能を使用し、C++音源エンジンと音色データをデスクトップ版と
共有します。

## 音源構成

```text
Oscillator A ─┐
Oscillator B ─┼─ Mixer ─ OTA 4-pole LPF ─ VCA ─ Voice Pan ─ Output
Noise ────────┘

LFO ──────── Oscillator A/B Pitch、Pulse Width、Filter Cutoff
Poly Mod ─── Oscillator B + Filter Envelope
              └─ Oscillator A Frequency / Pulse Width / Filter Cutoff
```

Oscillator BはMixer Levelが0でもPoly Mod音源として動作します。そのため、
最終MIXへOscillator Bを出さずに、繊細な動き、オーディオレート変調、
金属的な音、Hard Sync Sweepを作れます。

## Wave Memory

Oscillator A／Bは、32ステップの単周期Wave Memoryをアナログ波形と同時に
使用できます。16種類の内蔵Wave Memoryを収録し、エディターではUSER波形の
描画、Copy、Paste、音色への保存が可能です。Wave MemoryにはOscillator Rangeと
Oscillator B Detuneが反映され、オーディオレートまたは低周波のPoly Mod音源
としても使用できます。

## 音色ライブラリ

Aurelineは、各32スロットの書換可能な8バンクを搭載します。

1. Analog 1 — Brass、Strings/Pad、Piano/Keys、Bass、SE
2. Analog 2 — Lead、Poly Mod/Sync、Percussion、Rhythm、SE
3. Circuit — アナログ・ドラム／パーカッション
4. Retro — コンパクトなビンテージゲーム／アーケード風サウンド
5. 8-Bit — Pulse、Noise、Wavetable、PSG、拡張音源風サウンド
6. Bank 6 — ユーザーバンク
7. Bank 7 — ユーザーバンク
8. DRUM KIT — 鍵盤配置された専用32音色

通常音色の内蔵バンクでは最後4スロットが効果音です。STOREは現在選択中のスロットを
上書きします。SAVEはバンクを変更せず、現在の音色を外部ファイルへ保存します。
SAVE BANKは32スロットすべてを書き出し、ライブラリをLOADすると置換先バンクを
選択できます。

BANK 8専用の[DRUM KITライブラリ](assets/Circuit.aurelinelibrary.xml)には、
Aurelineで新規作成した打楽器／リズム音色を32音色収録しています。収録音色と
合成方法は[音色説明](assets/Circuit_音色説明.md)を参照してください。

## ファイル形式

| データ | 拡張子 | 内容 |
|---|---|---|
| 単一音色 | `.aurelinevoice` | 音源、Performance、Wave Memoryを含む共通JSON音色 |
| 音色バンク | `.aurelinelibrary.xml` | 32音色を収録するVersion 2 XMLライブラリ |
| Wave Memory | `.aurelinewave` | 32ステップ波形とCHARACTERをMac／iPhone間で交換 |

配布・編集用ライブラリのソースは[`assets/`](assets/)に置きます。実行時の
ライブラリとユーザーが編集したバンクは、各プラットフォームのApplication
Support領域へ保存します。

## ビルド

デスクトップAppとプラグインにはJUCE 8.0.14を使用します。JUCEを
`external/JUCE`へ配置するか、`AURELINE_JUCE_DIR`を指定します。ローカル開発では
兄弟フォルダのOpalineFMにあるJUCEも使用できます。

```sh
cmake -S . -B build -DAURELINE_BUILD_STANDALONE=ON -DAURELINE_BUILD_PLUGINS=ON
cmake --build build --config Release
```

macOS用Standalone、VST3、Audio Unitのインストーラーは`dist/`へ生成します。

```sh
./scripts/build-macos-installers.sh
```

Windowsでは[Inno Setup 6](https://jrsoftware.org/isinfo.php)をインストールしてから、
Release版のStandalone、VST3、インストーラーを`dist/`へ生成します。

```powershell
.\scripts\build-windows-installer.ps1
```

配布用署名には`AURELINE_APPLICATION_SIGN_IDENTITY`と
`AURELINE_INSTALLER_SIGN_IDENTITY`を設定します。未指定時はローカル確認用の
ad-hoc署名パッケージを生成します。

iPhone／AUv3プロジェクトの生成とSimulatorビルド：

```sh
cd iOS/AurelineMobile
xcodegen generate
xcodebuild \
  -project AurelineMobile.xcodeproj \
  -scheme AurelineMobile \
  -sdk iphonesimulator \
  -destination 'generic/platform=iOS Simulator' \
  -derivedDataPath ../../build/ios-mobile \
  CODE_SIGNING_ALLOWED=NO \
  build
```

実機ArchiveではApp本体とAUv3 Extensionの両方にApple署名が必要です。

## テスト

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

操作方法は[Aureline取扱説明書](docs/Aureline_Manual_ja.md)を参照してください。

技術詳細は[製品仕様](docs/Aureline_Spec_ja.md)と
[iPhone／AUv3実装情報](iOS/AurelineMobile/README.md)を参照してください。
