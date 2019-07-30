#!/bin/bash
# 下载ArduinoIDE Linux64版本
echo -e "\e[93mDownload ArduinoIDE...\e[0m"
appveyor DownloadFile $ARDUINO_URL -FileName $BUILD_HOME/arduino-linux64.tar.xz
# 下载ESP8266支持包
echo -e "\e[93mDownload ESP8266 Package...\e[0m"
appveyor DownloadFile $ESP8266_URL -FileName $BUILD_HOME/esp8266.zip
# 解包
echo -e "\e[93mUnpack ArduinoIDE...\e[0m"
tar xf $BUILD_HOME/arduino-linux64.tar.xz -C $BUILD_HOME
# 解包
echo -e "\e[93mUnpack ESP8266 Package...\e[0m"
unzip $BUILD_HOME/esp8266.zip -d $BUILD_HOME > /dev/null
echo -e "\e[93mDone.\e[0m"
# 安装ESP8266支持包
echo -e "\e[93mInstall ESP8266 Library...\e[0m"
mkdir -p $ESP8266_PATH
# 复制目录
cp -R $BUILD_HOME/$ESP8266_VER $ESP8266_PATH/esp8266
cp -R $TARGET_LIB/test/package $ESP8266_PATH/esp8266/package
cd $ESP8266_PATH/esp8266/tools
python get.py
echo -e "\e[93mDone.\e[0m"
ls -la $ESP8266_PATH/esp8266/tools/python