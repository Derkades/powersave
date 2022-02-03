#!/bin/bash
set -ex
arm-linux-androideabi-clang++ -static-libstdc++ Governor.cpp -o Governor
adb push Governor /data/local/Working_dir/
adb exec-out sh -c 'echo 0 > /sys/class/fan/mode'
adb exec-out sh -c 'echo 1 > /sys/class/fan/enable'
adb exec-out sh -c 'echo 4 > /sys/class/fan/level'
adb exec-out sh -c 'cd /data/local/Working_dir && chmod +x ./Governor && ./Governor graph_alexnet_all_pipe_sync'
