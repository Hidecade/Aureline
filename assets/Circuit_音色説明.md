# DRUM KIT Library

`Circuit.aurelinelibrary.xml`（アプリへのインストール名は
`DrumKit.aurelinelibrary.xml`）は、AurelineのBANK 8で管理する
32音色のドラムキット専用ライブラリです。

Aurelineのオシレーター、ノイズ、フィルター、エンベロープ、Poly Modを
使ったオリジナル音色です。

## 音色構成

1. DEEP KICK
2. RIM SHOT
3. CLASSIC SNARE
4. HAND CLAP
5. TIGHT SNARE
6. LOW TOM
7. CLOSED HAT
8. DISCO TOM
9. METAL HAT
10. MID TOM
11. OPEN HAT
12. ELECTRO SNARE
13. HIGH TOM
14. SHORT CYMBAL
15. HAT PULSE
16. METAL CYMBAL
17. ACCENT KICK
18. SHORT KICK
19. BOOM KICK
20. TUNED KICK
21. COWBELL
22. CLICK KICK
23. SUB DROP
24. NOISE SNARE
25. HIGH CONGA
26. MID CONGA
27. LOW CONGA
28. CLAVES
29. SUB BASS
30. MUTED COWBELL
31. MARACAS
32. TRIGGER FX

キック、タム、コンガはFilter EnvelopeからOscillator Aのピッチへ送る
Poly Modで、発音直後の急激なピッチ下降を作っています。Cowbell、
Hi-Hat、Cymbalは2つのパルス波、非整数デチューン、Oscillator Bの
Poly Modとノイズを組み合わせています。Cowbellは例外として、約540 Hzと
約800 Hzに相当する2つのパルス波を直接重ね、通常のFILTERをBPモードにして
2.64 kHz付近を強調し、短い減衰を与えています。

HAT PULSEはArpeggioを有効にしたリズム演奏用音色です。MUTED COWBELLは、
通常のCOWBELLより低く短い、乾いたアクセント用音色です。

## 読み込み

Aurelineの`LOAD`から`Circuit.aurelinelibrary.xml`を選び、読み込み先の
バンクを指定します。選択したバンクの32音色は上書きされます。
