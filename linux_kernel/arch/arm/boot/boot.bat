adb shell mount -o remount / /
adb push uImage /
adb shell sync
adb shell reboot
