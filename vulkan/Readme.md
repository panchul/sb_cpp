# Vulkan Rendering Demo

This project is a small Vulkan + GLFW demo that renders a colorful animated 3D cube on screen and supports simple keyboard interaction. It is built as a lightweight example of the Vulkan rendering pipeline: instance setup, surface creation, swapchain, render pass, depth buffering, shader pipeline, command recording, and frame presentation.

## Included features

- Vulkan instance and validation layer setup
- GLFW window and Vulkan surface creation
- Swapchain and framebuffer setup
- Depth buffer and depth testing
- Vertex + fragment shader pipeline
- Animated colored 3D cube
- Optional triangulated sphere mesh (UV sphere or icosphere)
- On-screen Dear ImGui controls for mesh and animation parameters
- Keyboard interaction for rotation

## Controls

- `A` / `D`: rotate around the Y axis
- `W` / `S`: rotate around the X axis
- `Q` / `E`: rotate around the Z axis
- `R`: reset the rotation

An on-screen control panel is rendered with Dear ImGui and includes:

- FPS display
- FPS history graph
- Frame-time display (ms)
- Current triangle and vertex counts
- Toggle between cube and sphere
- Optional wireframe rendering mode (if supported by the active GPU)
- Sphere generator radio buttons (`UV Sphere` vs `Icosphere`)
- Triangle budget slider that auto-maps detail settings for UV sphere or icosphere
- Live geometry detail sliders (triangle count updates in real time)
- Auto-rotate toggle
- Rotation speed control
- Demo vs benchmark mode radio buttons
- Benchmark checkboxes for uncapped present mode, queue idle wait, and CPU FPS cap
- Benchmark instance multiplier for synthetic geometry load scaling

## Requirements

This project is designed to run on macOS, Linux, and Windows, but the installation steps differ by platform.

### macOS

Install the Vulkan SDK and GLFW:

```bash
brew install glfw molten-vk vulkan-headers vulkan-loader vulkan-validationlayers
```

The launch scripts automatically set the `VK_ICD_FILENAMES` and `VK_LAYER_PATH` values needed for MoltenVK.

### Linux

On Ubuntu/Debian, install the Vulkan development and runtime packages:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git libglfw3-dev libvulkan-dev glslang-tools vulkan-tools vulkan-validationlayers libdecor-0-plugin-1-gtk
```

If your distro exposes `libdecor-0-plugin-1` as a virtual package, install one concrete backend (`-gtk` or `-cairo`). If you use another distro, install the equivalent `glfw`, `vulkan`, `validation layer`, and `glslang` packages.

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

Note: during the first configure step, CMake fetches Dear ImGui from GitHub.

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

On Linux, this demo has been verified with both:

- NVIDIA ICD: `VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/nvidia_icd.json ./run_release.sh`
- Lavapipe software ICD fallback: `VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json ./run_release.sh`

The result looks like this:

![demo_run_mac.png](pics/demo_run_mac.png)

## Debugging the shaders

Shader problems are often the hardest part of a Vulkan app to diagnose, because GLSL errors show up at compile time and pipeline creation time, while runtime issues often appear as a blank screen or validation-layer warnings.

### 1. Check the validation layers

The program already enables Vulkan validation layers in debug builds. If the shader or pipeline setup is incorrect, the terminal usually prints a validation message describing the problem before the app crashes or renders incorrectly.

Pay attention to messages such as:

- shader validation errors
- invalid `VkPipeline` configuration
- mismatched vertex layout or attribute format
- incorrect descriptor or push constant usage
- depth/stencil attachment errors

### 2. Rebuild the SPIR-V shaders

This project compiles the GLSL sources into SPIR-V using `glslangValidator` during the build. If you edit `shaders/triangle.vert` or `shaders/triangle.frag`, rebuild the project:

```bash
./build_debug.sh
```

or on Windows:

```powershell
./build_debug.ps1
```

### 3. Validate the shader source itself

The GLSL source files are the most direct place to debug issues. If you suspect a transformation bug or incorrect matrix math, inspect:

- `shaders/triangle.vert`
- `shaders/triangle.frag`

Common issues include:

- wrong input attribute locations
- incorrect matrix math for perspective projection
- using the wrong `in` / `out` variable names
- writing to the wrong output location
- forgetting the `#version` declaration

### 4. Use validation output as your main debugging tool

When the app is built in debug mode, Vulkan validation layers are enabled by default. They often catch:

- incompatible formats
- missing depth attachment configuration
- invalid attribute descriptions
- pipeline state mistakes

If a validation message says the pipeline is invalid, the fix is usually in the pipeline creation code or the shader inputs/outputs, not in the render loop.

### 5. Keep the render loop simple while debugging

If the cube fails to appear, first simplify the problem:

- confirm the shader files compile
- confirm the render pass and framebuffer are valid
- confirm the vertex buffer contains valid data
- confirm `vkCmdDraw` is using the correct `vertexCount`
- confirm the pipeline uses the same vertex input layout as the CPU data

### 6. Useful commands

On macOS/Linux:

```bash
./build_debug.sh
./run_debug.sh
```

On Windows:

```powershell
./build_debug.ps1
./run_debug.ps1
```

If the program runs but appears blank, the most likely causes are pipeline state issues, a missing depth attachment, incorrect projection math, or a mismatch between the shader input layout and the CPU-side `Vertex` structure.

## How to inspect Vulkan validation output

Validation output is the first place to look when a Vulkan app fails. In debug builds, the app already registers a debug messenger and prints Vulkan warnings and errors to the terminal.

When something goes wrong, do not panic when the output seems long or confusing. A lot of Vulkan errors are actually very direct once you read them carefully. Typical issues include:

- a shader stage failing validation
- an image layout mismatch
- missing framebuffer attachment
- invalid command buffer usage
- queue submission or synchronization problems

A good workflow is:

1. run the app in debug mode
2. read the first validation warning or error printed to the console
3. check the corresponding Vulkan call or state in the C++ file
4. fix that piece and rebuild

The validation layer is especially helpful for:

- catching mismatched vertex input declarations
- spotting missing render pass attachments
- detecting invalid depth settings
- showing pipeline creation failures before runtime rendering happens

If you see an error that mentions `VkPipeline`, `VkRenderPass`, or a shader stage, that usually means the issue is in pipeline setup rather than the actual draw loop.

## Using a GPU debugger (RenderDoc / tooling)

A GPU debugger is extremely useful when the app launches but the output is wrong. Tools such as RenderDoc help you inspect:

- the draw calls being issued
- the pipeline state used for each draw
- the shaders being executed
- the render target contents
- the current framebuffer and attachments

### When to use a GPU debugger

Use a GPU debugger if:

- the app runs but the object is missing or distorted
- the cube looks clipped or disappears
- the colors are wrong
- the geometry is corrupted
- you suspect a depth or projection issue

### Typical beginner workflow

1. Launch the program under the debugger.
2. Capture a frame.
3. Inspect the pipeline state and render pass attachments.
4. Check whether the geometry is being submitted with the expected vertex format.
5. Confirm that the shader outputs and framebuffer match what the app expects.

While RenderDoc is not required for this sample, it is one of the best tools for understanding why a Vulkan scene is not rendering as expected.

## Simple shader debugging checklist

Use this checklist whenever shaders behave strangely:

- Did the shader compile successfully with `glslangValidator`?
- Are the input locations in the shader the same as in the C++ vertex format?
- Does the shader output match the fragment shader input variables?
- Is the projection matrix valid for the current aspect ratio?
- Is the depth attachment configured properly?
- Is the cube being drawn with the expected `vkCmdDraw` count?
- Are the vertex attribute descriptions aligned with the `Vertex` struct layout?

If one of those is wrong, the app may appear to be working but render nothing or render incorrectly.

## Common beginner mistakes in Vulkan

This sample is intentionally small, but Vulkan still has a lot of places where a small mistake causes a big headache. The most common beginner mistakes are:

### 1. Forgetting to match shader input/output layouts

The `Vertex` struct in C++ and the shader `layout(location = ...) in` declarations must match exactly.

If you change one side without changing the other, the app may:

- show nothing
- render garbage triangles
- produce a black screen
- trigger validation warnings about vertex attributes

### 2. Forgetting the depth buffer

A 3D cube needs depth testing to decide which faces are visible. If you do not create a depth image and attach it to the render pass, the cube may render with strange overdraw or missing faces.

### 3. Using the wrong buffer memory type

Vulkan requires a resource to be created in a buffer or image, then allocated from a compatible memory type. Beginners often forget that a GPU buffer and a CPU-visible staging buffer are different things.

### 4. Drawing the wrong number of vertices

If `vkCmdDraw` uses a vertex count different from the actual mesh size, the app may draw incomplete geometry or read past the buffer. This is a very common source of runtime corruption.

### 5. Mismatched render pass attachments

If the render pass expects a color attachment and depth attachment, the framebuffer must attach matching image views. If they do not line up, validation layers usually report the mismatch immediately.

### 6. Assuming the shader will fix bad CPU data

The vertex shader cannot magically repair invalid input data. If the data on the CPU side is wrong, the GPU will still render bad results.

### 7. Ignoring validation layers

Vulkan is strict. In debug mode, validation layers are one of the best tools for learning. If you ignore them, you may waste time chasing problems that the API is already clearly pointing out.

### 8. Confusing rotation, transform, and projection

A 3D object needs three different concepts:

- model transformation: rotate or move the object itself
- view transformation: place the camera in the scene
- projection: map 3D space to clip space for the screen

This sample does model rotation plus a simple perspective projection, which is enough for learning but not the full complexity of a production engine.

### 9. Forgetting to clear depth

Depth testing needs a known starting value. In this project, the render pass clears the depth buffer to `1.0`, which is the usual initial state for a depth buffer in Vulkan.

### 10. Assuming Vulkan is like OpenGL

Vulkan is more explicit and more verbose. You must set up most of the pipeline state yourself, define the render pass, create framebuffers, manage synchronization, and submit commands deliberately. That extra explicitness is exactly why it is powerful and why the validation layers matter so much.

## Vulkan alternatives and comparisons

Vulkan is not the only graphics API, but it is one of the most explicit and low-level ones. The main alternatives are OpenGL, OpenGL ES, Metal, and Direct3D. Each one trades ease of use against control and performance.

### OpenGL

OpenGL is the classic graphics API and is much easier to start with than Vulkan.

- Pros:
  - simpler to learn
  - fewer setup steps
  - easier to prototype with
  - widely used in older projects and teaching material

- Cons:
  - less explicit control over GPU work
  - can be harder to optimize well
  - older OpenGL features are less relevant in modern graphics work
  - driver behavior is often less predictable across vendors

- Compared to Vulkan:
  - OpenGL is more beginner-friendly
  - Vulkan gives more explicit control and usually better performance potential
  - Vulkan is more verbose and more complex to debug

### OpenGL ES

OpenGL ES is the mobile and embedded version of OpenGL. It is designed for phones, tablets, and other constrained devices.

- Pros:
  - good for mobile graphics
  - simpler than Vulkan for some embedded use cases

- Cons:
  - limited compared to desktop OpenGL feature sets
  - still more abstract than Vulkan
  - not the best choice for advanced desktop rendering

- Compared to Vulkan:
  - OpenGL ES is easier to use than Vulkan for mobile development
  - Vulkan is more powerful and lower-level, but harder to learn

### Metal

Metal is Apple’s native graphics API for macOS and iOS.

- Pros:
  - very optimized on Apple hardware
  - excellent integration with Apple platforms
  - often easier than Vulkan on macOS for native Apple development

- Cons:
  - not cross-platform
  - only available on Apple devices

- Compared to Vulkan:
  - Metal is a strong option on Apple hardware
  - Vulkan is portable across Windows, Linux, and macOS (with MoltenVK)
  - Vulkan is the better choice when you want one API across multiple operating systems

### Direct3D 12

Direct3D 12 is Microsoft’s low-level graphics API.

- Pros:
  - excellent on Windows
  - very low-level and high-performance
  - good for advanced PC graphics

- Cons:
  - Windows-only
  - requires more boilerplate and complexity than higher-level APIs

- Compared to Vulkan:
  - D3D12 and Vulkan are similar in philosophy: explicit, low-level, and performance-oriented
  - Vulkan is cross-platform; D3D12 is mainly Windows-focused

### Summary

If you are learning graphics programming for the first time:

- OpenGL is the easiest place to start
- Vulkan is the best choice when you want to learn the GPU pipeline in detail and work across multiple platforms
- Metal is strongest when targeting Apple-only apps
- Direct3D 12 is strong for Windows-specific work

### Which one should you choose?

Choose Vulkan if you want:

- explicit control over rendering
- cross-platform graphics work
- to learn how the GPU pipeline really works
- a modern low-level API with strong performance potential

Choose OpenGL if you want:

- a simpler learning curve
- faster experimentation
- a more approachable beginner path

Choose Metal or Direct3D 12 if you want:

- native API performance on Apple or Windows devices specifically

## More details

- Vulkan tutorial: https://vulkan-tutorial.com/
- GLFW docs: https://www.glfw.org/docs/latest/
- MoltenVK: https://github.com/KhronosGroup/MoltenVK
- Vulkan SDK / validation layer info: https://vulkan.lunarg.com/
- Vulkan for Windows/Linux: https://docs.vulkan.org/tutorial/latest/00_Introduction.html
- OpenGL overview: https://www.opengl.org/
- Metal: https://developer.apple.com/metal/
- Direct3D 12: https://learn.microsoft.com/en-us/windows/win32/direct3d12/
- Vulkan specification: https://docs.vulkan.org/spec/latest/chapters/pipelines.html
- Starting point for this sandbox: https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code
- RenderDoc: https://renderdoc.org/
