/*
    ESP32 AnyAudio Player
*/

#include <SPI.h>
#include <Audio.h>
#include "SdFat.h"
#include <RBD_Button.h>

#define I2S_USED false  // to output audio via I2S, please change 'false' into 'true'.
                        // 为了通过I2S播放音频，请将‘false’改成‘true’
#if I2S_USED
#define I2S_BCLK 27  // I2S BCLK Pin. U can change it if u want. | I2S BCLK引脚，可修改
#define I2S_LRC 33   // I2S LRC Pin. U can change it if u want. | I2S LRC引脚，可修改
#define I2S_DOUT 32  // I2S DOUT Pin. U can change it if u want. | I2S DOUT引脚，可修改
#endif

Audio audio;
#define S_D SD

RBD::Button PrevBtn(16);    // Press 16 to play previous music        | 上一首歌
RBD::Button NextBtn(17);    // Press 17 to play next music            | 下一首歌
RBD::Button PauseBtn(14);   // Press 14 to PauseResume                | 暂停/恢复
RBD::Button TenNextBtn(4);  // Just like press NextBtn(17); 12 times  | 无异于按下NextBtn(17);十二次

#define doge 64  // [doge]here means the maximum length of each music name(Byte) | doge 是每个音频文件名称的最大长度，单位是Byte
char openNextBuf[doge];
#define SD_CS_PIN 5
#define SD_SPEED 500 * 1000  // 500kHz for SD Card, This speed is low enough.| SD卡SPI频率为500kHz，速度够低了。

FsFile mainFile;
FsFile dir;
FsFile openNextFile;
FsFile openPrevFile;

String which_music_2_play;  // which music 2 play(And its path) | 播放哪首歌(包括它的路径)
char c;
unsigned char mp3_eof_detectd = 0;

String musicDir = "/Alexandif OS/Users/_Public/Musics/";         // your music path | 你的音频文件路径
String cacheDir = "/Alexandif OS/System/Cache/music_cache.txt";  // your cache path | 你的音频缓存路径
unsigned int InNeed_Music_Location = 0;
unsigned short amount_of_Found_Music = 0;

void setup() {
  Serial.begin(115200);
  // Serial.println("[E] ESP32曰：有朋自远方来，必先苦其心志，劳其筋骨，饿其体肤，空乏其身，行拂乱其所为。然后鞭三十，驱之于外户，虽远必诛。子曰：不亦乐乎!");

  disableCore0WDT();
  pinMode(2, OUTPUT);
  //randomSeed(analogRead());

  Serial.println("Installing SD card...");

  if (!S_D.begin(SdSpiConfig(5, DEDICATED_SPI, SD_SPEED))) {
    // S_D.initErrorHalt(&Serial);
    digitalWrite(2, HIGH);
    vTaskDelay(100);
    abort();
  } else {
    Serial.println("SD initialization done.");
  }
  vTaskDelay(10);
  mainFile = S_D.open(cacheDir.c_str(), FILE_WRITE);
  vTaskDelay(10);
  if (!dir.open(musicDir.c_str())) {
    Serial.println("dir.open failed");
    digitalWrite(2, HIGH);
    vTaskDelay(100);
    abort();
  }
  vTaskDelay(10);
  S_D.remove(cacheDir.c_str());
  // 删除旧的缓存文件（内容上，运用删除的写作手法，写出了作者对旧缓存文件无理占用SD卡空间的讨厌直至愤恨之情；结构上，为下文创建新缓存文件作铺垫
  // delete old cache
  vTaskDelay(10);
  if (!mainFile.open(cacheDir.c_str(), FILE_WRITE)) {
    Serial.println("dir.open failed");
    digitalWrite(2, HIGH);
    vTaskDelay(100);
    abort();
  }

  while (openNextFile.openNext(&dir, O_RDONLY)) {
    memset(openNextBuf, 0, doge);
    openNextFile.getName(openNextBuf, doge);
    mainFile.write(openNextBuf);
    mainFile.write("\n");
    //while ()
  }

  mainFile.sync();
  vTaskDelay(50);
  mainFile.close();
  openNextFile.close();
  dir.close();
  Serial.print("OpenNext Done!");

  mainFile = S_D.open(cacheDir.c_str());
  if (mainFile) {
    Serial.println("Cache Content:");

    while (mainFile.available()) {
      c = mainFile.read();
      Serial.write(c);

      if (c != '\n') {
      } else {
        Serial.println("__MUSIC!!!DETECTED!!!");
      }
    }
    // close the file:
    mainFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("Fuck!");
    vTaskDelay(100);
    abort();
  }

  audio.setVolume(21);  // 0......21
#if I2S_USED
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
#endif
  nextMusic(1);
}
/*
void prevMusic() {
  if (openPrevFile.open(musicDir.c_str())) {
    openPrevFile.seekEnd(0);  // Set file pointer to the end of the directory
    Serial.println("PrevMusic is alive");
    openPrevFile.prev();
    if (openNextFile.isFile()) {
      which_music_2_play += openPrevFile.name();
    } else {
      Serial.println("Not a file");
    }
    openPrevFile.close();
    Serial.println(which_music_2_play);
    Serial.println("Done.")
  } else {
    Serial.println("Fuck! Failed to open musics.")
  }
}

*/
void nextMusic(signed short nextTime) {
  mainFile = S_D.open(cacheDir.c_str());
  InNeed_Music_Location = InNeed_Music_Location + nextTime;
  if (mainFile) {
    mainFile.rewind();
    while (amount_of_Found_Music < InNeed_Music_Location) {
      c = mainFile.read();
      if (c == '\n') {
        amount_of_Found_Music++;
      }
    }

    c = mainFile.read();
    while (c != '\n') {
      which_music_2_play += c;
      c = mainFile.read();
    }

    amount_of_Found_Music = 0;
    Serial.println("which_music_2_play is:" + which_music_2_play);
    audio.connecttoFS(SD, which_music_2_play.c_str());
    vTaskDelay(20);
    which_music_2_play = musicDir;
  } else {
    Serial.println("FUCK!");
    vTaskDelay(100);
    digitalWrite(2, HIGH);
    vTaskDelay(100);
    digitalWrite(2, LOW);
  }
}

void loop() {
  audio.loop();

  if (PrevBtn.onPressed()) {
    Serial.println("PrevBtn Onpressed");
    vTaskDelay(10);
    nextMusic(-1);
  }
  if (NextBtn.onPressed()) {
    Serial.println("NextBtn Onpressed");
    vTaskDelay(10);
    nextMusic(1);
  }
  if (PauseBtn.onPressed()) {
    Serial.println("PauseBtn Onpressed");
    vTaskDelay(10);
    audio.pauseResume();
  }
  if (TenNextBtn.onPressed()) {
    nextMusic(12);
    vTaskDelay(10);
  }
}


// optional
void audio_info(const char *info) {
  Serial.print("info        ");
  Serial.println(info);
}

void audio_id3data(const char *info) {  //id3 metadata
  Serial.print("id3data     ");
  Serial.println(info);
}

void audio_eof_mp3(const char *info) {  //end of file
  Serial.print("eof_mp3     ");
  Serial.println(info);
  vTaskDelay(10);
  nextMusic(1);
  while (1) {
    audio.loop();
  }
  vTaskDelay(2147483647);
}
