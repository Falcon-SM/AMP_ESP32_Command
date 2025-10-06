#include <Arduino.h>
#include <driver/i2s.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// --- SDカードのピン設定 ---
#define SD_CS_PIN 5

// --- GPIOのピン設定 ---
const int KIRITSU_PIN = 22;
const int KIWOTSUKE_PIN = 35;
const int REI_PIN = 32;

// --- オーディオ設定 ---
// 44.1kHzの音声ファイルに合わせて設定
const int SAMPLE_RATE = 44100 / 2;
// 一度にSDカードから読み書きするバッファサイズ（バイト単位）
const int BUFFER_SIZE = 1024;
// 音量調整 (0.0 ～ 1.0)
const float VOLUME = 1.0;

// --- グローバル変数 ---
// SDカード上のファイルオブジェクト
File kiritsuFile;
File kiwotsukeFile;
File reiFile;

// ボタンの前の状態を記憶するための変数
int lastKiritsuState = LOW;
int lastKiwotsukeState = LOW;
int lastReiState = LOW;


/**
 * @brief I2Sの初期設定を行う
 */
void setupI2S() {
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_8BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_MSB,
      .intr_alloc_flags = 0,
      .dma_buf_count = 8,
      .dma_buf_len = 64,
      .use_apll = true, // trueにすることで対応できるサンプリングレートの範囲が広がる
      .tx_desc_auto_clear = true
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, NULL); // 内蔵DACを使用
  i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

/**
 * @brief 指定されたファイルを最後まで再生する
 * @param file 再生するファイルオブジェクト
 */
void playFile(File &file) {
  if (!file) {
    Serial.println("File invaild.");
    return;
  }

  // ファイルの読み取り位置を先頭に戻す
  file.seek(0);

  // 再生用のバッファ
  uint8_t buffer[BUFFER_SIZE];
  size_t bytesRead = 0;
  size_t bytes_written = 0;

  // ファイルに読み込めるデータがある間、ループする
  while ((bytesRead = file.read(buffer, BUFFER_SIZE)) > 0) {
    // 音量調整
    for (int i = 0; i < bytesRead; i++) {
      int16_t sample = (int16_t)buffer[i] - 128;
      sample = (int16_t)(sample * VOLUME);
      buffer[i] = (uint8_t)(sample + 128);
    }
    // I2Sに書き込んで音声を出力
    // (最後の書き込みでファイル終端に達するまで待機する)
    i2s_write(I2S_NUM_0, buffer, bytesRead, &bytes_written, portMAX_DELAY);
  }

  // 再生終了後、DMAバッファをクリアしてノイズを防ぐ
  i2s_zero_dma_buffer(I2S_NUM_0);
  Serial.println("Playing completed.");
}


void setup() {
  Serial.begin(115200);

  // ボタン用のピンを入力モードに設定
  pinMode(KIRITSU_PIN, INPUT);
  pinMode(KIWOTSUKE_PIN, INPUT);
  pinMode(REI_PIN, INPUT);

  // SDカードの初期化
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Failed to mount SD card.");
    while (1);
  }

  // オーディオファイルを開く
  kiritsuFile = SD.open("/commands/kiritsu.raw");
  kiwotsukeFile = SD.open("/commands/kiwotsuke.raw");
  reiFile = SD.open("/commands/rei.raw");

  if (!kiritsuFile || !kiwotsukeFile || !reiFile) {
    Serial.println("Couldn't open file.");
    while (1);
  }

  // I2Sのセットアップ
  setupI2S();
  Serial.println("Setup completed. Waiting for button to be pressed...");
}


void loop() {
  // 各ボタンの現在の状態を読み取る
  int currentKiritsuState = digitalRead(KIRITSU_PIN);
  int currentKiwotsukeState = digitalRead(KIWOTSUKE_PIN);
  int currentReiState = digitalRead(REI_PIN);

  // 「起立」ボタンが押された瞬間か？ (LOWからHIGHに変わった時)
  if (currentKiritsuState == HIGH && lastKiritsuState == LOW) {
    Serial.println("KIRITSU Pressed. Starting...");
    playFile(kiritsuFile);
  }

  // 「気をつけ」ボタンが押された瞬間か？
  if (currentKiwotsukeState == HIGH && lastKiwotsukeState == LOW) {
    Serial.println("KIWOTSUKE Pressed. Starting...");
    playFile(kiwotsukeFile);
  }

  // 「礼」ボタンが押された瞬間か？
  if (currentReiState == HIGH && lastReiState == LOW) {
    Serial.println("Rei Pressed. Starting...");
    playFile(reiFile);
  }

  // 現在のボタン状態を「前の状態」として保存する
  lastKiritsuState = currentKiritsuState;
  lastKiwotsukeState = currentKiwotsukeState;
  lastReiState = currentReiState;
  
  // CPUに少し余裕を持たせる
  delay(10);
}