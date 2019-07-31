#!/bin/bash

# 加,
if [ -n "$TOOL_PATH" ]; then
    export TOOL_PATH=",$TOOL_PATH"
fi

# 生成构建配置
echo -e "\e[93mGenerate build.option.json\e[0m"
cat <<EOF > $BUILD_HOME/build.option.json
{
  "additionalFiles": "",
  "builtInLibrariesFolders": "$ARDUINO_PATH/libraries",
  "customBuildProperties": "$BUILD_CONF",
  "fqbn": "$BOARD_CONF",
  "hardwareFolders": "$ARDUINO_PATH/hardware,$BUILD_HOME/Arduino/hardware",
  "otherLibrariesFolders": "$BUILD_HOME/Arduino/libraries",
  "toolsFolders": "$ARDUINO_PATH/tools-builder,$ARDUINO_PATH/hardware/tools/avr$TOOL_PATH"
}
EOF
cat $BUILD_HOME/build.option.json
echo -e "\e[93mGenerated complete.\e[0m"

echo -e "\e[42mStart compiling...\e[0m"
# 遍历ino文件
for sketch in `find $TARGET_LIB -name '*.ino'`
    do
        echo -e "\e[32mCompiling:$sketch\e[0m"
        $ARDUINO_PATH/arduino-builder -build-cache $BUILD_HOME/build_cache -build-options-file $BUILD_HOME/build.option.json $sketch
        ret=$?
        if [ $ret -ne 0 ]; then
            echo -e "\e[101mCompile failed:$ret:$sketch\e[0m"
        fi
    done
echo -e "\e[32mBuild completed\e[0m"
