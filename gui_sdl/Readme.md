
# GUI examples

This folder contains a cross-platform C++ GUI sample using SDL2.
There is also SDL3, but SDL2 is more stable and platform independent at least for now.


## Why SDL2

- Runs on Linux and macOS with the same source code.
- Supports multiple windows.
- Uses hardware-accelerated rendering and vsync for smooth animation.

## Sample: multi-window animated bitmap rendering

Files:

- `CMakeLists.txt`
- `multiwindow_anim.cpp`

What it demonstrates:

- Two independent GUI windows.
- Per-window animation loops.
- Fast per-frame bitmap updates via a streaming texture.
- Handling resize and close events per window.

## Build

### 1) Install SDL2

macOS (Homebrew):

```bash
brew install sdl2
```

Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y libsdl2-dev
```

### 2) Configure and compile

```bash
cmake -S . -B build
cmake --build build -j
```

### 3) Run

```bash
./build/gui_multiwindow_anim
```

Press `Esc` to quit. Closing one window keeps the other running.

## SDL2 links and resources

- Official SDL website: https://www.libsdl.org/
- SDL2 API documentation (Wiki): https://wiki.libsdl.org/SDL2
- SDL source and releases (GitHub): https://github.com/libsdl-org/SDL
- SDL forums and community discussions: https://discourse.libsdl.org/
- SDL examples repository: https://github.com/libsdl-org/SDL/tree/main/examples
- Lazy Foo SDL2 tutorials (practical beginner-to-intermediate): https://lazyfoo.net/tutorials/SDL/

