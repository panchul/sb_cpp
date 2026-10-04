
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

On macOS, install the dependencies needed for the Vulkan loader and MoltenVK runtime:

```bash
brew install glfw molten-vk vulkan-headers vulkan-loader vulkan-validationlayers
```

## Build

From this folder, you can build either configuration explicitly:

```bash
./build_debug.sh
./build_release.sh
```

The convenience wrappers also work:

```bash
./build.sh debug
./build.sh release
```

Each script creates a separate local build folder:

- `build/debug`
- `build/release`

## Run

```bash
./run_debug.sh
./run_release.sh
```

You can also use the convenience wrappers:

```bash
./run.sh debug
./run.sh release
```

The launch scripts set the Vulkan environment variables needed for MoltenVK on macOS:

- `VK_ICD_FILENAMES`: points to the MoltenVK ICD
- `VK_LAYER_PATH`: loads the validation layer from the Vulkan installation

The result looks like this:

![demo_run_mac.png](pics/demo_run_mac.png)

## More details

- Vulkan tutorial: https://vulkan-tutorial.com/
- GLFW docs: https://www.glfw.org/docs/latest/
- MoltenVK: https://github.com/KhronosGroup/MoltenVK
- Vulkan SDK / validation layer info: https://vulkan.lunarg.com/

## Original code reference

- https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code


[def]: !pic