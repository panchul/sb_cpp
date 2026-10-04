# Vulkan Rendering Demo

This project is a small Vulkan + GLFW demo that renders a colorful animated 3D cube on screen and supports simple keyboard interaction. It is built as a lightweight example of the Vulkan rendering pipeline: instance setup, surface creation, swapchain, render pass, depth buffering, shader pipeline, command recording, and frame presentation.

## Included features

- Vulkan instance and validation layer setup
- GLFW window and Vulkan surface creation
- Swapchain and framebuffer setup
- Depth buffer and depth testing
- Vertex + fragment shader pipeline
- Animated colored 3D cube
- Keyboard interaction for rotation

## Controls

- `A` / `D`: rotate around the Y axis
- `W` / `S`: rotate around the X axis
- `Q` / `E`: rotate around the Z axis
- `R`: reset the rotation

## Requirements

This project is designed to run on macOS, Linux, and Windows, but the installation steps differ by platform.

### macOS

Install the Vulkan SDK and GLFW:

```bash
brew install glfw molten-vk vulkan-headers vulkan-loader vulkan-validationlayers
```

The launch scripts automatically set the `VK_ICD_FILENAMES` and `VK_LAYER_PATH` values needed for MoltenVK.

### Linux

On Ubuntu/Debian, install the usual Vulkan development packages:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git libglfw3-dev libvulkan-dev glslang-tools
```

If you use another distro, install the equivalent `glfw`, `vulkan`, and `glslang` packages.

### Windows

Install:

- CMake
- A Vulkan SDK (such as LunarG's Vulkan SDK)
- GLFW
- A C++ compiler (MSVC or MinGW)

The easiest approach is usually to use a package manager like `vcpkg` or the official Vulkan SDK installer and then point CMake to the installed dependencies.

## Build

From this folder, you can build either configuration explicitly.

### macOS / Linux (bash)

```bash
./build_debug.sh
./build_release.sh
```

Or use the wrappers:

```bash
./build.sh debug
./build.sh release
```

### Windows (PowerShell)

```powershell
./build_debug.ps1
./build_release.ps1
```

Or:

```powershell
./build.ps1 debug
./build.ps1 release
```

Each build creates a separate output directory:

- `build/debug`
- `build/release`

## Run

### macOS / Linux (bash)

```bash
./run_debug.sh
./run_release.sh
```

Or:

```bash
./run.sh debug
./run.sh release
```

### Windows (PowerShell)

```powershell
./run_debug.ps1
./run_release.ps1
```

Or:

```powershell
./run.ps1 debug
./run.ps1 release
```

The macOS launch scripts set the Vulkan environment variables needed for MoltenVK:

- `VK_ICD_FILENAMES`: points to the MoltenVK ICD
- `VK_LAYER_PATH`: loads the validation layer from the Vulkan installation

Linux and Windows usually do not require extra environment variables beyond the installed runtime and loader configuration.

The result looks like this:

![demo_run_mac.png](pics/demo_run_mac.png)

## More details

- Vulkan tutorial: https://vulkan-tutorial.com/
- GLFW docs: https://www.glfw.org/docs/latest/
- MoltenVK: https://github.com/KhronosGroup/MoltenVK
- Vulkan SDK / validation layer info: https://vulkan.lunarg.com/
- Vulkan for Windows/Linux: https://docs.vulkan.org/tutorial/latest/00_Introduction.html

## Original code reference

- https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code

[def]: !pic