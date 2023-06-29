# ESP32-Arduino_AnyAudioPlayer
A powerful ANY audio player based on ESP32 &amp; Arduino. 
# ESP32 Arduino 任意音频播放器 (中文版本README在下面)
一个强大的基于ESP32&amp;Arduino的播放器。
---
## To I2S Users:
You may have to change 'audio.setVolume(HERE);' to change the volume. volume is 0~21.

## Downloading .h:
### SdFat.h at https://github.com/greiman/sdfat UNDER MIT LICENSE!
### Audio.h at https://github.com/schreibfaul1/ESP32-audioI2S UNDER GPL 3.0 LICENSE!
### Warning, to be compatible with sdfat, you should change Audio.cpp and Audio.h which is in ESP32-audioI2S.
### After downloading, please use the .cpp(s) and .h(s) at http://alexander-zone.wikidot.com/local--files/code/Audio_cpp_h_changes.zip to replace them.

## Wiring:
### SD CARD
SD PIN | ESP32-WROOM PIN
8      | NC   
7      | 19   
6      | GND   
5      | 18   
4      | VDD = 3.3V   
3      | GND   
2      | 23     
1      | 5   
9      | NC   

### Button:
16-Button-GND previous   
17-Button-GND next  
14-Button-GND pauseresume   
4-Button-GND 10x next  

### AMP: (FOR EXAMPLE, PAM8403)  
AMP Pin | Connect 2......  
GND     | GND  
VCC     | VCC(5V is better than 3V3)   
L       | ESP32-WROOM GPIO26   
R       | ESP32-WROOM GPIO27   
ROUT    | connect to Ur external Speaker(4ohm/8ohm, R-CH) with an 470uF ≥10V Polarized Capacitor   
LOUT    | connect to Ur external Speaker(4ohm/8ohm, L-CH) with an 470uF ≥10V Polarized Capacitor   
R       | connect to Ur external Speaker(R-CH, cathode)  
L       | connect to Ur external Speaker(L-CH, cathode)   

### Earphone：

![图片](https://user-images.githubusercontent.com/120780632/222940321-a78a1a4c-6252-4ccd-bc01-8c6738b2d35b.png)


# 下面是中文版本   
---
## 对I2S用户:    
你可能需要修改'audio.setVolume(这里)'来修改音量。音量范围是0~21.   

## 下载库文件:
### SdFat.h 在 https://github.com/greiman/sdfat    
# SdFat 使用 MIT 许可证
### Audio.h at https://github.com/schreibfaul1/ESP32-audioI2S
# Audio 使用 GPL 3.0 许可证
### 警告：Audio为了兼容SdFat，需要在下载的ESP32-AudioI2s中用下面的文件替换Audio.cpp与Audio.h
### http://alexander-zone.wikidot.com/local--files/code/Audio_cpp_h_changes.zip
# 修改后的文件依然使用GPL 3.0许可证！！！

## 接线：
### SD卡   
SD 引脚 | ESP32-WROOM 引脚     
8      | NC    
7      | 19   
6      | GND(电源负极)   
5      | 18   
4      | VDD = 3.3V   
3      | GND(电源负极)   
2      | 23   
1      | 5   
9      | NC   

### 按钮
16-Button-GND(电源负极) previous 上一首   
17-Button-GND(电源负极) next 下一首   
14-Button-GND(电源负极) pauseresume 暂停恢复   
4-Button-GND(电源负极) 10x next 向下十首歌   

### 功放: (例如, PAM8403)   
功放引脚  | 连接到......   
GND      | GND(电源负极)  
VCC      | VCC(尽量用5V，尽量不用3V3)   
L        | ESP32-WROOM GPIO26   
R        | ESP32-WROOM GPIO27   
ROUT     | 通过470uF≥10V电解电容器连接到功放（≥4ohm）   
LOUT     | 通过470uF≥10V电解电容器连接到功放（≥4ohm）   
R        | 连接到你的外置扬声器（右声道，负极）   
L        | 连接到你的外置扬声器（左声道，负极）   

### 耳机：
耳机左声道可以直接通过一个可调电阻（经过测试，最好是103（即10KΩ）电位器）和一个电容（47uF基本可以，470uF就很不错了，没试过更大的电容）连接到ESP32的GPIO26上，    
右声道也是如此，不过连接到GPIO27上。   
        
          
![图片](https://user-images.githubusercontent.com/120780632/222940313-c74dde40-51ec-44df-ad44-3c0f08fde834.png)





