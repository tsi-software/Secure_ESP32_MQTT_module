# Secure_ESP32_MQTT_module
*Copyright (c) 2019 Warren Taylor.*

# ESP32 Development Environment Setup
<https://docs.espressif.com/projects/esp-idf/en/stable/get-started/>

## Debian Toolchain
```bash
sudo apt-get install gcc git wget make libncurses-dev flex bison gperf python python-serial

mkdir -p ~/bin/esp
cd ~/bin/esp
tar -xzf ~/Downloads/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz

export PATH="$PATH:$HOME/bin/esp/xtensa-esp32-elf/bin"

sudo usermod -a -G dialout $USER

ls /dev/tty*

screen /dev/ttyUSB0 115200
ctrl-A k
```

## ESP IDF
```bash
cd ~/bin/esp
git clone -b v3.1.2 --recursive https://github.com/espressif/esp-idf.git

export IDF_PATH=$HOME/bin/esp/esp-idf
```
**NOTE:** Do NOT use ~ when setting environment variables. Use $HOME instead.

## Hello World
```bash
cd ~/bin/esp
cp -r $IDF_PATH/examples/get-started/hello_world .

cd hello_world
make menuconfig
```

### Updating IDF to a Release Branch
```bash
cd $IDF_PATH
git fetch
git checkout release/v3.2
git pull
git submodule update --init --recursive
```
