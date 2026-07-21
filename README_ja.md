# Aureline

Aurelineは、Hidecade Instrumentsによる8音ポリフォニック・アナログモデリングシンセサイザーです。

Opaline FMとは別製品として開発し、音源、パッチ、UI、プリセット、プラグインIDを分離します。共通ライブラリには、両製品で実装と要件が一致した汎用リアルタイム部品だけを将来抽出します。

## 現在の実装

- 1ボイスあたり2基のVCO
- Saw、Pulse、Triangle波形
- Filter ADSRおよびAmp ADSR
- レゾナンス付き4-poleローパスフィルター
- 8音のボイス割り当てとボイススティール
- サステインペダルとレンジ設定可能なピッチベンド
- LFO、ノイズ、モジュレーションホイール、ボイス単位のPoly Mod
- オシレーターHard SyncとVCO A/B個別のPWM
- Poly、Mono Legato、8ボイスUnisonとGlide
- Constant-power方式のステレオVoice Spread
- Filter VelocityとKeyboard Tracking
- CMakeによるヘッドレス・エンジンテスト

JUCE Standaloneは実装済みです。Opaline FMと同じ1024×668のウインドウ、上部パネル、Pitch／Modホイール、37鍵キーボード、標準オーディオ出力、MIDI入力を備えます。

VST3およびmacOS Audio Unitも同じ音源・UIを使用するプラグインとしてビルドできます。成果物は `build/Aureline_Plugin_artefacts/VST3/Aureline.vst3` と `build/Aureline_Plugin_artefacts/AU/Aureline.component` に生成されます。

macOS用のStandalone、VST3、Audio Unitインストーラーは次のコマンドで`dist/`へ生成できます。

```sh
./scripts/build-macos-installers.sh
```

配布用署名を行う場合は`AURELINE_APPLICATION_SIGN_IDENTITY`と`AURELINE_INSTALLER_SIGN_IDENTITY`を指定します。未指定時はローカル検証用のad-hoc署名パッケージを生成します。

## テスト

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

macOSでは次のコマンドで起動できます。

```sh
open build/Aureline_Standalone_artefacts/Aureline.app
```

ローカル開発時は兄弟フォルダのOpalineFMにあるJUCEを再利用できます。独立環境では`AURELINE_JUCE_DIR`を指定するか、`external/JUCE`へJUCEを配置します。

製品仕様は [docs/Aureline_Spec_ja.md](docs/Aureline_Spec_ja.md) を参照してください。
