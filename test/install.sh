#!/bin/bash

# 下载ArduinoIDE Linux64版本
echo -e "\e[93mDownload ArduinoIDE...\e[0m"
appveyor DownloadFile $ARDUINO_URL -FileName $BUILD_HOME/arduino-linux64.tar.xz
# 提取ArduinoIDE文件
echo -e "\e[93mExtract ArduinoIDE...\e[0m"
tar xf $BUILD_HOME/arduino-linux64.tar.xz -C $BUILD_HOME
# 下载目标板支持包
if [ -n "$LIB_URL" ]; then
    echo -e "\e[93mDownload $TARGET...\e[0m"
    appveyor DownloadFile $LIB_URL -FileName $BUILD_HOME/$TARGET.zip
    # 提取板支持包
    echo -e "\e[93mExtract $TARGET...\e[0m"
    unzip $BUILD_HOME/$TARGET.zip -d $BUILD_HOME > /dev/null
    echo -e "\e[93mInstall $TARGET...\e[0m"
    mkdir -p $HWLIB_PATH
    # 执行指定平台安装脚本
    echo -e "\e[93mRun the $TARGET install script"
    if [ "$TARGET" == "ESP8266" ]; then
        # 从库中复制工具链索引文件
        cp -R $BUILD_HOME/$LIB_VER $HWLIB_PATH/esp8266
        cp -R $TARGET_LIB/test/package $HWLIB_PATH/esp8266/package
        cd $HWLIB_PATH/esp8266/tools
        python get.py
    fi
else
    echo -e "\e[93mNo package set, skip.\e[0m"
fi
echo -e "\e[93mInstall Done.\e[0m"
