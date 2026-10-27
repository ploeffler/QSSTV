# QSSTV
QSSTV is a program for receiving and transmitting SSTV and HAMDRM (sometimes called DSSTV). It is compatible with most of MMSSTV and EasyPal

## QSSTV 9.6.0 development build

MMSSTV template import is integrated in this version. In the template editor, choose **File → Import MMSSTV template...** to preview and convert an `.mtm` file into an editable QSSTV `.templ` file, or **File → Batch import MMSSTV templates...** to convert multiple files into a selected directory. Existing files are preserved. See the [supported objects and limitations](docs/development/mmsstv-template-import-9.6.md) and [9.6 release notes](documentation/RELEASE_NOTES_9.6.md).

Linux configuration files follow the full application version (`~/.config/qsstv_9.6.0.conf` for this build, honoring `XDG_CONFIG_HOME`). Older files are upgraded automatically without changing the originals. See [configuration migration](documentation/PACKAGING.md).

This is a development build, not an official release. KG-STV has an experimental native RX/TX, standalone ASCII text and manual BSR path. Original KG-STV interoperability and physical audio/CAT/PTT testing remain open; see the [KG-STV validation notes](tests/kgstv-tests.md).

## Installation

### Dependencies 

For apt based distros you can install dependencies as follows:

```
apt install pkg-config g++ libfftw3-dev qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools libqt5svg5-dev libhamlib++-dev libasound2-dev libpulse-dev libopenjp2-7 libopenjp2-7-dev libv4l-dev build-essential
```

### macOS Dependencies

For macOS users, you can install dependencies using Homebrew:

```bash
brew install qt@5 fftw hamlib openjpeg pulseaudio qwt pkg-config
```

**Note:** You must have PulseAudio running for sound to work:
```bash
brew services start pulseaudio
```

### Compile and Install
	mkdir src/build
	cd src/build
	# For Linux
	qmake ..
	# For macOS
	/opt/homebrew/opt/qt@5/bin/qmake ..
	
	make -j2
	sudo make install

Note: make -j2, 2 is the number of cores to be used for parallel compiling. If you have more cores, use a higher number.

### Debug Compile
If you have problems compiling the software, please give as much information as possible but at least:
* Linux Distribution and Version (e.g. Ubuntu 18.04)
* QT Version (e.g. Qt 5.4.1)
* Screen dump of the compile process showing the error

If you want to be able to debug the program, the simplest way is to install QtCreator and from within QtCreator open a new project and point to the qsstv.pro file. Note: you will need to install doxygen and libqwt

`sudo apt-get install doxygen libqwt-qt5-dev`

You can also run qmake with the following attributes:

`qmake CONFIG+=debug`

and use an external debugger (such as gdb)

## Experimental cross-platform builds

QSSTV now has experimental GitHub Actions builds for **Windows x86_64**, **Linux x86_64 AppImage** and **Linux amd64 DEB**. All three builds succeeded on `main` at commit `c7dd2dce`.

These builds are **not yet validated on real radio hardware** and must not be treated as stable releases. See the [experimental release notes](documentation/RELEASE_NOTES_EXPERIMENTAL.md) for download links and the testing checklist.

Windows workflows now produce a **single portable EXE** that starts without installation, bundles Qt/runtime dependencies and cleans its temporary runtime after exit. The new packaging still requires a Windows CI run and clean-machine validation.

Further documentation:
- [Packaging and installation](documentation/PACKAGING.md)
- [Windows port and validation](documentation/WINDOWS_PORT.md)
- [Development roadmap](documentation/ROADMAP.md)
