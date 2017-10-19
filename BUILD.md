# Build from source code

Currently Spectacol can be build only from a unix host (linux, osx, freebsd, etc) and only for Android & desktop. Might work from/on other O.S. but I didn't test it

The next steps are for a Linux host targeting desktop & android builds:

Requirements:
- QtCreator
- Qt 5.8 or later.
- Android NDK & SDK needed to build for Android devices
You can download the Qt SDK which contains Qt for desktop & for Android + QtCreator IDE from [here](https://www.qt.io/download-open-source/).
[Here](https://www.kdab.com/qt-on-android-episode-2/) you can find more info about how to setup dev env for Android.

Step by step build:
- clone the code:

```
$ git clone https://github.com/bog-dan-ro/spectacol.git
$ cd spectacol
$ git submodule update --init --recursive
```

- open spectacol.pro into QtCreator, choose the right kit.
