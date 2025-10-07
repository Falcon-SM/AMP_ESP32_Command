# AMP_ESP32_Command

## 機能
SDカードにcommandsファイルをつくり、その中にkiritsu.raw, kiwotsuke.raw, rei.rawの3つのファイルをいれる。
再生したい音声をWAVファイルにおさめておき、Audacityで44100Hz, Stereo, Unsigned 8 bitでraw形式に出力した。
3つのボタンを配線して、そのボタンが押されたらそれぞれの音声が再生される仕組みになっている。
TPA2006というアンプキットを用いた。秋月電子で購入可能である。


