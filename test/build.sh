#!/bin/bash
# 生成构建配置
echo -e "\e[93mGenerate esp8266.build.option.json\e[0m"
cat <<EOF > $BUILD_HOME/esp8266.build.option.json
{
  "additionalFiles": "",
  "builtInLibrariesFolders": "$ARDUINO_BUILDIN_LIB",
  "customBuildProperties": "build.warn_data_percentage=75,runtime.tools.xtensa-lx106-elf-gcc.path=$ESP8266_PATH/esp8266/tools/xtensa-lx106-elf,runtime.tools.xtensa-lx106-elf-gcc-2.5.0-3-20ed2b9.path=$ESP8266_PATH/esp8266/tools/xtensa-lx106-elf,runtime.tools.mkspiffs.path=$ESP8266_PATH/esp8266/tools/mkspiffs,runtime.tools.mkspiffs-2.5.0-3-20ed2b9.path=$ESP8266_PATH/esp8266/tools/mkspiffs,runtime.tools.python.path=$ESP8266_PATH/esp8266/tools/python,runtime.tools.python-3.7.2-post1.path=$ESP8266_PATH/esp8266/tools/python",
  "fqbn": "esp8266com:esp8266:wifiduino:xtal=80,vt=flash,exception=disabled,ssl=all,eesz=4M,ip=lm2f,dbg=Disabled,lvl=None____,wipe=none,baud=921600",
  "hardwareFolders": "$ARDUINO_PATH/hardware,$BUILD_HOME/Arduino/hardware",
  "otherLibrariesFolders": "$BUILD_HOME/Arduino/libraries",
  "toolsFolders": "$ARDUINO_PATH/tools-builder,$ARDUINO_PATH/hardware/tools/avr,$ESP8266_PATH/esp8266/tools"
}
EOF
echo -e "\e[93mDump esp8266.build.option.json:\e[92m"
cat $BUILD_HOME/esp8266.build.option.json
echo -e "\e[93mDone.\e[0m"

echo -e "\e[42mStart Building.\e[0m"
# 遍历ino文件
for sketch in `find $TARGET_LIB -name '*.ino'`
    do
        echo -e "\e[32mBuilding:$sketch\e[0m"
        $ARDUINO_PATH/arduino-builder -build-options-file $BUILD_HOME/esp8266.build.option.json $sketch
        echo -e "\e[32mDone.\e[0m"
    done
