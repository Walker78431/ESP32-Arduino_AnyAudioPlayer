/*
    ESP32 AnyAudio Player
    By Walker78431 

    Make your ESP32 an MP5

    This code is build & debugged on ESP32-WROOM-32UE
    Cannot operate on ESP32-S2 || ESP32-C3 because AudioI2S(https://github.com/schreibfaul1/ESP32-audioI2S/)
    does not support.

    created 2022
    by Walker78431@github.com
    last modified 28 Jun 2023
    by Walker78431@github.com

    There was a story between SD_DLD(Download)_SPEED and SD_PLA(Play)_SPEED. Maybe it's useful for you.
    Two and a half months ago, I found it hard to enumerate (line 293 ~ 308) except the SD card is in 
    such a LOW speed (even less than 800 kHz :[ ). Yesterday I added large numbers of 'delayMicroseconds(100)' 
    as buffer then that bug was fixed, though it still needed about 2.5 seconds to load 534 
    songs.
    So, try to add some delay()s || delayMicroseconds()s as buffer when it always goes wrong.

*/

#include <SPI.h>
#include <Audio.h>
#include "SdFat.h"
#include <RBD_Button.h>
#include "esp_task_wdt.h"

#define WDT_TIMEOUT_MS 011451  // 4
// DEC 4905ms

#define I2S_USED false  // to output audio via I2S, please change 'false' into 'true' | 为了通过I2S播放音频，请将‘false’改成‘true'
#if I2S_USED
#define I2S_BCLK 27  // I2S BCLK Pin. U can change it if u want. | I2S BCLK引脚，可修改
#define I2S_LRC 12   // I2S LRC Pin. U can change it if u want. | I2S LRC引脚，可修改
#define I2S_DOUT 14  // I2S DOUT Pin. U can change it if u want. | I2S DOUT引脚，可修改
#endif

Audio audio;
#define S_D SD

const char *musicDir = "/Alexandif OS/Users/_Public/Musics/";                          // your music path | 你的音频文件路径
                                                                                       // ATTENTION:'musicDir' MUST ENDS WITH '/'!!!
                                                                                       // 注意：'musicDir'的结尾必须为'/'!!!
const char *cacheDir = "/Alexandif OS/System/AllCache/music_cache.txt";                // your cache path | 你的音频缓存路径
const char *cacheDir_Amount = "/Alexandif OS/System/AllCache/music_cache_amount.txt";  // your cache path | 你的音频缓存路径
const char *boot_audio = "/Alexandif OS/System/Apps/AnyAudioPlayer/Elytra_loop.mp3";   // it will play when ESP32 boots | 这首歌会在ESP32启动的时候播放

#define gamemode pinMode
                                  // SimpleMode, CtrlPressed, ShiftPressed, Ctrl&ShiftPressed
RBD::Button PrevBtn(39, false);   // Play the previous, Enum Backwards, Repeat Playlist in Reverse, Shuffle playback
RBD::Button NextBtn(36, false);   // Play the next, Enum Forwards, Repeat Playlist, TURN OFF
RBD::Button PauseBtn(34, false);  // Pause || Resume, Enum Back/Forwards(Random), Repeat playing a song, Play the last song and repeat playlist in reverse
RBD::Button LikeBtn(35, false);   // Still not used (NC)
#define ShiftBtn 13               // Shortcut key 'Shift'
#define CtrlBtn 0                 // Shortcut key 'Ctrl'

#define doge 128
char openNextBuf[doge];
// [doge]here means the maximum length of each music name(Byte).[doge] | doge 是每个音频文件名称的最大长度，单位是Byte[狗头]

// SD Card, Using HSPI to customize pins, so that wiring will be easier
// SD卡，用HSPI来自定义引脚，使布线更便捷
#define SD_PLA_SPEED 3 * 1000 * 1000
#define SD_DLD_SPEED SD_PLA_SPEED
#define SD_MISO_PIN 4
#define SD_CLK_PIN 16
#define SD_MOSI_PIN 17
#define SD_CS_PIN 5
SPIClass hspi(HSPI);

// File*s 文件指针
FsFile mainFile;
FsFile dir;
FsFile openNextFile;
FsFile openPrevFile;

String which_music_2_play;  // which music 2 play(And its path) | 播放哪首歌(包括它的路径)
char c;
unsigned char mp3_eof_detectd = 0;

unsigned long InNeed_Music_Location = 0;
unsigned long amount_of_Found_Music = 0;
unsigned long tot_music_amount = 0;
unsigned long likes[1024];
signed short liked_musics;

unsigned long Operating_Time = 0;
signed char music_mode_code = 0;

bool finished = false;
/*
void vol_Down_or_Up(bool isVolUp, unsigned short duration_ms) {
  unsigned char v_change = 0;
  while (v_change < 21) {
    if (isVolUp) {
      audio.setVolume(audio.getVolume() + 1);
    } else {
      audio.setVolume(audio.getVolume() - 1);
    }
    v_change++;
    vTaskDelay(duration_ms / 21);
  }
}
*/

void refreshMusicList() {
  /*
  Serial.println("Installing SD card...");  // Installin' SD Card
  hspi.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!S_D.begin(SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_DLD_SPEED, &hspi))) {
    S_D.initErrorHalt(&Serial);
  } else {
    Serial.println("SD initialization done.");
  }




  vTaskDelay(20);
  mainFile = S_D.open(cacheDir, FILE_WRITE);
  vTaskDelay(20);
  while (!dir.open(musicDir)) {
    Serial.println("dir.open failed.RETRY-");
    digitalWrite(2, HIGH);
    vTaskDelay(20);
    digitalWrite(2, LOW);
    vTaskDelay(20);
  }
  vTaskDelay(20);

  while (!S_D.remove(cacheDir)) {
    Serial.println("S_D.remove failed.RETRY-");
    digitalWrite(2, HIGH);
    vTaskDelay(20);
    digitalWrite(2, LOW);
    vTaskDelay(20);
  }
  // 删除旧的缓存文件（内容上，运用删除的写作手法，写出了作者对旧缓存文件无理占用SD卡空间的讨厌直至
  // 愤恨之情；结构上，为下文创建新缓存文件作铺垫
  // delete old cache

  if (!mainFile.open(cacheDir, FILE_WRITE)) {
    Serial.println("dir.open failed");
    digitalWrite(2, HIGH);
    vTaskDelay(100);
    abort();
  }

  if (dir.getError()) {
    Serial.println("DIR get err before opennext.");
    S_D.initErrorHalt(&Serial);
    abort();
  } else {
    Serial.println("Once!");
  }

  while (openNextFile.openNext(&dir, O_RDWR)) {
    //memset(openNextBuf, 0, doge);
    openNextFile.getName(openNextBuf, doge);
    mainFile.println(openNextBuf);
    Serial.println(openNextBuf);
    mainFile.flush();
    Serial.flush();
    //mainFile.write("\n");
    tot_music_amount++;

    if (dir.getError()) {
      Serial.println("openNext failed");
      S_D.initErrorHalt(&Serial);
      abort();
    } else {
      Serial.println("Once!");
      // delayMicroseconds(50);
    }
  }

  vTaskDelay(5);
  mainFile.sync();
  vTaskDelay(5);

  if (mainFile.getError()) {
    Serial.println("mainFile went wrong");
    S_D.initErrorHalt(&Serial);
    abort();
  } else {
    Serial.println("Once!");
    // delayMicroseconds(50);
  }
  Serial.println("DONE!");
  closeAllFile();

  mainFile = S_D.open(amount_cacheDir);
  mainFile.print(String(tot_music_amount));
  mainFile.close();

  Serial.print("OpenNext Done!");

  memset(openNextBuf, 0, doge);
  vTaskDelay(20);
  mainFile = S_D.open(cacheDir, O_RDWR);
  vTaskDelay(10);
  if (mainFile) {
    Serial.println("Cache Content:");

    while (mainFile.available()) {
      c = mainFile.read();
      Serial.write(c);

      if (c != '\n') {
      } else {
        Serial.println("Music!_");
      }
    }
    // close the file:
    mainFile.close();
    Serial.println(String(tot_music_amount) + " Musics found.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("Fuck!");
    vTaskDelay(100);
    abort();
  }


  if (!mainFile.open(amount_cacheDir)) {
    Serial.println("Something went wrong when opening amount_cache.Reboot pleaz!");
    return;
  }

  String s;
  while (mainFile.available()) { s += mainFile.read(); }
  tot_music_amount = s.toInt();
  s.clear();


  Serial.println("Removing SD Card...");
  S_D.end();
  vTaskDelay(2);
  */
  //mainFile = S_D.open(cacheDir, O_TRUNC);
  //mainFile.close();

  Serial.println("Installing SD card...");  // Installin' SD Card
  hspi.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!S_D.begin(SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_DLD_SPEED, &hspi))) {
    S_D.initErrorHalt(&Serial);
  } else {
    Serial.println("SD initialization done.");
  }


  S_D.remove(cacheDir);
  // 删除旧的缓存文件（内容上，运用删除的写作手法，写出了作者对旧缓存文件无理占用SD卡空间的讨厌直至愤恨之情；结构上，为下文创建新缓存文件作铺垫
  // delete old cache
  vTaskDelay(1);
  Serial.println("203 executed");

  if (!mainFile.open(cacheDir, O_CREAT | O_RDWR)) {
    Serial.println("dir.open failed");
    digitalWrite(2, HIGH);
    vTaskDelay(100);
    abort();
  }
  vTaskDelay(1);
  Serial.println("209 executed");

  if (!dir.open(musicDir, O_RDONLY)) {
    Serial.println("dir.open failed");
    digitalWrite(2, HIGH);
    vTaskDelay(100);
    abort();
  }
  vTaskDelay(1);
  Serial.println("218 executed");

  while (openNextFile.openNext(&dir, O_RDONLY)) {
    memset(openNextBuf, 0, doge);
    delayMicroseconds(100);
    openNextFile.getName(openNextBuf, doge);
    delayMicroseconds(100);
    mainFile.write(openNextBuf);
    delayMicroseconds(100);
    mainFile.write("\n");
    delayMicroseconds(100);
    mainFile.sync();
    delayMicroseconds(120);
    mainFile.flush();
    delayMicroseconds(60);
    tot_music_amount++;
    delayMicroseconds(60);
  }
  Serial.println("227 enum exected");
  vTaskDelay(2);

  mainFile.sync();
  vTaskDelay(2);
  Serial.println("244 enum_sync exected");
  mainFile.flush();
  Serial.println("247 enum_sync exected");
  vTaskDelay(3);
  mainFile.close();
  openNextFile.close();
  dir.close();

  Serial.println("OpenNext Done!" + String(tot_music_amount));
  vTaskDelay(1);
  mainFile = S_D.open(cacheDir, O_RDONLY);
  vTaskDelay(2);
  if (mainFile) {
    Serial.println("Cache Content:");

    while (mainFile.available()) {
      c = mainFile.read();
      Serial.write(c);

      if (c != '\n') {
      } else {
        Serial.println("__MUSIC_DETECTED!");
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

  mainFile = SD.open(cacheDir_Amount, O_CREAT | O_RDWR);
  if (mainFile) {
    mainFile.write(String(tot_music_amount).c_str());
    mainFile.close();
  } else {
    Serial.print("Fuck");
    vTaskDelay(70);
    abort();
  }
  Serial.flush();

  SD.end();
  vTaskDelay(100);
}

bool myDigitalRead(const char pin) {
  signed char total = 0;
  for (signed char i = 0; i < 16; i++) {
    total = total + digitalRead(pin);
  }
  return round(total);
}

void nextMusic(signed short nextTime) {
  audio.stopSong();
  vTaskDelay(1);
  Operating_Time = millis();
  which_music_2_play = musicDir;

  mainFile = S_D.open(cacheDir);
  delayMicroseconds(514);
  InNeed_Music_Location = InNeed_Music_Location + nextTime;

  if (InNeed_Music_Location >= tot_music_amount && InNeed_Music_Location < 4294967295 - 1919810) {
    InNeed_Music_Location = InNeed_Music_Location % tot_music_amount;
  } else if (InNeed_Music_Location >= 4294967295 - 1919810) {
    unsigned long a = 4294967296 - InNeed_Music_Location;
    unsigned long b = a % tot_music_amount;
    InNeed_Music_Location = tot_music_amount - b;
  }

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
    vTaskDelay(1);

  } else {
    Serial.println("FUCK!");
    digitalWrite(2, HIGH);
    vTaskDelay(/*11*/ 4514);
    digitalWrite(2, LOW);
    abort();
  }

  Serial.println("[I] Music Mode:" + music_mode_code);
}


void myEnum_Vector(signed char enumDirection) {
  audio.stopSong();

  unsigned long e_presstime_millseconds = millis();
  unsigned long e_duration;
  unsigned long e_length;
  delayMicroseconds(5);

  while (myDigitalRead(CtrlBtn) == 0) {}
  e_duration = millis() - e_presstime_millseconds;

  if (enumDirection == 0) {
    if (random(0, 2) == 0) {
      enumDirection = -114;  //514
    } else {
      enumDirection = 114;  //514
    }
  }
  if (enumDirection < 0) {  // Direction: Prev
    e_length = e_duration / -12;
    nextMusic(e_length);
  } else if (enumDirection > 0) {  // Direction: Next
    e_length = e_duration / 12;
    nextMusic(e_length);
  }
}


void playBootloadMusic() {
  audio.connecttoFS(SD, boot_audio);
  while (1) {
    audio.loop();
    if (millis() - Operating_Time > 0x1145 /*14*/ && millis() > Operating_Time && audio.getAudioFileDuration() - audio.getAudioCurrentTime() < 2) {
      audio.stopSong();
      return;
    }
  }
}










void setup() {
  //String tot_music_amount_str;
  Serial.begin(921600);

  //disableCore0WDT();
  gamemode(2, OUTPUT);
  gamemode(CtrlBtn, INPUT_PULLUP);
  gamemode(ShiftBtn, INPUT_PULLUP);

  digitalWrite(2, HIGH);
  vTaskDelay(2000);
  digitalWrite(2, LOW);

  if (myDigitalRead(0) == 0) {  // if we need to refresh the music list
    refreshMusicList();         // then the SD will be refreshed
  }

  Serial.println("(Re)Installing SD card...");  // Installin' SD Card
  hspi.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!S_D.begin(SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_PLA_SPEED, &hspi))) {
    S_D.initErrorHalt(&Serial);
  } else {
    Serial.println("SD initialization done.");
  }

  vTaskDelay(1);
  audio.setVolume(21);  // HIGHEST VOLUME, LOWEST SINR

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 0);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, 0);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
  vTaskDelay(1);

  mainFile = SD.open(cacheDir_Amount, O_RDONLY);
  if (mainFile) {
    tot_music_amount = mainFile.readString().toInt();
    vTaskDelay(1);
  } else {
    Serial.println("Fcuk!");
    abort();
  }

  Serial.println(String(tot_music_amount));
  vTaskDelay(10);
  Serial.println("[ATTENTION] ESP32曰：有朋自远方来，必先苦其心志，劳其筋骨，饿其体肤，空乏其身，行拂乱其所为。然后鞭三十，驱之于外户，虽远必诛。子曰：不亦乐乎！");

  mainFile.close();
  Serial.flush();
#if I2S_USED
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
#endif
  playBootloadMusic();
  esp_task_wdt_init(WDT_TIMEOUT_MS, true);
  nextMusic(1);
}
/*
void prevMusic() {
  if (openPrevFile.open(musicDir)) {
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
unsigned short findMusicInArray(unsigned long number) {
  for (unsigned short p = 0; p < 1024; p++) {
    if (likes[p] == number) {
      return p;  // 返回数字在数组中的索引位置
    }
  }
  return 0114514;  // 数字未找到，返回39244表示不存在
}


void loop() {
  audio.loop();
  esp_task_wdt_reset();

#define SimpleMode myDigitalRead(ShiftBtn) == 1 && myDigitalRead(CtrlBtn) == 1
#define CtrlMode myDigitalRead(ShiftBtn) == 1 && myDigitalRead(CtrlBtn) == 0
#define ShiftMode myDigitalRead(ShiftBtn) == 0 && myDigitalRead(CtrlBtn) == 1
#define GoodNight_and_ModeMode myDigitalRead(ShiftBtn) == 0 && myDigitalRead(CtrlBtn) == 0

  if (SimpleMode) {  // if Nothing is pressed, Normal Mode
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
      if (audio.isRunning()) {
        esp_task_wdt_reset();
        enableCore0WDT();
        enableCore1WDT();
      } else {
        disableCore0WDT();
        disableCore1WDT();
      }
    }
    if (LikeBtn.onPressed()) {
      Serial.println("Oh! It seemed that you like that.");
      vTaskDelay(1);
      if (findMusicInArray(InNeed_Music_Location) == 0114514) {
        likes[liked_musics] = InNeed_Music_Location;
        if (liked_musics >= 1023) liked_musics = -1;
        liked_musics++;
      }
    }
  } else if (CtrlMode) {
    if (PrevBtn.onPressed()) {
      Serial.println("PrevBtn Onpressed, so do ctrl");
      vTaskDelay(10);
      myEnum_Vector(-1);
    }
    if (NextBtn.onPressed()) {
      Serial.println("NextBtn Onpressed, so do ctrl");
      vTaskDelay(10);
      myEnum_Vector(1);
    }
    if (PauseBtn.onPressed()) {
      Serial.println("PauseBtn Onpressed, so do ctrl");
      vTaskDelay(10);
      myEnum_Vector(0);
    }
    if (LikeBtn.onPressed()) {
      Serial.println("Oh! It seemed that you dislike that.");
      vTaskDelay(1);
      unsigned short sss = findMusicInArray(InNeed_Music_Location);
      if (sss != 0114514) likes[sss] = tot_music_amount + 2;
    }
  } else if (ShiftMode) {
    /*
    0 -> Repeat playlist
    1 -> Repeat playlist in reverse
    2 -> Repeat song
    3 -> Shuffle playback
    */

    if (PrevBtn.onPressed()) {
      Serial.println("PrevBtn Onpressed, So do shift. Repeat playlist in Reverse!");
      vTaskDelay(1);
      music_mode_code = 1;
    }
    if (NextBtn.onPressed()) {
      Serial.println("NextBtn Onpressed, So do shift. Repeat playlist!");
      vTaskDelay(1);
      music_mode_code = 0;
    }
    if (PauseBtn.onPressed()) {
      Serial.println("PauseBtn Onpressed, So do shift. Repeat song!");
      vTaskDelay(1);
      music_mode_code = 2;
    }
    /*
    if (LikeBtn.onPressed()) {
      vTaskDelay(1);
      audio.stopSong();
      InNeed_Music_Location = 
    }
    */
  } else if (GoodNight_and_ModeMode) {
    if (PauseBtn.onPressed()) {
      disableCore0WDT();
      disableCore1WDT();

      vTaskDelay(1);
      Serial.println("3.Thanks for listening! I'm going to sleep now.");
      Serial.println("2.Author:Walker#78431@github.com");
      Serial.println("1.Good Night!Press any button to wake me up. Fuck you!");
      unsigned short i = 0;
      while (i < 25) {
        digitalWrite(2, LOW);
        vTaskDelay(10 * i + 5);
        digitalWrite(2, HIGH);
        vTaskDelay(10 * i + 5);
        i++;
      }
      vTaskDelay(800);
      digitalWrite(2, LOW);
      Serial.println("NaiNai!");
      Serial.flush();

      esp_deep_sleep_start();
      Serial.println("This will never be printed because of DeepSleep.");
    }
    if (PrevBtn.onPressed()) {
      Serial.println("PauseBtn Onpressed, So do shift and ctrl. Shuffle playback!");
      vTaskDelay(1);
      music_mode_code = 3;
    }
    if (NextBtn.onPressed()) {
      Serial.println("PauseBtn Onpressed, So do shift and ctrl. ALL REVERSE!!");
      vTaskDelay(1);
      music_mode_code = 1;
      InNeed_Music_Location = tot_music_amount - 1;
      nextMusic(0);
    }
  }

  if (millis() - Operating_Time > 0x1145 /*14*/ && millis() > Operating_Time && audio.getAudioFileDuration() - audio.getAudioCurrentTime() < 2) {
    delayMicroseconds(100);
    switch (music_mode_code) {
      /*
      0 -> Repeat playlist
      1 -> Repeat playlist in reverse
      2 -> Repeat song
      3 -> Shuffle playback
      */
      case 0:
        nextMusic(1);
        digitalWrite(2, HIGH);
        vTaskDelay(50);
        digitalWrite(2, LOW);
        break;
      case 1:
        nextMusic(-1);
        digitalWrite(2, HIGH);
        vTaskDelay(150);
        digitalWrite(2, LOW);
        break;
      case 2:
        nextMusic(0);
        digitalWrite(2, HIGH);
        vTaskDelay(249);
        digitalWrite(2, LOW);
        break;
      case 3:
        nextMusic(random(-114, 514));
        digitalWrite(2, HIGH);
        vTaskDelay(350);
        digitalWrite(2, LOW);
        break;
      default: Serial.println("Invaild music_mode_code."); break;
    }
  }
}

// ----------------------------------------------------optional----------------------------------------------------
const char *cc = "Closing audio file";
const char *dd = "MP3 decode error -6 : INVALID_FRAMEHEADER";

void audio_info(const char *info) {
  if (info == dd) {
    audio.stopSong();
    nextMusic(1);
  } else if (info == cc) {
    Operating_Time = millis();
  }
  vTaskDelay(1);
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
}
