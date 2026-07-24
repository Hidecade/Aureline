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

## 音色・WAVEデータの拡張子

Aureline固有データには次の拡張子を使用します。

| データ | 拡張子 | 内容 | 実装状況 |
|---|---|---|---|
| 単一音色データ | `.aurelinevoice` | Mac／iPhone共通JSON。1音色分のシンセパラメータ、Voice Mode、Wave Memoryなど | Mac／iPhoneで実装済み |
| WAVEデータ | `.aurelinewave` | Wave Memory AまたはBの32ステップ波形、Character、フォーマットバージョン | 形式予約。入出力は未実装 |
| 全音色データ | `.aurelinelibrary.xml` | 番号付き50音色をまとめたXMLライブラリ／バックアップ | Mac版で実装済み |

命名規則はOpalineFMと共通です。OpalineFMの`.opalinevoice`／`.opalinelibrary.xml`に対応して、Aurelineでは`.aurelinevoice`／`.aurelinelibrary.xml`を使用します。WAVE Memory単体は同じ「製品名＋データ種別」の規則で`.aurelinewave`とします。

Mac／iPhoneとも単一音色には`.aurelinevoice`を使用します。

LOADとPASTEは、読み込んだ音色データを現在選択中の番号へ一時的に反映します。この時点では、その番号の保存データは変更されません。選択中の番号を上書き保存する操作はSTOREだけです。STOREせずに別の音色へ移動してから戻ると、以前STOREした音色、または一度もSTOREしていない場合は工場出荷時の音色へ戻ります。SAVEは選択番号を変更せず、現在の音を外部の`.aurelinevoice`ファイルへ書き出します。

`.xml`および`.json`はデバッグまたは互換インポートに使用できますが、ユーザー向けの標準拡張子には使用しません。

Mac版のSAVE ALL LIBRARYは、50個すべての番号について最後にSTOREされた内容を1つの`.aurelinelibrary.xml`へ保存します。LOADでこのファイルを開くと確認メッセージを表示し、承認後に50音色すべてを置き換えます。

Mac版はAurelineの書類フォルダに出荷状態復元用の`factory.aurelinelibrary.xml`を用意します。このファイルをLOADして確認すると、50スロットすべてを出荷状態へ戻せます。SAVE ALL LIBRARYでは、この予約ファイル名への上書きを拒否します。

同じフォルダに`RetroGame.aurelinelibrary.xml`も用意します。初期8-bit機風のパルス／三角波、矩形波PSG、5-bitウェーブテーブル、80年代アーケード風の波形メモリ、ノイズ効果音を使ったオリジナル50音色を収録します。この内蔵ファイル名もSAVE ALL LIBRARYでは上書きできません。

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
