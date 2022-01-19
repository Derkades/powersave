#!/bin/bash
set -ex
adb connect 10.42.0.148:5555
adb root
adb shell