# Aureline 取扱説明書

- 対象バージョン：1.0.10
- ブランド：Hidecade Instruments
- 対応環境：macOS、Windows、iPhone / AUv3

![Aurelineデスクトップ版](images/aureline-desktop.png)

## 1. はじめに

Aurelineは、2基のオシレーター、ノイズ、4段OTAローパスフィルター、2基の
エンベロープ、LFO、Poly Modを備えた8音ポリフォニック・アナログモデリング
シンセサイザーです。

デスクトップ版とiPhone版は同じ音源エンジンと音色データを使用します。
`.aurelinevoice`、`.aurelinelibrary.xml`、`.aurelinewave`ファイルを使って、
環境間で音色やWave Memoryを交換できます。

## 2. 対応形式

| プラットフォーム | 形式 |
|---|---|
| macOS | Standalone、VST3、Audio Unit |
| Windows | Standalone、VST3 |
| iPhone | Standalone App、AUv3 Instrument |

## 3. インストール

### macOS

使用する形式のインストーラーを実行します。

- `Aureline-Standalone-1.0.10-macOS.pkg`
- `Aureline-VST3-1.0.10-macOS.pkg`
- `Aureline-AU-1.0.10-macOS.pkg`

インストーラーはApple公証済みです。インストール後、DAWを再起動して
プラグインを再スキャンしてください。

### Windows

Windows用ZIPを展開します。Standalone版はアプリケーションを直接起動します。
VST3版は使用するDAWのVST3検索対象へ配置し、プラグインを再スキャンします。

### iPhone

App Storeからインストールします。Standalone Appとして演奏できるほか、
AUv3対応ホストからInstrumentとして読み込めます。

## 4. 最初の音を出す

1. Aurelineを起動します。
2. ヘッダーの`Audio`表示で出力デバイスを確認します。
3. `MIDI`表示で入力元を確認します。
4. 音色名を選びます。
5. MIDI鍵盤または画面下部の鍵盤を演奏します。
6. `VOLUME`と`MASTER`で音量を調整します。

Standalone版のAudio/MIDI機器はウインドウ上部の`Options`から設定します。
プラグイン版ではヘッダーに`Audio: host | MIDI: host`と表示され、接続はホストが
管理します。

音が出ない場合は、`MASTER`、Amp Envelopeの`SUSTAIN`、Mixerの`VCO A / VCO B`、
フィルターの`CUTOFF`を確認してください。

## 5. 画面構成

### ヘッダー

- `AURELINE`：製品名とバージョン
- `Audio`：現在の出力デバイス
- `MIDI`：現在選択されているMIDI入力
- `WAV`：最終ステレオ出力の録音
- `SAVE BANK`：現在の32音色バンクを書き出す

### 音色操作

- 音色名：8バンク、各32音色から選択
- `<` / `>`：前後の音色へ移動
- `LOAD`：単一音色または音色ライブラリを読み込む
- `SAVE`：現在の単一音色を書き出す
- `COPY`：現在の音色をコピー
- `PASTE`：コピーした音色を現在位置へ一時的に適用
- `INIT`：初期音色を一時的に適用
- `STORE`：現在の編集内容を選択スロットへ保存

`LOAD`、`PASTE`、`INIT`だけではバンクへ確定保存されません。内容を残す場合は、
必ず保存先スロットを選んで`STORE`を押してください。

## 6. 音色バンク

Aurelineには、32音色ずつの8バンクがあります。BANK 3はCircuit、
BANK 4はRetro、BANK 5は8-Bit、BANK 8はDRUM KIT専用です。

1. `Analog 1`：Brass、Strings/Pad、Piano/Keys、Bass、SE
2. `Analog 2`：Lead、Poly Mod/Sync、Percussion、Rhythm、SE
3. `Circuit`：アナログ・ドラム／パーカッションの通常音色
4. `Retro`：ビンテージゲーム／アーケード風
5. `8-Bit`：Pulse、Noise、Wave Memory、PSG／拡張音源風
8. `DRUM KIT`：KITモード専用の32音色

BANK 8の音色は、鍵盤上の位置と選択スロットが一致するように
MIDIノートの低い順から高い順へ並んでいます。

各バンクの最後4音色は効果音です。

### DRUM KIT

PERFORMANCEの`KIT`をオンにすると、BANK 8「DRUM KIT」の32音色を
1つのドラムセットとして鍵盤／MIDIから演奏できます。各キーは独立した
音色スナップショットで発音するため、Kick、Snare、Hatなどを最大8音まで
同時に鳴らせます。中心となる配置はMIDIドラムセットに準じています。
デスクトップ版の画面鍵盤は、通常モードとKITモードのどちらも
C2（MIDI 36）からC5まで表示します。

- MIDI 36 (C2): Deep Kick
- MIDI 38 (D2): Classic Snare
- MIDI 39 (D#2): Hand Clap
- MIDI 42 (F#2): Closed Hat
- MIDI 46 (A#2): Open Hat
- MIDI 49 (C#2): Metal Cymbal
- MIDI 41／45／48: Low／Mid／High Tom
- MIDI 56: Cowbell

Closed Hatを鳴らすとOpen Hatの余韻が止まります。KITをオンにした際は
ARPEGGIOとCHORDが自動的にオフになります。BANK 8の音色をSTOREした場合、
ドラムセットにも更新内容が反映されます。

全32音色のキーマップと内部動作は
[ドラムキット演奏モード仕様](Aureline_Drum_Kit_Spec_ja.md)を参照してください。

### 単一音色の保存

`SAVE`を押し、`.aurelinevoice`として保存します。音源設定、Performance設定、
Wave Memory波形が1ファイルに保存されます。

### バンクの保存

`SAVE BANK`を押すと、現在のバンク全32音色を
`.aurelinelibrary.xml`として保存します。

### バンクの読み込み

`LOAD`で`.aurelinelibrary.xml`を選ぶと、読み込み先バンクを選択する画面が
表示されます。選択したバンクの32音色は置き換えられるため、必要なら先に
`SAVE BANK`でバックアップしてください。

ライブラリにバンク名が保存されている場合、読み込み先バンクの表示名もその名前へ
変わります。バンク名は最大16文字で、デスクトップ版とiPhone版のBANK選択画面に
反映されます。名前を持たない旧形式のライブラリではファイル名を使用します。

## 7. Oscillator A

Oscillator Aは主音源で、Hard Sync時にはSlave Oscillatorとして動作します。

- `RANGE`：32′、16′、8′、4′、2′
- Saw：鋸歯状波
- Triangle：三角波
- Pulse：矩形／パルス波
- Wave Memory：32ステップのユーザー波形
- `PW`：Pulse Width
- `MEMORY`：Wave Memory波形を選択
- `CHARACTER`：`5-BIT`、`4-BIT`、`SMOOTH`
- `EDIT WAVE`：Wave Memoryエディターを開く

複数の波形ボタンを同時にオンにすると、波形を重ねられます。

## 8. Oscillator B

Oscillator Bは第2音源であり、Poly ModとHard Syncの変調源にもなります。

- `RANGE`：オクターブ範囲
- Saw / Triangle / Pulse / Wave Memory
- `PW`：Pulse Width
- `DETUNE`：Oscillator Aに対する微調整
- `LF`：低周波動作。ゆっくりした変調源として使用
- `KB`：鍵盤追従

`DETUNE`はOscillator BのFine Tuneです。値を少し変えると厚みが生まれ、
大きく変えると音程差のある2オシレーター音になります。

Oscillator BのMixer Levelを0にしても、Poly Modの変調源としては動作します。

## 9. Mixer

- `VCO A`：Oscillator Aの出力レベル
- `VCO B`：Oscillator Bの出力レベル
- `NOISE`：ノイズレベル

レベルを上げると単に音量が増えるだけでなく、フィルター入力の飽和感も
強くなります。Poly ModだけにOscillator Bを使う場合は`VCO B`を0にします。

## 10. Filter

Aurelineは、レゾナンスと自己発振に対応する独自の4段OTAローパスフィルターを
各ボイスに搭載しています。

- `CUTOFF`：フィルターの基準カットオフ
- `RESONANCE`：カットオフ周辺の強調。最大付近では自己発振
- `FILTER ENV`：Filter Envelopeの深さ
- `KEY TRACK`：鍵盤位置によるカットオフ追従
- `VELOCITY`：演奏ベロシティによる反応

高いResonanceでは低域が細く感じられることがあります。これは共振型
ローパスフィルターの音響的な性質です。必要に応じてOscillator Range、
Mixer Level、Cutoffを調整してください。

## 11. Filter Envelope / Amp Envelope

各エンベロープはADSR方式です。

- `ATTACK`：鍵盤を押してから最大値へ達する時間
- `DECAY`：最大値からSustain値へ移る時間
- `SUSTAIN`：鍵盤を押している間の保持レベル
- `RELEASE`：鍵盤を離してから消えるまでの時間

`FILTER ENV`はフィルターの動き、`AMP ENV`は音量の動きを作ります。

打楽器音ではAttackを短く、Decayを短め、Sustainを低くします。PadではAttackと
Releaseを長くします。非常に短いAttackの音を連打した場合も、Aurelineは
再トリガー時のクリックを抑える処理を行います。

## 12. Poly Mod

Poly Modは、各ボイス内で次の2信号を合成します。

```text
Oscillator B × OSC B Amount
          +
Filter Envelope × FILTER ENV Amount
```

合成した信号を、選択したDestinationへ送ります。

- `FREQ A`：Oscillator Aの周波数
- `PW A`：Oscillator AのPulse Width
- `FILTER`：Filter Cutoff

### 金属的な音を作る

1. Oscillator Bの`LF`をオフにします。
2. Oscillator Bの`KB`をオンにします。
3. Oscillator BでTriangleまたはSawを選びます。
4. Poly Modの`OSC B`を上げます。
5. `FREQ A`をオンにします。
6. Oscillator Bの`RANGE`と`DETUNE`を調整します。

Oscillator Bが可聴周波数で動作すると、通常のビブラートではなく
オーディオレート変調になり、ベル、金属、非整数倍音を含む音になります。
深くかけるほど原音の音程感は弱くなるため、まず少量から調整してください。

### Hard Sync音を作る

1. Performanceの`SYNC`をオンにします。
2. Oscillator AとBのRangeを変えます。
3. Poly Modの`FILTER ENV`を上げます。
4. `FREQ A`をオンにします。
5. Filter EnvelopeのAttack / Decayを調整します。

Sync時はOscillator BがMaster、Oscillator AがSlaveです。Oscillator Aの周波数を
Envelopeで動かすと、明るく切り裂くようなSync Sweepが得られます。

## 13. LFO

- `LFO RATE`：変調速度
- Triangle / Saw Up / Saw Down / Square / Sample & Hold
- `MOD AMT`：初期変調量
- `LFO DELAY`：ノートオンから変調開始までの待ち時間
- `LFO FADE`：変調が最大になるまでの時間
- `RETRIG`：ノートオン時にLFO位相を再スタート

Destination：

- `A FREQ`
- `B FREQ`
- `PW A`
- `PW B`
- `FILTER`

LFO変調は周期的なビブラート、PWM、ワウ、ランダム変化に向きます。Poly Modの
Oscillator B変調は、より速いオーディオレート変調にも使用できます。

## 14. Performance

- `VOLUME`：音色側の出力レベル
- `VINTAGE`：ボイスごとの微小なPitch、Filter、Envelope、Gain、Pan差
- `TEMPO`：Arpeggiatorのテンポ
- `MASTER`：最終出力レベル
- `SPREAD`：ボイスのステレオ幅
- `TRANSPOSE`：半音単位の移調
- `MONO`：単音モード
- `UNISON`：複数ボイスを重ねる
- `SYNC`：Oscillator Hard Sync

### 演奏コントロール

- `PITCH`：Pitch Bend Wheel
- `MOD`：Modulation Wheel
- `RANGE`：Pitch Bend幅
- `UNI DETUNE`：Unison時の音程のばらつき
- `MOD RANGE`：Modulation Wheelの深さ
- `GLIDE`：音程の滑らかな移動時間
- `ARP`：Arpeggiator
- `CORD`：Chord Mode
- `LEGATO`：重ねて弾いた場合だけGlide / Mono Legatoを適用
- `HOLD`：鍵盤を離しても音またはArpeggioを保持

デスクトップ版の画面鍵盤では、C音の下部に`C2`、`C3`、`C4`のような
オクターブ名を表示します。PCキーボード演奏は下段`Z`をC2、上段`Q`をF3
として割り当てます。上段の黒鍵は`2`=F#3、`3`=G#3、`4`=A#3、
`6`=C#4、`7`=D#4の順です。日本語配列では`_`をF3（MIDI 53）、`]`をF#3
（MIDI 54）、`¥`をC#5（MIDI 73）として使用できます。

## 15. Arpeggiator

- `SCALE`：基準音
- `ARP RATE`：音符間隔
- `DIRECTION`：`UP`、`DOWN`、`U/D`、`RND`
- `GATE`：各音の長さ
- `TEMPO`：全体の速度

`ARP`をオンにして複数の鍵盤を押します。`HOLD`を併用すると、鍵盤を離した後も
パターンを継続できます。

## 16. Wave Memory Editor

`EDIT WAVE`を押すと、Oscillatorごとの32ステップ波形を編集できます。

- グラフをドラッグ：各ステップの値を描画
- `STEP`：現在選択中のステップ
- `<` / `>`：選択ステップを移動
- `VALUE`：選択ステップの値
- `AUDITION C4`：編集中の波形をC4で試聴
- `COPY` / `PASTE`：波形をコピー／貼り付け
- `INIT`：波形を初期化
- `LOAD` / `SAVE`：`.aurelinewave`を読込／保存
- `CLOSE`：エディターを閉じる

デスクトップ版では、グラフにフォーカスがある状態で次のキーを使用できます。

- `←` / `→`：ステップ移動
- `↑` / `↓`：値を上下

Value入力中も`←` / `→`でステップを移動できます。選択中のステップは色で
強調表示されます。

`AUDITION C4`を解除すると、試聴ノートは通常のAmp Envelope Releaseに従って
消音します。

iPhone版ではタッチ操作に適した専用レイアウトを使用し、数値キーボードによる
直接入力は行いません。

## 17. FINAL MIXモニター

`FINAL MIX`は、フィルター、Amp Envelope、Poly Modを含む最終出力波形を表示します。
押した鍵盤のノートオンを基準に波形を揃えるため、波形が横へ流れ続けにくく、
音色ごとの形を比較できます。

`OSC A`と`OSC B`の小型モニターは、各オシレーターの波形確認に使用します。
モニターは視認性のため表示振幅を増幅しており、正確な音量メーターではありません。

## 18. WAV録音

WAV録音はmacOS／WindowsのStandalone版で使用できます。

1. `WAV`を押すと録音が開始され、ボタンが`STOP`になります。
2. 演奏します。
3. `STOP`を押します。
4. 保存場所とファイル名を指定します。

現在の最終ステレオ出力を24-bit WAVで保存します。プラグイン版では、DAWの
録音／書き出し機能を使用してください。

`WAV recording is empty`と表示された場合は、録音開始後に実際の音声が
出力されたか、Audioデバイスが有効かを確認してください。

## 19. iPhone版

![Aureline iPhone版](images/aureline-iphone.png)

iPhone版は横画面で使用します。画面下部の鍵盤を残したまま、Play / Edit画面を
切り替えて操作します。音源パラメータ、8バンク、Wave Memory、音色ファイルは
デスクトップ版と共通です。
画面鍵盤の左端は通常音色ではC3、KITモードではキックを含むC2へ自動的に
切り替わります。

AUv3版ではAudioとMIDIをホストアプリが管理します。Standalone版ではCore MIDI入力と
画面鍵盤を使用できます。

ファイルAppの共有機能を使って、Macと次のデータを交換できます。

- `.aurelinevoice`
- `.aurelinelibrary.xml`
- `.aurelinewave`

## 20. MIDI

対応する主なMIDIメッセージ：

| MIDI | 動作 |
|---|---|
| Note On / Off | 発音／消音 |
| Velocity | Filter / Ampの強弱 |
| Pitch Bend | Pitch Wheel |
| CC 1 | Mod Wheel |
| CC 64 | Sustain Pedal |
| CC 120 / 123 | Panic / All Notes Off |

MIDI入力名が長い場合も、ヘッダー内で右端揃えされ、`WAV`ボタンへ重ならないよう
表示範囲が制限されます。

## 21. トラブルシューティング

### 音が出ない

- Audio出力が`off`になっていないか確認する
- MIDI入力名または`all inputs`が表示されているか確認する
- `MASTER`と`VOLUME`を上げる
- MixerのVCOレベルを上げる
- Filter Cutoffを上げる
- Amp Envelope Sustainを上げる
- `HOLD`を解除し、All Notes Offを送る

### 音程が不安定／外れて聞こえる

- `DETUNE`、`VINTAGE`、`UNI DETUNE`を下げる
- `TRANSPOSE`とPitch Bend Wheelを確認する
- Poly Modの`FREQ A`を解除するか、`OSC B`量を下げる
- Oscillator Bの`LF`と`KB`設定を確認する

### Poly Modがゆっくり揺れるだけ

Oscillator Bの`LF`をオフにしてください。金属的な音には、Oscillator Bを
可聴周波数で動かし、`KB`をオンにして`FREQ A`へ送ります。

### 音色を変更したのに再起動後に戻った

編集後に`STORE`を押してください。`LOAD`、`PASTE`、`INIT`は一時適用です。
外部バックアップには`SAVE`または`SAVE BANK`を使用します。

### プラグインがDAWに表示されない

- DAWを再起動する
- プラグインを再スキャンする
- macOSではAU / VST3、WindowsではVST3の形式がDAWに合っているか確認する
- Apple Silicon環境ではDAWとプラグインのアーキテクチャ設定を確認する

## 22. ファイル形式

| 拡張子 | 内容 |
|---|---|
| `.aurelinevoice` | 単一音色、Performance、Wave Memory |
| `.aurelinelibrary.xml` | 32音色のバンク |
| `.aurelinewave` | 32ステップWave Memory波形 |
| `.wav` | Standalone版で録音した24-bitステレオ音声 |

重要な音色は、定期的に`SAVE BANK`で別の場所へバックアップしてください。

## 23. サポート情報

- [Aureline GitHub](https://github.com/Hidecade/Aureline)
- [最新リリース](https://github.com/Hidecade/Aureline/releases/latest)
- [製品仕様](Aureline_Spec_ja.md)
- [iPhone／AUv3実装情報](../iOS/AurelineMobile/README.md)
