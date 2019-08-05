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
    echo -e "\e[93mRun the $TARGET install script\e[0m"
    if [ "$TARGET" == "ESP8266" ]; then
        # 从库中复制工具链索引文件
        cp -R $BUILD_HOME/$LIB_VER $HWLIB_PATH/esp8266
        cp -R $TARGET_LIB/test/package $HWLIB_PATH/esp8266/package
        cd $HWLIB_PATH/esp8266/tools
        python get.py
    elif [ "$TARGET" == "ESP32" ]; then
        # 从库中复制工具链索引文件
        mkdir -p $HWLIB_PATH
        cp -R $BUILD_HOME/$LIB_VER $HWLIB_PATH/esp32
        cd $HWLIB_PATH/esp32/tools
        python3 get.py
        echo -e "\e[93mInstall PySerial.\e[0m"
        sudo pip install pyserial
    fi
else
    echo -e "\e[93mNo package set, skip.\e[0m"
fi
# 安装第三方库
if [ -n "$OTHER_LIB" ]; then
    # 解析字符串为数组
    readarray -td, libs <<<"$OTHER_LIB,"
    unset 'libs[-1]'

    for lib in "${libs[@]}"
    do
        echo -e "\e[93mCloning $lib...\e[0m"
        cd $BUILD_HOME/Arduino/libraries
        # 克隆最近的release
        git clone --depth=1 $lib
    done
fi

echo -e "\e[93mInstall Done.\e[0m"
