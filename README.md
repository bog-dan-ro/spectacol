# Spectacol

Spectacol (in Romanian it means **show**) is a [free](http://www.gnu.org/licenses/gpl-3.0.en.html) [ZX Spectrum](https://en.wikipedia.org/wiki/ZX_Spectrum) emulator based on famous [FUSE spectrum emulator](http://fuse-emulator.sourceforge.net/) but it uses its own UI made in QML.

Why QML? Well, first and foremost because I love Qt and also because I needed a framework which allows me to quickly target [lots of platforms](http://doc.qt.io/qt-5/supported-platforms.html) painlessly.

Why [FUSE](http://fuse-emulator.sourceforge.net/)? Because it has, by far, the reachest ZX features list.
**Spectacol**, currently, enables the following features:
 - Loads all ZX Spectrum files
 - Accurate 16K, 48K, 128K, +2, +2A and +3 emulation
 - Quick load & save snapshots. It is quite useful when you play games
 - Easily select the Joystick type
 - Poke finder, disassambler & **inline assambler**. Allows the user to crack the games and gain infinite lives quickly.
 - Last but not least, FUSE is very high-efficient, it needs only ~15% of CPU on a Nexus 10 to emulate your games

In addition to FUSE's features, Spectacol adds:
 - The UI is design with gamepad controls in mind. The user will be able to play all the games that support Joysticks using the gamepad
 - **Online search** allows the user to access **thousands** of ZX Spectrum apps & games! The search results are downloaded locally and they can be used to start the games offline
 - On screen numeric cursor (available only when a gamepad is present), allows the user to quickly start & control the games
 - On Screen full/simplified 48K Keyboard needed when the numeric cursor is not enough
 - On screen virtual Joystick (available only when no gamepad is present), allows the user to play the games on touch screen

Be aware that, Spectacol is in **BETA** stage, so, if you find bugs or you like to see new features, please add them to the [bugtracker](#issue)
