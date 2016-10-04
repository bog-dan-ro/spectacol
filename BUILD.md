# Build from source code

Currently Spectacol can be build only from a unix host (linux, osx, freebsd, etc) and only for Android & desktop. Might work from/on other O.S. but I didn't test it

The next steps are for a Linux host targeting desktop & android builds:

Requirements:
- QtCreator
- Qt 5.7 or later.
- Android NDK & SDK needed to build for Android devices
You can download the Qt SDK which contains Qt for desktop & for Android + QtCreator IDE from [here](https://www.qt.io/download-open-source/).
[Here](https://www.kdab.com/qt-on-android-episode-2/) you can find more info about how to setup dev env for Android.

Step by step build:
- clone the code:

```
$ git clone https://github.com/bog-dan-ro/spectacol.git
```

- run the build scripts:
```
$ cd spectacol/build
% ./android.sh -n path/to/android/ndk
```
if you target the Android devices, or
```
$ cd spectacol/build
% ./x86.sh
```
if you target the desktop.
Every time when you switch the build type you must clean & re-run the appropriate script.

- open spectacol/fuse/ui/qml/qml.pro into QtCreator, choose the right kit.
