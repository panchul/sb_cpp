#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <thread>
#include <unordered_map>
#include <stdexcept>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// Vulkan exposes many functions through function pointers because the Vulkan
// loader can optionally support different extensions. This wrapper resolves the
// debug messenger creation function at runtime and safely handles systems where
// the extension is not available.
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// This is the matching destroy function for the debug message callback. Vulkan
// expects us to clean up debug resources explicitly when the app shuts down.
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

// A vertex describes a single point in our mesh. Each vertex stores its local
// position in 3D space and the color that should be used when shading it.
struct Vertex {
    float pos[3];
    float color[3];
};

// Push constants are a small block of uniform-like data sent directly to the
// shader for each draw call. They are used here for rotation and timing values,
// which change every frame but do not require a full descriptor set/buffer.
struct PushConstants {
    float rotationX;
    float rotationY;
    float rotationZ;
    float aspect;
    float time;
};

// A GPU may expose multiple queue families. We need at least one queue capable
// of graphics work and another capable of presenting to the surface.
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class VulkanDemo {
public:
    // The application's top-level lifecycle: create the window, initialize Vulkan,
    // run the rendering loop, and then destroy all objects in reverse order.
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    enum class SphereGenerator {
        UV = 0,
        Icosphere = 1
    };
    enum class RunMode {
        Demo = 0,
        Benchmark = 1
    };

    // GLFW window used to create a Vulkan surface and receive input.
    GLFWwindow* window = nullptr;

    // Vulkan handles are opaque integer-like values that identify objects such as
    // the instance, the logical device, the swapchain, and the pipeline.
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    // The selected physical GPU and the logical device created from it.
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    // Swapchain and image objects are how Vulkan presents rendered frames to the
    // window. The app renders into one of these images and then presents it.
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapChainExtent{};
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // Render pass and pipeline are the GPU-side configuration that tells Vulkan
    // how to transform vertices and write the final image.
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    VkPipeline wireframePipeline = VK_NULL_HANDLE;
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    // Command buffers store the GPU commands that will be submitted in each frame.
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    // The mesh is uploaded to a GPU buffer. The application tracks the buffer,
    // its memory, and how many vertices are in the draw call.
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;

    // Synchronization objects prevent the CPU and GPU from racing each other.
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;
    VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;

    // Per-frame animation and UI state.
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    float time = 0.0f;
    float rotationSpeedDegreesPerSecond = 90.0f;
    float autoRotationY = 0.0f;
    bool autoRotate = true;
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
    float fpsAccumulator = 0.0f;
    uint32_t fpsFrameCount = 0;
    std::chrono::steady_clock::time_point lastFrameTime{};
    bool hasLastFrameTime = false;

    bool useSphere = false;
    SphereGenerator sphereGenerator = SphereGenerator::UV;
    int uvLatitudeSegments = 18;
    int uvLongitudeSegments = 36;
    int icoSubdivisions = 2;
    int triangleBudget = 1224;
    RunMode runMode = RunMode::Demo;
    bool preferImmediatePresentMode = false;
    bool pendingSwapchainRecreate = false;
    bool immediatePresentSupported = false;
    VkPresentModeKHR activePresentMode = VK_PRESENT_MODE_FIFO_KHR;
    bool benchmarkCpuCapEnabled = false;
    float benchmarkCpuCapFps = 120.0f;
    bool waitForPresentQueueIdle = true;
    uint32_t drawInstanceCount = 1;
    uint32_t triangleCount = 12;
    bool meshDirty = true;
    bool imguiInitialized = false;
    bool supportsWireframe = false;
    bool wireframeEnabled = false;
    static constexpr int fpsHistorySize = 120;
    std::array<float, fpsHistorySize> fpsHistory{};
    int fpsHistoryOffset = 0;

    // On macOS with MoltenVK, some portability extensions may be required to
    // enumerate and use the GPU correctly. We keep the extension names in one
    // place so the instance and device setup stay readable.
    static constexpr const char* portabilitySubsetExtensionName = "VK_KHR_portability_subset";
    static constexpr const char* portabilityEnumerationExtensionName = "VK_KHR_portability_enumeration";

    static constexpr const char* swapchainExtensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    void initWindow() {
        // GLFW creates the OS window and provides the surface we will hand to
        // Vulkan. A Vulkan app still needs a windowing library to create the
        // actual visible framebuffer.
        if (!glfwInit()) {
            throw std::runtime_error("failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Demo", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("failed to create GLFW window");
        }
    }

    void initVulkan() {
        // Vulkan is a low-level API: before drawing anything, we must create the
        // Vulkan instance, select a compatible GPU, create a swapchain for the
        // window, and prepare the rendering pipeline. These calls set up the
        // render context the application will use for every frame.
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createDepthResources();
        createFramebuffers();
        createCommandPool();
        allocateCommandBuffers();
        createVertexBuffer();
        initImGui();
        createSyncObjects();
    }

    void mainLoop() {
        // The application loop is where the game/demo logic and rendering are
        // intertwined: poll events, update transforms, render one frame, repeat
        // until the user closes the window.
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            updateScene();
            drawFrame();
        }

        vkDeviceWaitIdle(device);
    }

    void cleanup() {
        // Vulkan resources are explicit and must be destroyed in reverse order of
        // creation. This avoids dangling handles and makes cleanup predictable.
        shutdownImGui();

        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        vkDestroyFence(device, inFlightFence, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        cleanupSwapchainDependentResources();
        vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyPipeline(device, wireframePipeline, nullptr);
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void updateScene() {
        // The application updates its simulation state once per frame.
        // Here, keyboard input changes the object's Euler rotation so the user can
        // see the 3D object from different angles. This is the CPU-side logic
        // that feeds values into the GPU shaders via push constants.
        const auto now = std::chrono::steady_clock::now();
        float deltaSeconds = 0.0f;
        if (hasLastFrameTime) {
            deltaSeconds = std::chrono::duration<float>(now - lastFrameTime).count();
        }
        lastFrameTime = now;
        hasLastFrameTime = true;
        frameTimeMs = deltaSeconds * 1000.0f;
        if (deltaSeconds > 0.0f) {
            fpsHistory[static_cast<size_t>(fpsHistoryOffset)] = 1.0f / deltaSeconds;
            fpsHistoryOffset = (fpsHistoryOffset + 1) % fpsHistorySize;
        }

        fpsAccumulator += deltaSeconds;
        ++fpsFrameCount;
        if (fpsAccumulator >= 0.25f) {
            fps = static_cast<float>(fpsFrameCount) / fpsAccumulator;
            fpsAccumulator = 0.0f;
            fpsFrameCount = 0;
        }

        const float rotationSpeed = rotationSpeedDegreesPerSecond * (3.14159265358979323846f / 180.0f);
        if (autoRotate) {
            autoRotationY += rotationSpeed * deltaSeconds;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            rotationY -= rotationSpeed * deltaSeconds;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            rotationY += rotationSpeed * deltaSeconds;
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            rotationX -= rotationSpeed * deltaSeconds;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            rotationX += rotationSpeed * deltaSeconds;
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            rotationZ -= rotationSpeed * deltaSeconds;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            rotationZ += rotationSpeed * deltaSeconds;
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            rotationX = 0.0f;
            rotationY = 0.0f;
            rotationZ = 0.0f;
            autoRotationY = 0.0f;
        }

        time += deltaSeconds;
    }

    void createInstance() {
        // Vulkan is initialized through a VkInstance, which is the global entry
        // point into the API. This object is required before any device, surface,
        // or shader-related setup can happen.
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested but not available");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Demo";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        if (checkInstanceExtensionSupport(portabilityEnumerationExtensionName)) {
            createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create Vulkan instance");
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        // Validation layers are optional but invaluable during development. This
        // configures which message severities and message categories are reported,
        // so the app can print warnings and errors during setup and rendering.
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger() {
        // Once the instance exists, we can attach a Vulkan debug messenger. This
        // allows the validation layers to print warnings when we do something
        // incorrect, which is extremely helpful while learning Vulkan.
        if (!enableValidationLayers) {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to setup debug messenger");
        }
    }

    void createSurface() {
        // The Vulkan surface connects the Vulkan presentation pipeline to the OS
        // window. Without a surface, Vulkan cannot display images presentable to
        // the user.
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface");
        }
    }

    void pickPhysicalDevice() {
        // A Vulkan app may be able to use multiple GPUs. We enumerate them and
        // choose the one that satisfies our requirements for graphics and surface
        // presentation.
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& candidate : devices) {
            if (isDeviceSuitable(candidate)) {
                physicalDevice = candidate;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU");
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        // A physical device is suitable only if it can do the work we need. This
        // function checks queue support, required extensions, and a few features
        // needed by the sample.
        QueueFamilyIndices indices = findQueueFamilies(device);
        bool extensionSupport = checkDeviceExtensionSupport(device);
        return indices.isComplete() && extensionSupport;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice candidate) {
        // This checks whether the selected GPU supports the extensions needed by
        // the app. Swapchain is required everywhere. Portability subset is
        // enabled opportunistically only when the selected driver advertises it.
        return hasDeviceExtension(candidate, swapchainExtensionName);
    }

    bool hasDeviceExtension(VkPhysicalDevice candidate, const char* extensionName) {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, availableExtensions.data());

        for (const auto& extension : availableExtensions) {
            if (strcmp(extensionName, extension.extensionName) == 0) {
                return true;
            }
        }

        return false;
    }

    std::vector<const char*> getRequiredDeviceExtensions(VkPhysicalDevice candidate) {
        std::vector<const char*> extensions = { swapchainExtensionName };
        if (hasDeviceExtension(candidate, portabilitySubsetExtensionName)) {
            extensions.push_back(portabilitySubsetExtensionName);
        }
        return extensions;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        // The graphics queue and present queue are separate concepts. A GPU may
        // expose them in the same family or different families; this function
        // discovers which one is which.
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            const auto& queueFamily = queueFamilies[i];
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }
        }

        return indices;
    }

    void createLogicalDevice() {
        // A logical device is the actual interface used by the app to interact
        // with a physical GPU. It is created with the queue families and extensions
        // we need for rendering and presenting.
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::vector<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };

        std::sort(uniqueQueueFamilies.begin(), uniqueQueueFamilies.end());
        uniqueQueueFamilies.erase(std::unique(uniqueQueueFamilies.begin(), uniqueQueueFamilies.end()), uniqueQueueFamilies.end());

        std::vector<float> queuePriorities(uniqueQueueFamilies.size(), 1.0f);
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            size_t queueIndex = queueCreateInfos.size();
            queueCreateInfo.pQueuePriorities = &queuePriorities[queueIndex];
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures supportedFeatures{};
        vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);
        supportsWireframe = supportedFeatures.fillModeNonSolid == VK_TRUE;

        VkPhysicalDeviceFeatures deviceFeatures{};
        if (supportsWireframe) {
            deviceFeatures.fillModeNonSolid = VK_TRUE;
        }

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        std::vector<const char*> requiredExtensions = getRequiredDeviceExtensions(physicalDevice);
        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();
        createInfo.enabledLayerCount = 0;

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void createSwapChain() {
        // A swapchain is the presentation layer that connects the Vulkan render
        // target to the window. It owns the images that will be rendered into and
        // later presented to the screen. The app renders into one image, then
        // swaps it to the front buffer for display.
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
        activePresentMode = presentMode;
    }

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        // Before creating a swapchain, Vulkan requires us to inspect the surface's
        // capabilities, formats, and presentation modes. This is how we choose a
        // configuration that matches the window and GPU.
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        // Surface format describes the color channel layout of the swapchain. We
        // prefer the standard sRGB color format used by modern graphics APIs.
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        // Presentation mode controls how frames are displayed: immediate,
        // mailbox, FIFO, etc. MAILBOX is often the most responsive for demos.
        immediatePresentSupported = false;
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                immediatePresentSupported = true;
                break;
            }
        }
        if (preferImmediatePresentMode && immediatePresentSupported) {
            return VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        // Swap extent defines the pixel size of the swapchain images. It usually
        // matches the window's framebuffer size, clamped to the surface limits.
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }

    void createImageViews() {
        // Each swapchain image is wrapped in a VkImageView so the GPU can access
        // it as a texture-like resource for color attachments.
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); ++i) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views");
            }
        }
    }

    void createRenderPass() {
        // A render pass describes the sequence of image attachments and
        // subpasses used during rendering. Here we define a color attachment for
        // the window and a depth attachment for our 3D cube so the pipeline can
        // write color and depth information in the right order. The render pass is
        // essentially the high-level description of the render target layout.
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass");
        }
    }

    void createGraphicsPipeline() {
        // The graphics pipeline is the GPU program configuration: shader stages,
        // vertex input layout, rasterizer state, depth testing, blending, and
        // other fixed-function settings. Once built, it is reusable for every
        // frame while drawing the scene. It is the Vulkan equivalent of the
        // "shader + state machine" that the GPU executes during drawing.
        std::vector<char> vertCode = readFile("shaders/triangle.vert.spv");
        std::vector<char> fragCode = readFile("shaders/triangle.frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertCode);
        VkShaderModule fragShaderModule = createShaderModule(fragCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstants);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline");
        }

        if (supportsWireframe) {
            VkPipelineRasterizationStateCreateInfo wireRasterizer = rasterizer;
            wireRasterizer.polygonMode = VK_POLYGON_MODE_LINE;
            pipelineInfo.pRasterizationState = &wireRasterizer;
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &wireframePipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create wireframe graphics pipeline");
            }
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    std::vector<char> readFile(const std::string& filename) {
        // Shader bytecode files are stored on disk and loaded into memory before
        // creating a VkShaderModule. Vulkan expects SPIR-V code, not raw GLSL text.
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open shader file: " + filename);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        // A shader module is the compiled shader program object Vulkan uses.
        // Once created, it can be attached to the graphics pipeline as the vertex
        // or fragment stage.
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module");
        }

        return shaderModule;
    }

    void createFramebuffers() {
        // Framebuffers bind the render pass attachments to actual images. They are
        // the final link between the swapchain images and the render pass.
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
            std::array<VkImageView, 2> attachments = { swapChainImageViews[i], depthImageView };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer");
            }
        }
    }

    void createCommandPool() {
        // Command pools manage the lifetime of command buffers, which record the
        // GPU operations to be executed. This pool is tied to the graphics queue
        // family that will submit drawing commands.
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool");
        }
    }

    void allocateCommandBuffers() {
        if (!commandBuffers.empty()) {
            vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
            commandBuffers.clear();
        }

        commandBuffers.resize(swapChainFramebuffers.size());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers");
        }
    }

    void cleanupSwapchainDependentResources() {
        if (!commandBuffers.empty()) {
            vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
            commandBuffers.clear();
        }
        for (VkFramebuffer framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        swapChainFramebuffers.clear();

        if (depthImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, depthImageView, nullptr);
            depthImageView = VK_NULL_HANDLE;
        }
        if (depthImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, depthImage, nullptr);
            depthImage = VK_NULL_HANDLE;
        }
        if (depthImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, depthImageMemory, nullptr);
            depthImageMemory = VK_NULL_HANDLE;
        }

        for (VkImageView imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        swapChainImageViews.clear();
        swapChainImages.clear();

        if (swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device, swapChain, nullptr);
            swapChain = VK_NULL_HANDLE;
        }
    }

    void recreateSwapChain() {
        vkDeviceWaitIdle(device);
        cleanupSwapchainDependentResources();
        createSwapChain();
        createImageViews();
        createDepthResources();
        createFramebuffers();
        allocateCommandBuffers();
        ImGui_ImplVulkan_SetMinImageCount(static_cast<uint32_t>(swapChainImages.size()));
        pendingSwapchainRecreate = false;
    }

    void createVertexBuffer() {
        // Vertex data lives on the CPU at first and is uploaded to a GPU buffer.
        // The active mesh can be a cube or a sphere generated with different
        // algorithms and detail levels.
        rebuildMeshBuffer();
    }

    void rebuildMeshBuffer() {
        std::vector<Vertex> vertices;
        if (!useSphere) {
            vertices = generateCubeMesh();
        } else if (sphereGenerator == SphereGenerator::UV) {
            vertices = generateUVSphereMesh(uvLatitudeSegments, uvLongitudeSegments);
        } else {
            vertices = generateIcosphereMesh(icoSubdivisions);
        }

        uploadVerticesToGpu(vertices);
        vertexCount = static_cast<uint32_t>(vertices.size());
        triangleCount = vertexCount / 3;
        meshDirty = false;
    }

    std::vector<Vertex> generateCubeMesh() const {
        std::vector<Vertex> vertices;
        vertices.reserve(36);

        const std::array<std::array<float, 3>, 8> corners = {{
            {{-0.6f, -0.6f, -0.6f}},
            {{ 0.6f, -0.6f, -0.6f}},
            {{ 0.6f,  0.6f, -0.6f}},
            {{-0.6f,  0.6f, -0.6f}},
            {{-0.6f, -0.6f,  0.6f}},
            {{ 0.6f, -0.6f,  0.6f}},
            {{ 0.6f,  0.6f,  0.6f}},
            {{-0.6f,  0.6f,  0.6f}}
        }};

        auto addVertex = [&vertices, &corners](int index, const std::array<float, 3>& color) {
            vertices.push_back({
                {corners[index][0], corners[index][1], corners[index][2]},
                {color[0], color[1], color[2]}
            });
        };

        auto addFace = [&addVertex](int i0, int i1, int i2, int i3, const std::array<float, 3>& color) {
            addVertex(i0, color);
            addVertex(i1, color);
            addVertex(i2, color);
            addVertex(i2, color);
            addVertex(i3, color);
            addVertex(i0, color);
        };

        addFace(4, 5, 6, 7, {1.0f, 0.2f, 0.2f}); // Front
        addFace(1, 0, 3, 2, {0.2f, 1.0f, 0.2f}); // Back
        addFace(0, 4, 7, 3, {0.2f, 0.4f, 1.0f}); // Left
        addFace(5, 1, 2, 6, {1.0f, 0.8f, 0.2f}); // Right
        addFace(3, 7, 6, 2, {0.8f, 0.2f, 1.0f}); // Top
        addFace(0, 1, 5, 4, {0.2f, 1.0f, 1.0f}); // Bottom
        return vertices;
    }

    std::vector<Vertex> generateUVSphereMesh(int latitudeSegments, int longitudeSegments) const {
        const int lat = std::max(3, latitudeSegments);
        const int lon = std::max(3, longitudeSegments);
        const float radius = 0.78f;
        const float pi = 3.14159265358979323846f;

        std::vector<Vertex> vertices;
        vertices.reserve(static_cast<size_t>(lat * lon * 6));

        auto appendVertex = [&vertices, radius](float nx, float ny, float nz) {
            vertices.push_back({
                {radius * nx, radius * ny, radius * nz},
                {(nx + 1.0f) * 0.5f, (ny + 1.0f) * 0.5f, (nz + 1.0f) * 0.5f}
            });
        };

        auto spherePoint = [pi](int stack, int slice, int latCount, int lonCount) {
            float v = static_cast<float>(stack) / static_cast<float>(latCount);
            float phi = v * pi;
            float u = static_cast<float>(slice) / static_cast<float>(lonCount);
            float theta = u * 2.0f * pi;
            float sinPhi = std::sin(phi);
            float x = sinPhi * std::cos(theta);
            float y = std::cos(phi);
            float z = sinPhi * std::sin(theta);
            return std::array<float, 3>{x, y, z};
        };

        for (int stack = 0; stack < lat; ++stack) {
            int nextStack = stack + 1;
            for (int slice = 0; slice < lon; ++slice) {
                int nextSlice = (slice + 1) % lon;
                auto p00 = spherePoint(stack, slice, lat, lon);
                auto p01 = spherePoint(stack, nextSlice, lat, lon);
                auto p10 = spherePoint(nextStack, slice, lat, lon);
                auto p11 = spherePoint(nextStack, nextSlice, lat, lon);

                if (stack != 0) {
                    appendVertex(p00[0], p00[1], p00[2]);
                    appendVertex(p10[0], p10[1], p10[2]);
                    appendVertex(p11[0], p11[1], p11[2]);
                }
                if (stack != lat - 1) {
                    appendVertex(p00[0], p00[1], p00[2]);
                    appendVertex(p11[0], p11[1], p11[2]);
                    appendVertex(p01[0], p01[1], p01[2]);
                }
            }
        }

        return vertices;
    }

    std::vector<Vertex> generateIcosphereMesh(int subdivisions) const {
        struct P3 {
            float x;
            float y;
            float z;
        };

        auto normalize = [](const P3& p) {
            float length = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
            return P3{p.x / length, p.y / length, p.z / length};
        };

        std::vector<P3> points;
        std::vector<std::array<int, 3>> faces;

        const float t = (1.0f + std::sqrt(5.0f)) * 0.5f;
        points = {
            normalize({-1.0f,  t, 0.0f}), normalize({ 1.0f,  t, 0.0f}),
            normalize({-1.0f, -t, 0.0f}), normalize({ 1.0f, -t, 0.0f}),
            normalize({0.0f, -1.0f,  t}), normalize({0.0f,  1.0f,  t}),
            normalize({0.0f, -1.0f, -t}), normalize({0.0f,  1.0f, -t}),
            normalize({ t, 0.0f, -1.0f}), normalize({ t, 0.0f,  1.0f}),
            normalize({-t, 0.0f, -1.0f}), normalize({-t, 0.0f,  1.0f})
        };

        faces = {
            {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
            {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
            {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
            {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
        };

        const int clampedSubdivisions = std::clamp(subdivisions, 0, 6);
        for (int i = 0; i < clampedSubdivisions; ++i) {
            std::unordered_map<uint64_t, int> midpointCache;
            std::vector<std::array<int, 3>> newFaces;
            newFaces.reserve(faces.size() * 4);

            auto midpoint = [&points, &midpointCache, &normalize](int a, int b) {
                int low = std::min(a, b);
                int high = std::max(a, b);
                uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(low)) << 32) |
                               static_cast<uint32_t>(high);
                auto it = midpointCache.find(key);
                if (it != midpointCache.end()) {
                    return it->second;
                }

                P3 pa = points[static_cast<size_t>(a)];
                P3 pb = points[static_cast<size_t>(b)];
                P3 m = normalize({(pa.x + pb.x) * 0.5f, (pa.y + pb.y) * 0.5f, (pa.z + pb.z) * 0.5f});
                points.push_back(m);
                int index = static_cast<int>(points.size() - 1);
                midpointCache.emplace(key, index);
                return index;
            };

            for (const auto& tri : faces) {
                int a = tri[0];
                int b = tri[1];
                int c = tri[2];
                int ab = midpoint(a, b);
                int bc = midpoint(b, c);
                int ca = midpoint(c, a);

                newFaces.push_back({a, ab, ca});
                newFaces.push_back({b, bc, ab});
                newFaces.push_back({c, ca, bc});
                newFaces.push_back({ab, bc, ca});
            }

            faces = std::move(newFaces);
        }

        const float radius = 0.78f;
        std::vector<Vertex> vertices;
        vertices.reserve(faces.size() * 3);
        for (const auto& face : faces) {
            for (int index : face) {
                const P3 p = points[static_cast<size_t>(index)];
                vertices.push_back({
                    {radius * p.x, radius * p.y, radius * p.z},
                    {(p.x + 1.0f) * 0.5f, (p.y + 1.0f) * 0.5f, (p.z + 1.0f) * 0.5f}
                });
            }
        }

        return vertices;
    }

    void uploadVerticesToGpu(const std::vector<Vertex>& vertices) {
        if (vertices.empty()) {
            throw std::runtime_error("mesh has no vertices");
        }

        if (vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, vertexBuffer, nullptr);
            vertexBuffer = VK_NULL_HANDLE;
        }
        if (vertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, vertexBufferMemory, nullptr);
            vertexBufferMemory = VK_NULL_HANDLE;
        }

        VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer,
                     stagingBufferMemory);

        void* data = nullptr;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     vertexBuffer,
                     vertexBufferMemory);

        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createDepthResources() {
        // Depth resources are required when rendering a 3D scene. The depth image
        // stores the distance from the camera for each pixel so the GPU can reject
        // fragments that should be behind other triangles.
        VkFormat depthFormat = findDepthFormat();
        createImage(swapChainExtent.width,
                    swapChainExtent.height,
                    depthFormat,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    depthImage,
                    depthImageMemory);

        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (hasStencilComponent(depthFormat)) {
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        depthImageView = createImageView(depthImage, depthFormat, aspectMask);
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        // Vulkan does not expose a "malloc-like" memory pool directly. Instead,
        // we create buffers and then allocate device memory for them. The memory
        // type depends on whether the buffer is host-visible (for staging) or
        // device-local (for the final GPU-resident resource).
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer");
        }

        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        // Vulkan memory is not uniform; different heap types have different
        // capabilities. We select the memory type that matches the resource's
        // intended usage, such as host-visible staging or GPU-local rendering.
        VkPhysicalDeviceMemoryProperties memProperties{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type");
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void createImage(uint32_t width,
                     uint32_t height,
                     VkFormat format,
                     VkImageTiling tiling,
                     VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     VkImage& image,
                     VkDeviceMemory& imageMemory) {
        // Images are Vulkan's representation of textures and render targets. A depth
        // attachment is just another image with a different format and usage.
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image");
        }

        VkMemoryRequirements memRequirements{};
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory");
        }

        if (vkBindImageMemory(device, image, imageMemory, 0) != VK_SUCCESS) {
            throw std::runtime_error("failed to bind image memory");
        }
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        // Image views are the GPU-facing way of interpreting an image. They define
        // how a raw image should be read, such as a 2D color texture or a depth map.
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView = VK_NULL_HANDLE;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view");
        }

        return imageView;
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                                 VkImageTiling tiling,
                                 VkFormatFeatureFlags features) {
        // Vulkan leaves format selection to the app because GPUs differ. This helper
        // picks a depth format that supports the requested attachment operations.
        for (VkFormat format : candidates) {
            VkFormatProperties props{};
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format");
    }

    VkFormat findDepthFormat() {
        // Depth buffers must be stored in a format that supports depth/stencil
        // attachment operations. Different GPUs support different formats, so we
        // choose the best one available.
        return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    uint32_t estimateTriangleCount() const {
        if (!useSphere) {
            return 12;
        }
        if (sphereGenerator == SphereGenerator::UV) {
            const int lat = std::max(3, uvLatitudeSegments);
            const int lon = std::max(3, uvLongitudeSegments);
            return static_cast<uint32_t>(2 * lon * (lat - 1));
        }
        const int s = std::clamp(icoSubdivisions, 0, 6);
        return static_cast<uint32_t>(20u << (2 * s));
    }

    uint32_t computeUvTriangleCount(int latitudeSegments, int longitudeSegments) const {
        const int lat = std::max(3, latitudeSegments);
        const int lon = std::max(3, longitudeSegments);
        return static_cast<uint32_t>(2 * lon * (lat - 1));
    }

    uint32_t computeIcoTriangleCount(int subdivisions) const {
        const int s = std::clamp(subdivisions, 0, 6);
        return static_cast<uint32_t>(20u << (2 * s));
    }

    void syncTriangleBudgetFromCurrentSettings() {
        triangleBudget = static_cast<int>(estimateTriangleCount());
    }

    void applyTriangleBudgetToGenerator(int requestedBudget) {
        if (sphereGenerator == SphereGenerator::UV) {
            int bestLat = uvLatitudeSegments;
            int bestLon = uvLongitudeSegments;
            int bestDiff = std::numeric_limits<int>::max();

            for (int lat = 3; lat <= 128; ++lat) {
                const int denom = 2 * (lat - 1);
                if (denom <= 0) {
                    continue;
                }
                int baseLon = std::clamp(requestedBudget / denom, 3, 256);
                for (int candidateLon : {baseLon, std::min(baseLon + 1, 256)}) {
                    const int triangles = static_cast<int>(computeUvTriangleCount(lat, candidateLon));
                    const int diff = std::abs(triangles - requestedBudget);
                    if (diff < bestDiff) {
                        bestDiff = diff;
                        bestLat = lat;
                        bestLon = candidateLon;
                    }
                }
            }

            uvLatitudeSegments = bestLat;
            uvLongitudeSegments = bestLon;
            triangleBudget = static_cast<int>(computeUvTriangleCount(uvLatitudeSegments, uvLongitudeSegments));
            return;
        }

        int bestSubdivision = icoSubdivisions;
        int bestDiff = std::numeric_limits<int>::max();
        for (int s = 0; s <= 6; ++s) {
            const int triangles = static_cast<int>(computeIcoTriangleCount(s));
            const int diff = std::abs(triangles - requestedBudget);
            if (diff < bestDiff) {
                bestDiff = diff;
                bestSubdivision = s;
            }
        }
        icoSubdivisions = bestSubdivision;
        triangleBudget = static_cast<int>(computeIcoTriangleCount(icoSubdivisions));
    }

    void initImGui() {
        std::array<VkDescriptorPoolSize, 11> poolSizes = {{
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
        }};

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000 * static_cast<uint32_t>(poolSizes.size());
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create ImGui descriptor pool");
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(window, true);

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = instance;
        initInfo.PhysicalDevice = physicalDevice;
        initInfo.Device = device;
        initInfo.QueueFamily = indices.graphicsFamily.value();
        initInfo.Queue = graphicsQueue;
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPool = imguiDescriptorPool;
        initInfo.RenderPass = renderPass;
        initInfo.Subpass = 0;
        initInfo.MinImageCount = static_cast<uint32_t>(swapChainImages.size());
        initInfo.ImageCount = static_cast<uint32_t>(swapChainImages.size());
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Allocator = nullptr;
        initInfo.CheckVkResultFn = nullptr;
        if (!ImGui_ImplVulkan_Init(&initInfo)) {
            throw std::runtime_error("failed to initialize ImGui Vulkan backend");
        }

        if (!ImGui_ImplVulkan_CreateFontsTexture()) {
            throw std::runtime_error("failed to create ImGui font texture");
        }
        vkQueueWaitIdle(graphicsQueue);
        imguiInitialized = true;
    }

    void shutdownImGui() {
        if (!imguiInitialized) {
            return;
        }
        vkDeviceWaitIdle(device);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);
        imguiDescriptorPool = VK_NULL_HANDLE;
        imguiInitialized = false;
    }

    void buildUi() {
        ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Object")) {
                bool cubeSelected = !useSphere;
                bool sphereSelected = useSphere;
                if (ImGui::MenuItem("Cube", nullptr, cubeSelected) && useSphere) {
                    useSphere = false;
                    meshDirty = true;
                    syncTriangleBudgetFromCurrentSettings();
                }
                if (ImGui::MenuItem("Sphere", nullptr, sphereSelected) && !useSphere) {
                    useSphere = true;
                    meshDirty = true;
                    syncTriangleBudgetFromCurrentSettings();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("Frame time: %.2f ms", frameTimeMs);
        std::array<float, fpsHistorySize> orderedFps{};
        for (int i = 0; i < fpsHistorySize; ++i) {
            orderedFps[static_cast<size_t>(i)] = fpsHistory[static_cast<size_t>((fpsHistoryOffset + i) % fpsHistorySize)];
        }
        ImGui::PlotLines("FPS history", orderedFps.data(), fpsHistorySize, 0, nullptr, 0.0f, 240.0f, ImVec2(0.0f, 70.0f));
        ImGui::Text("Triangles: %u", triangleCount);
        ImGui::Text("Vertices: %u", vertexCount);
        ImGui::Text("Target triangles: %u", estimateTriangleCount());
        ImGui::Text("Present mode: %s", activePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR ? "Immediate (uncapped)" :
                                         activePresentMode == VK_PRESENT_MODE_MAILBOX_KHR ? "Mailbox" :
                                         activePresentMode == VK_PRESENT_MODE_FIFO_KHR ? "FIFO (capped)" : "Other");
        ImGui::Separator();

        int mode = runMode == RunMode::Demo ? 0 : 1;
        if (ImGui::RadioButton("Demo mode", mode == 0) && runMode != RunMode::Demo) {
            runMode = RunMode::Demo;
            waitForPresentQueueIdle = true;
            drawInstanceCount = 1;
            benchmarkCpuCapEnabled = false;
        }
        if (ImGui::RadioButton("Benchmark mode", mode == 1) && runMode != RunMode::Benchmark) {
            runMode = RunMode::Benchmark;
            waitForPresentQueueIdle = false;
        }
        ImGui::Separator();

        if (runMode == RunMode::Benchmark) {
            bool changed = false;
            bool uncappedPresent = preferImmediatePresentMode;
            ImGui::BeginDisabled(!immediatePresentSupported);
            if (ImGui::Checkbox("Uncapped present mode (IMMEDIATE)", &uncappedPresent) && immediatePresentSupported) {
                preferImmediatePresentMode = uncappedPresent;
                changed = true;
            }
            ImGui::EndDisabled();
            if (!immediatePresentSupported) {
                ImGui::TextDisabled("IMMEDIATE present mode not supported by this surface");
            }
            if (changed) {
                pendingSwapchainRecreate = true;
            }

            ImGui::Checkbox("Wait for present queue idle each frame", &waitForPresentQueueIdle);
            ImGui::Checkbox("CPU frame cap", &benchmarkCpuCapEnabled);
            if (benchmarkCpuCapEnabled) {
                ImGui::SliderFloat("CPU cap FPS", &benchmarkCpuCapFps, 10.0f, 240.0f, "%.0f");
            }
            int instances = static_cast<int>(drawInstanceCount);
            if (ImGui::SliderInt("Instance multiplier", &instances, 1, 512)) {
                drawInstanceCount = static_cast<uint32_t>(instances);
            }
            ImGui::Separator();
        }

        if (ImGui::Checkbox("Render sphere", &useSphere)) {
            meshDirty = true;
            syncTriangleBudgetFromCurrentSettings();
        }

        if (supportsWireframe) {
            ImGui::Checkbox("Wireframe", &wireframeEnabled);
        } else {
            ImGui::TextDisabled("Wireframe: unsupported by this GPU");
        }

        if (useSphere) {
            int generator = sphereGenerator == SphereGenerator::UV ? 0 : 1;
            if (ImGui::RadioButton("UV Sphere", generator == 0)) {
                generator = 0;
                sphereGenerator = SphereGenerator::UV;
                syncTriangleBudgetFromCurrentSettings();
                meshDirty = true;
            }
            if (ImGui::RadioButton("Icosphere", generator == 1)) {
                generator = 1;
                sphereGenerator = SphereGenerator::Icosphere;
                syncTriangleBudgetFromCurrentSettings();
                meshDirty = true;
            }

            const int minBudget = sphereGenerator == SphereGenerator::UV ? static_cast<int>(computeUvTriangleCount(3, 3)) : static_cast<int>(computeIcoTriangleCount(0));
            const int maxBudget = sphereGenerator == SphereGenerator::UV ? static_cast<int>(computeUvTriangleCount(128, 256)) : static_cast<int>(computeIcoTriangleCount(6));
            if (ImGui::SliderInt("Triangle budget", &triangleBudget, minBudget, maxBudget)) {
                applyTriangleBudgetToGenerator(triangleBudget);
                meshDirty = true;
            }

            if (sphereGenerator == SphereGenerator::UV) {
                if (ImGui::SliderInt("UV latitude segments", &uvLatitudeSegments, 3, 128)) {
                    syncTriangleBudgetFromCurrentSettings();
                    meshDirty = true;
                }
                if (ImGui::SliderInt("UV longitude segments", &uvLongitudeSegments, 3, 256)) {
                    syncTriangleBudgetFromCurrentSettings();
                    meshDirty = true;
                }
            } else {
                if (ImGui::SliderInt("Icosphere subdivisions", &icoSubdivisions, 0, 6)) {
                    syncTriangleBudgetFromCurrentSettings();
                    meshDirty = true;
                }
            }
        }

        ImGui::Checkbox("Auto rotate", &autoRotate);
        ImGui::SliderFloat("Rotation speed (deg/s)", &rotationSpeedDegreesPerSecond, 0.0f, 360.0f, "%.1f");
        if (ImGui::Button("Reset rotation")) {
            rotationX = 0.0f;
            rotationY = 0.0f;
            rotationZ = 0.0f;
            autoRotationY = 0.0f;
        }
        ImGui::End();
    }

    void createSyncObjects() {
        // Vulkan is asynchronous: the GPU may still be drawing while the CPU starts
        // preparing the next frame. Semaphores and fences coordinate queue
        // submissions so we don't overwrite frames or read from images before the
        // GPU is finished with them.
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects");
        }
    }

    void drawFrame() {
        // This function is the heart of the frame loop. It acquires a swapchain
        // image, records draw commands into a command buffer, submits them to the
        // graphics queue, and then presents the result. In other words, this is
        // where the CPU tells the GPU: "render the cube into this window image."
        auto frameStart = std::chrono::steady_clock::now();
        if (pendingSwapchainRecreate) {
            recreateSwapChain();
        }

        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFence);

        uint32_t imageIndex = 0;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image");
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        buildUi();
        ImGui::Render();

        if (meshDirty) {
            rebuildMeshBuffer();
        }

        vkResetCommandBuffer(commandBuffers[imageIndex], 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.20f, 0.02f, 0.32f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkPipeline activePipeline = graphicsPipeline;
        if (wireframeEnabled && supportsWireframe && wireframePipeline != VK_NULL_HANDLE) {
            activePipeline = wireframePipeline;
        }
        vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, activePipeline);

        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

        PushConstants push{};
        push.rotationX = rotationX;
        push.rotationY = rotationY + autoRotationY;
        push.rotationZ = rotationZ;
        push.aspect = static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        push.time = time;
        vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &push);

        vkCmdDraw(commandBuffers[imageIndex], vertexCount, drawInstanceCount, 0, 0);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[imageIndex]);
        vkCmdEndRenderPass(commandBuffers[imageIndex]);

        if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer");
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            return;
        }
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image");
        }

        if (waitForPresentQueueIdle) {
            vkQueueWaitIdle(presentQueue);
        }

        if (runMode == RunMode::Benchmark && benchmarkCpuCapEnabled && benchmarkCpuCapFps > 0.0f) {
            const auto frameEnd = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration<float>(frameEnd - frameStart).count();
            const float targetSeconds = 1.0f / benchmarkCpuCapFps;
            if (elapsed < targetSeconds) {
                std::this_thread::sleep_for(std::chrono::duration<float>(targetSeconds - elapsed));
            }
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (checkInstanceExtensionSupport(portabilityEnumerationExtensionName)) {
            extensions.push_back(portabilityEnumerationExtensionName);
        }

        if (checkInstanceExtensionSupport("VK_KHR_get_physical_device_properties2")) {
            extensions.push_back("VK_KHR_get_physical_device_properties2");
        }

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    bool checkInstanceExtensionSupport(const char* extensionName) {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

        for (const auto& extension : availableExtensions) {
            if (strcmp(extensionName, extension.extensionName) == 0) {
                return true;
            }
        }
        return false;
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool found = false;
            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }

        return true;
    }
};

int main() {
    VulkanDemo app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
