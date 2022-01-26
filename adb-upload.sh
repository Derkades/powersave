#!/bin/bash
set -ex
adb connect 10.42.0.148:5555
adb root
arm-linux-androideabi-clang++ -static-libstdc++ Governor.cpp -o Governor
adb push Governor /data/local/workingdir/
adb exec-out sh -c 'echo 0 > /sys/class/fan/mode'
adb exec-out sh -c 'echo 1 > /sys/class/fan/enable'
adb exec-out sh -c 'echo 4 > /sys/class/fan/level'
adb exec-out sh -c 'cd /data/local/workingdir && chmod +x ./Governor && ./Governor graph_alexnet_all_pipe_sync 3 15 1000'