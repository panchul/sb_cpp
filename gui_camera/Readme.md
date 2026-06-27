# Camera GUI example

This sample shows a cross-platform camera viewer built with SDL2 for the windows and controls, and OpenCV for camera capture.

## What it does

- Opens a live video window for the attached camera.
- Opens a separate control window with clickable buttons.
- Lets you switch cameras when multiple devices are available.
- Lets you manipulate live frames with toggles such as grayscale, mirror, flip, blur, and edge detection.

## Dependencies

macOS (Homebrew):

```bash
brew install sdl2 sdl2_ttf opencv
```

Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y libsdl2-dev libsdl2-ttf-dev libopencv-dev
```

If CMake cannot find OpenCV on your machine, set `OpenCV_DIR` to the OpenCV package directory provided by your install.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
./build/gui_camera_control
```

Use the control window to choose a camera and toggle frame effects. Press `Esc` in either window to quit.
