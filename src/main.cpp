// ===========================================================================
// Project Astra Cosmos — src/main.cpp
// Phase 0: "Hello Vulkan" — the last sandbox check before Phase 1.
//
// Modes:
//   --headless  Create a VkInstance, enumerate every physical device
//               (name, type, driver version), print queue families, select
//               the first integrated/discrete GPU, clean destroy, exit 0.
//   (default)   Raw Win32 window at 1920x1080 (clamped to the work area),
//               swapchain + render pass + pipeline, a rotating colored
//               triangle, vsync (VK_PRESENT_MODE_FIFO), ESC to quit.
//
// Constraints honored: C++17, Vulkan 1.0, VK_NO_PROTOTYPES (all Vulkan
// functions loaded at runtime — see vk_loader.h), no GLFW, validation
// layers only in debug (!NDEBUG) builds.
// ===========================================================================
#include <vulkan/vulkan.h>
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif
#include "vk_loader.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <string>
#include <vector>


// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
static VkInstance g_instance = VK_NULL_HANDLE;

#ifdef _WIN32
static VkPhysicalDevice g_physical = VK_NULL_HANDLE;
static VkDevice g_device = VK_NULL_HANDLE;
static VkQueue g_queue = VK_NULL_HANDLE;
static VkSurfaceKHR g_surface = VK_NULL_HANDLE;
static VkSwapchainKHR g_swapchain = VK_NULL_HANDLE;
static std::vector<VkImage> g_images;
static VkRenderPass g_render_pass = VK_NULL_HANDLE;
static std::vector<VkFramebuffer> g_framebuffers;
static VkPipelineLayout g_pipeline_layout = VK_NULL_HANDLE;
static VkPipeline g_pipeline = VK_NULL_HANDLE;
static VkShaderModule g_vert_module = VK_NULL_HANDLE;
static VkShaderModule g_frag_module = VK_NULL_HANDLE;
static VkCommandPool g_cmd_pool = VK_NULL_HANDLE;
static VkCommandBuffer g_cmd = VK_NULL_HANDLE;
static VkFence g_fence = VK_NULL_HANDLE;
static VkBuffer g_vertex_buffer = VK_NULL_HANDLE;
static VkDeviceMemory g_vertex_mem = VK_NULL_HANDLE;
static VkSemaphore g_acquire = VK_NULL_HANDLE;
#endif  // _WIN32 (windowed-only state)

#ifdef _WIN32
// Vertex layout: vec2 pos (loc 0) + vec3 color (loc 1) — 20 bytes.
struct Vertex {
  float x, y;
  float r, g, b;
};
static const Vertex kVertices[3] = {
  {-0.45f, 0.35f, 0.43f, 0.91f, 1.00f},  // signal cyan
  {0.45f, 0.35f, 1.00f, 0.70f, 0.28f},   // star amber
  {0.00f, -0.45f, 0.48f, 0.41f, 0.93f},  // warp violet
};
// Push constant: one float — the rotation angle (radians).
struct PushConstants {
  float angle;
};
#endif  // _WIN32 (windowed-only data)

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------
static const char* device_type_str(VkPhysicalDeviceType t) {
  switch (t) {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER: return "other";
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "integrated";
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "discrete";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "virtual";
    case VK_PHYSICAL_DEVICE_TYPE_CPU: return "cpu";
    default: return "unknown";
  }
}

#ifdef _WIN32
static bool read_file(const std::string& path, std::vector<char>& out) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  if (std::fseek(f, 0, SEEK_END) != 0) { std::fclose(f); return false; }
  long n = std::ftell(f);
  if (n <= 0) { std::fclose(f); return false; }
  std::fseek(f, 0, SEEK_SET);
  out.resize(static_cast<size_t>(n));
  size_t got = std::fread(out.data(), 1, out.size(), f);
  std::fclose(f);
  return got == out.size();
}

// Directory of the running executable (ASCII paths are sufficient for the
// shipped layout; the project lives in a normal user profile directory).
static std::string exe_dir() {
#ifdef _WIN32
  char buf[4096];
  DWORD n = GetModuleFileNameA(nullptr, buf, static_cast<DWORD>(sizeof(buf) - 1));
  std::string p(buf, n);
  size_t slash = p.find_last_of("\\/");
  if (slash == std::string::npos) return ".";
  return p.substr(0, slash);
#else
  char buf[4096];
  ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n <= 0) return ".";
  buf[n] = '\0';
  std::string p(buf);
  size_t slash = p.find_last_of("/");
  if (slash == std::string::npos) return ".";
  return p.substr(0, slash);
#endif
}
#endif  // _WIN32 (windowed-only helpers)

// ---------------------------------------------------------------------------
// Instance
// ---------------------------------------------------------------------------
static VkResult create_instance_result() {
  VkApplicationInfo app{};
  app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app.pApplicationName = "AstraCosmos";
  app.applicationVersion = 0;  // 0.0.0
  app.pEngineName = "AstraCosmos Phase 0 (Hello Vulkan)";
  app.engineVersion = 0;
  app.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ci.pApplicationInfo = &app;

#ifdef _WIN32
  const char* exts[2] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
  ci.enabledExtensionCount = 2;
  ci.ppEnabledExtensionNames = exts;
#endif

#ifndef NDEBUG
  // Validation layers: debug builds only.
  uint32_t nlayers = 0;
  vkEnumerateInstanceLayerProperties(&nlayers, nullptr);
  std::vector<VkLayerProperties> layers(nlayers);
  if (nlayers) vkEnumerateInstanceLayerProperties(&nlayers, layers.data());
  for (const auto& l : layers) {
    if (std::strcmp(l.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
      static const char* kValidation = "VK_LAYER_KHRONOS_validation";
      ci.enabledLayerCount = 1;
      ci.ppEnabledLayerNames = &kValidation;
      std::printf("[astra] validation layers enabled (debug build)\n");
      break;
    }
  }
#endif

  return vkCreateInstance(&ci, nullptr, &g_instance);
}

// ---------------------------------------------------------------------------
// Headless mode
// ---------------------------------------------------------------------------
static int run_headless() {
  VkResult r = create_instance_result();
  if (r != VK_SUCCESS) {
    if (r == VK_ERROR_INCOMPATIBLE_DRIVER) {
      std::printf(
          "WARNING: no Vulkan drivers found in this environment "
          "(expected in a sandbox without a GPU/ICD).\n"
          "On the target laptop with the AMD driver, this probe prints every "
          "physical device.\n");
      return 0;
    }
    std::fprintf(stderr, "[astra] vkCreateInstance failed (%d) — is the Vulkan "
                         "runtime installed?\n", static_cast<int>(r));
    return 1;
  }

  uint32_t count = 0;
  vkEnumeratePhysicalDevices(g_instance, &count, nullptr);
  std::vector<VkPhysicalDevice> devs(count);
  if (count) vkEnumeratePhysicalDevices(g_instance, &count, devs.data());

  std::printf("Astra Cosmos - headless Vulkan probe\n");
  std::printf("physical devices: %u\n", static_cast<unsigned>(count));

  int selected = -1;
  for (uint32_t i = 0; i < count; ++i) {
    VkPhysicalDeviceProperties p{};
    vkGetPhysicalDeviceProperties(devs[i], &p);

    uint32_t qf = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(devs[i], &qf, nullptr);
    std::vector<VkQueueFamilyProperties> fams(qf);
    if (qf) vkGetPhysicalDeviceQueueFamilyProperties(devs[i], &qf, fams.data());

    std::printf("[%u] %-40s type=%-10s api=%u.%u.%u driver=%u.%u.%u vendors=%u queues=%u\n",
                static_cast<unsigned>(i),
                p.deviceName,
                device_type_str(p.deviceType),
                VK_VERSION_MAJOR(p.apiVersion),
                VK_VERSION_MINOR(p.apiVersion),
                VK_VERSION_PATCH(p.apiVersion),
                VK_VERSION_MAJOR(p.driverVersion),
                VK_VERSION_MINOR(p.driverVersion),
                VK_VERSION_PATCH(p.driverVersion),
                static_cast<unsigned>(p.vendorID),
                static_cast<unsigned>(qf));
    for (uint32_t q = 0; q < qf; ++q) {
      std::printf("    family %u: queues=%u flags=0x%lx\n",
                  static_cast<unsigned>(q),
                  static_cast<unsigned>(fams[q].queueCount),
                  static_cast<unsigned long>(fams[q].queueFlags));
    }
    if (selected < 0 &&
        (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
         p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)) {
      selected = static_cast<int>(i);
    }
  }

  if (count == 0) {
    std::printf(
        "WARNING: no Vulkan devices visible. In a sandbox without a GPU/ICD this "
        "is expected; the user-side test on the laptop is authoritative.\n");
  } else if (selected >= 0) {
    VkPhysicalDeviceProperties p{};
    vkGetPhysicalDeviceProperties(devs[selected], &p);
    std::printf("selected: [%d] %s (%s)\n", selected, p.deviceName,
                device_type_str(p.deviceType));
  } else {
    std::printf(
        "WARNING: no integrated/discrete GPU among %u device(s); runtime "
        "selection will fall back to the first graphics-capable device.\n",
        static_cast<unsigned>(count));
  }

  vkDestroyInstance(g_instance, nullptr);
  g_instance = VK_NULL_HANDLE;
  return 0;
}

// ---------------------------------------------------------------------------
// Windowed mode (Windows only — raw Win32, no GLFW)
// ---------------------------------------------------------------------------
#ifdef _WIN32

static HWND g_hwnd = nullptr;
static WNDCLASSEXW g_wc{};

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProcW(hwnd, msg, wp, lp);
  }
}

static bool create_window(HINSTANCE inst, int width, int height) {
  g_wc.cbSize = sizeof(g_wc);
  g_wc.style = CS_HREDRAW | CS_VREDRAW;
  g_wc.lpfnWndProc = WndProc;
  g_wc.hInstance = inst;
  g_wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  g_wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  g_wc.lpszClassName = L"AstraCosmosPhase0";
  if (!RegisterClassExW(&g_wc)) {
    std::fprintf(stderr, "[astra] RegisterClassExW failed\n");
    return false;
  }

  // Request 1080p, clamped to the current work area (smaller laptops).
  RECT wa{};
  SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
  long aw = wa.right - wa.left;
  long ah = wa.bottom - wa.top;
  long ww = width, wh = height;
  if (ww > aw) ww = aw;
  if (wh > ah) wh = ah;
  long x = wa.left + (aw - ww) / 2;
  long y = wa.top + (ah - wh) / 2;

  g_hwnd = CreateWindowExW(0, g_wc.lpszClassName,
                           L"Project Astra Cosmos - Phase 0: Hello Vulkan",
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           static_cast<int>(x), static_cast<int>(y),
                           static_cast<int>(ww), static_cast<int>(wh),
                           nullptr, nullptr, inst, nullptr);
  if (!g_hwnd) {
    std::fprintf(stderr, "[astra] CreateWindowExW failed\n");
    return false;
  }
  ShowWindow(g_hwnd, SW_SHOW);
  UpdateWindow(g_hwnd);
  return true;
}

static bool select_physical_device() {
  uint32_t count = 0;
  vkEnumeratePhysicalDevices(g_instance, &count, nullptr);
  if (count == 0) {
    std::fprintf(stderr, "[astra] no Vulkan physical devices\n");
    return false;
  }
  std::vector<VkPhysicalDevice> devs(count);
  vkEnumeratePhysicalDevices(g_instance, &count, devs.data());

  // Preference: first integrated GPU with a surface (the Vega 8 laptop),
  // then first discrete, then any surface-capable device.
  int pick[3] = {-1, -1, -1};
  for (uint32_t i = 0; i < count; ++i) {
    VkBool32 ok = VK_FALSE;
    if (vkGetPhysicalDeviceSurfaceSupportKHR(devs[i], 0, g_surface, &ok) != VK_SUCCESS ||
        !ok) {
      continue;
    }
    VkPhysicalDeviceProperties p{};
    vkGetPhysicalDeviceProperties(devs[i], &p);
    if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && pick[0] < 0) pick[0] = static_cast<int>(i);
    else if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && pick[1] < 0) pick[1] = static_cast<int>(i);
    if (pick[2] < 0) pick[2] = static_cast<int>(i);
  }

  int chosen = (pick[0] >= 0) ? pick[0] : (pick[1] >= 0 ? pick[1] : pick[2]);
  if (chosen < 0) {
    std::fprintf(stderr, "[astra] no physical device supports the Win32 surface\n");
    return false;
  }
  g_physical = devs[chosen];
  VkPhysicalDeviceProperties p{};
  vkGetPhysicalDeviceProperties(g_physical, &p);
  std::printf("[astra] using device: %s (%s)\n", p.deviceName, device_type_str(p.deviceType));
  return true;
}

static uint32_t find_graphics_queue_family() {
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(g_physical, &count, nullptr);
  std::vector<VkQueueFamilyProperties> fams(count);
  vkGetPhysicalDeviceQueueFamilyProperties(g_physical, &count, fams.data());
  for (uint32_t i = 0; i < count; ++i) {
    if (fams[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) return i;
  }
  return UINT32_MAX;
}

static bool create_device(uint32_t qfam) {
  float prio = 1.0f;
  VkDeviceQueueCreateInfo qci{};
  qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  qci.queueFamilyIndex = qfam;
  qci.queueCount = 1;
  qci.pQueuePriorities = &prio;

  const char* exts[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo dci{};
  dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  dci.queueCreateInfoCount = 1;
  dci.pQueueCreateInfos = &qci;
  dci.enabledExtensionCount = 1;
  dci.ppEnabledExtensionNames = exts;

  if (vkCreateDevice(g_physical, &dci, nullptr, &g_device) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkCreateDevice failed\n");
    return false;
  }
  if (!vk_load_device_funcs(g_device)) return false;
  vkGetDeviceQueue(g_device, qfam, 0, &g_queue);
  return true;
}

static bool create_swapchain(VkExtent2D* out_extent, VkFormat* out_format) {
  VkSurfaceCapabilitiesKHR caps{};
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_physical, g_surface, &caps) != VK_SUCCESS) return false;

  // Format: prefer B8G8R8A8_SRGB, else the first offered.
  uint32_t nfmt = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(g_physical, g_surface, &nfmt, nullptr);
  std::vector<VkSurfaceFormatKHR> fmts(nfmt ? nfmt : 1);
  if (nfmt) vkGetPhysicalDeviceSurfaceFormatsKHR(g_physical, g_surface, &nfmt, fmts.data());
  VkFormat format = fmts[0].format;
  VkColorSpaceKHR color_space = fmts[0].colorSpace;
  for (const auto& f : fmts) {
    if (f.format == VK_FORMAT_B8G8R8A8_SRGB) {
      format = f.format;
      color_space = f.colorSpace;
      break;
    }
  }
  // VK_FORMAT_UNDEFINED is the driver's way of saying "any format" — resolve
  // it to a concrete one, because the render pass must be built against the
  // exact swapchain image format.
  if (format == VK_FORMAT_UNDEFINED) {
    format = VK_FORMAT_B8G8R8A8_SRGB;
    color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  }

  // Present mode: FIFO (vsync) — always available, per Phase 0 requirements.
  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  uint32_t nmode = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(g_physical, g_surface, &nmode, nullptr);
  if (nmode == 1) {
    std::vector<VkPresentModeKHR> modes(1);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(g_physical, g_surface, &nmode, modes.data()) == VK_SUCCESS) {
      present_mode = modes[0];
    }
  }

  // Image count: min+1 (triple-ish buffering), clamped to max, floor 2.
  uint32_t img_count = (caps.minImageCount > 0) ? caps.minImageCount + 1 : 2;
  if (caps.maxImageCount > 0 && img_count > caps.maxImageCount) img_count = caps.maxImageCount;
  if (img_count < 2) img_count = 2;

  // Extent: the window client rect, clamped to the surface capabilities.
  RECT rc{};
  GetClientRect(g_hwnd, &rc);
  VkExtent2D ext{static_cast<uint32_t>(rc.right), static_cast<uint32_t>(rc.bottom)};
  if (ext.width < caps.minImageExtentWidth) ext.width = caps.minImageExtentWidth;
  if (ext.height < caps.minImageExtentHeight) ext.height = caps.minImageExtentHeight;
  if (caps.maxImageExtentWidth > 0 && ext.width > caps.maxImageExtentWidth)
    ext.width = caps.maxImageExtentWidth;
  if (caps.maxImageExtentHeight > 0 && ext.height > caps.maxImageExtentHeight)
    ext.height = caps.maxImageExtentHeight;

  VkSwapchainCreateInfoKHR sc{};
  sc.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sc.surface = g_surface;
  sc.minImageCount = img_count;
  sc.imageFormat = format;
  sc.imageColorSpace = color_space;
  sc.imageExtent = ext;
  sc.imageArrayLayers = 1;
  sc.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  sc.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  sc.preTransform = caps.currentTransform;
  sc.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  sc.presentMode = present_mode;
  sc.clipped = VK_TRUE;
  sc.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(g_device, &sc, nullptr, &g_swapchain) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkCreateSwapchainKHR failed\n");
    return false;
  }
  vkGetSwapchainImagesKHR(g_device, g_swapchain, &img_count, nullptr);
  g_images.resize(img_count);
  vkGetSwapchainImagesKHR(g_device, g_swapchain, &img_count, g_images.data());
  *out_extent = ext;
  *out_format = format;
  return true;
}

static bool create_render_pass(VkFormat format) {
  VkAttachmentDescription att{};
  att.format = format;
  att.samples = VK_SAMPLE_COUNT_1_BIT;
  att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkSubpassDescription sub{};
  sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  sub.colorAttachmentCount = 1;
  sub.pColorAttachments = &color_ref;

  VkSubpassDependency dep{};
  dep.srcSubpass = VK_SUBPASS_EXTERNAL;
  dep.dstSubpass = 0;
  dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dep.srcAccessMask = 0;
  dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo rpi{};
  rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rpi.attachmentCount = 1;
  rpi.pAttachments = &att;
  rpi.subpassCount = 1;
  rpi.pSubpasses = &sub;
  rpi.dependencyCount = 1;
  rpi.pDependencies = &dep;

  return vkCreateRenderPass(g_device, &rpi, nullptr, &g_render_pass) == VK_SUCCESS;
}

static bool create_framebuffers(const VkExtent2D& ext) {
  g_framebuffers.resize(g_images.size());
  for (size_t i = 0; i < g_images.size(); ++i) {
    VkFramebufferCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fci.renderPass = g_render_pass;
    fci.attachmentCount = 1;
    fci.pAttachments = &g_images[i];
    fci.width = ext.width;
    fci.height = ext.height;
    fci.layers = 1;
    if (vkCreateFramebuffer(g_device, &fci, nullptr, &g_framebuffers[i]) != VK_SUCCESS) {
      std::fprintf(stderr, "[astra] vkCreateFramebuffer failed\n");
      return false;
    }
  }
  return true;
}

static VkShaderModule load_shader(const std::string& path) {
  std::vector<char> spv;
  if (!read_file(path, spv)) {
    std::fprintf(stderr, "[astra] cannot read shader: %s\n", path.c_str());
    return VK_NULL_HANDLE;
  }
  VkShaderModuleCreateInfo sci{};
  sci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  sci.codeSize = spv.size();
  sci.pCode = reinterpret_cast<const uint32_t*>(spv.data());
  VkShaderModule module = VK_NULL_HANDLE;
  if (vkCreateShaderModule(g_device, &sci, nullptr, &module) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkCreateShaderModule failed for %s\n", path.c_str());
    return VK_NULL_HANDLE;
  }
  return module;
}

static bool create_pipeline(VkExtent2D ext) {
  const std::string dir = exe_dir();
  g_vert_module = load_shader(dir + "/shaders/triangle.vert.spv");
  g_frag_module = load_shader(dir + "/shaders/triangle.frag.spv");
  if (!g_vert_module || !g_frag_module) return false;

  VkPushConstantRange push{};
  push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  push.offset = 0;
  push.size = sizeof(PushConstants);
  VkPipelineLayoutCreateInfo plci{};
  plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  plci.pushConstantRangeCount = 1;
  plci.pPushConstantRanges = &push;
  if (vkCreatePipelineLayout(g_device, &plci, nullptr, &g_pipeline_layout) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkCreatePipelineLayout failed\n");
    return false;
  }

  VkVertexInputBindingDescription binding{};
  binding.binding = 0;
  binding.stride = sizeof(Vertex);
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  VkVertexInputAttributeDescription attrs[2] = {};
  attrs[0].location = 0;
  attrs[0].binding = 0;
  attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
  attrs[0].offset = 0;
  attrs[1].location = 1;
  attrs[1].binding = 0;
  attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrs[1].offset = 8;

  VkPipelineVertexInputStateCreateInfo vinci{};
  vinci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vinci.vertexBindingDescriptionCount = 1;
  vinci.pVertexBindingDescriptions = &binding;
  vinci.vertexAttributeDescriptionCount = 2;
  vinci.pVertexAttributeDescriptions = attrs;

  VkPipelineInputAssemblyStateCreateInfo iaci{};
  iaci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  iaci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineViewportStateCreateInfo vpst{};
  vpst.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  vpst.viewportCount = 1;
  vpst.scissorCount = 1;
  vpst.viewportDynamicState = VK_TRUE;
  vpst.scissorDynamicState = VK_TRUE;

  VkPipelineRasterizationStateCreateInfo rasci{};
  rasci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasci.polygonMode = VK_POLYGON_MODE_FILL;
  rasci.cullMode = VK_CULL_MODE_NONE;
  rasci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasci.lineWidth = 1.0f;

  VkPipelineColorBlendAttachmentState blend_att{};
  blend_att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  VkPipelineColorBlendStateCreateInfo cbci{};
  cbci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  cbci.attachmentCount = 1;
  cbci.pAttachments = &blend_att;

  VkPipelineMultisampleStateCreateInfo msci{};
  msci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  msci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineShaderStageCreateInfo stages[2] = {};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = g_vert_module;
  stages[0].pName = "main";
  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = g_frag_module;
  stages[1].pName = "main";

  VkDynamicState dyn[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynci{};
  dynci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynci.dynamicStateCount = 2;
  dynci.pDynamicStates = dyn;

  VkGraphicsPipelineCreateInfo gpci{};
  gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  gpci.stageCount = 2;
  gpci.pStages = stages;
  gpci.pVertexInputState = &vinci;
  gpci.pInputAssemblyState = &iaci;
  gpci.pViewportState = &vpst;
  gpci.pRasterizationState = &rasci;
  gpci.pMultisampleState = &msci;
  gpci.pColorBlendState = &cbci;
  gpci.pDynamicState = &dynci;
  gpci.layout = g_pipeline_layout;
  gpci.renderPass = g_render_pass;

  if (vkCreateGraphicsPipelines(g_device, VK_NULL_HANDLE, 1, &gpci, nullptr,
                                &g_pipeline) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkCreateGraphicsPipelines failed\n");
    return false;
  }
  (void)ext;
  return true;
}

static bool create_vertex_buffer() {
  VkBufferCreateInfo bci{};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.size = sizeof(kVertices);
  bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(g_device, &bci, nullptr, &g_vertex_buffer) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkCreateBuffer failed\n");
    return false;
  }
  VkMemoryRequirements mr{};
  vkGetBufferMemoryRequirements(g_device, g_vertex_buffer, &mr);

  VkPhysicalDeviceMemoryProperties mprops{};
  vkGetPhysicalDeviceMemoryProperties(g_physical, &mprops);
  uint32_t mem_type = 0;
  for (uint32_t i = 0; i < mprops.memoryTypeCount; ++i) {
    if ((mr.memoryTypeBits & (1u << i)) &&
        (mprops.memoryTypes[i].propertyFlags &
         (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
      mem_type = i;
      break;
    }
  }
  if (mem_type == 0 && !(mprops.memoryTypes[0].propertyFlags &
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
    std::fprintf(stderr, "[astra] no host-visible memory type\n");
    return false;
  }

  VkMemoryAllocateInfo mi{};
  mi.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mi.allocationSize = mr.size;
  mi.memoryTypeIndex = mem_type;
  if (vkAllocateMemory(g_device, &mi, nullptr, &g_vertex_mem) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkAllocateMemory failed\n");
    return false;
  }
  if (vkBindBufferMemory(g_device, g_vertex_buffer, g_vertex_mem, 0) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkBindBufferMemory failed\n");
    return false;
  }
  void* data = nullptr;
  if (vkMapMemory(g_device, g_vertex_mem, 0, sizeof(kVertices), 0, &data) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkMapMemory failed\n");
    return false;
  }
  std::memcpy(data, kVertices, sizeof(kVertices));
  vkUnmapMemory(g_device, g_vertex_mem);
  return true;
}

static bool create_command_resources() {
  VkCommandPoolCreateInfo cpci{};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cpci.queueFamilyIndex = 0;  // the device has exactly one (graphics) family
  if (vkCreateCommandPool(g_device, &cpci, nullptr, &g_cmd_pool) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkCreateCommandPool failed\n");
    return false;
  }
  VkCommandBufferAllocateInfo cai{};
  cai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cai.commandPool = g_cmd_pool;
  cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cai.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(g_device, &cai, &g_cmd) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkAllocateCommandBuffers failed\n");
    return false;
  }
  VkSemaphoreCreateInfo sci{};
  sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  if (vkCreateSemaphore(g_device, &sci, nullptr, &g_acquire) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkCreateSemaphore failed\n");
    return false;
  }
  VkFenceCreateInfo fci{};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (vkCreateFence(g_device, &fci, nullptr, &g_fence) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkCreateFence failed\n");
    return false;
  }
  return true;
}

// Safe to call after ANY early failure: every scope is null-guarded, because
// the failure paths reach here with the device/surface not yet created.
static void destroy_windowed_resources() {
  if (g_device) {
    vkDeviceWaitIdle(g_device);
    if (g_vertex_mem) vkFreeMemory(g_device, g_vertex_mem, nullptr);
    if (g_vertex_buffer) vkDestroyBuffer(g_device, g_vertex_buffer, nullptr);
    for (auto fb : g_framebuffers) vkDestroyFramebuffer(g_device, fb, nullptr);
    if (g_pipeline) vkDestroyPipeline(g_device, g_pipeline, nullptr);
    if (g_pipeline_layout) vkDestroyPipelineLayout(g_device, g_pipeline_layout, nullptr);
    if (g_vert_module) vkDestroyShaderModule(g_device, g_vert_module, nullptr);
    if (g_frag_module) vkDestroyShaderModule(g_device, g_frag_module, nullptr);
    if (g_render_pass) vkDestroyRenderPass(g_device, g_render_pass, nullptr);
    if (g_cmd) vkFreeCommandBuffers(g_device, g_cmd_pool, 1, &g_cmd);
    if (g_cmd_pool) vkDestroyCommandPool(g_device, g_cmd_pool, nullptr);
    if (g_fence) vkDestroyFence(g_device, g_fence, nullptr);
    if (g_acquire) vkDestroySemaphore(g_device, g_acquire, nullptr);
    if (g_swapchain) vkDestroySwapchainKHR(g_device, g_swapchain, nullptr);
    vkDestroyDevice(g_device, nullptr);
    g_device = VK_NULL_HANDLE;
  }
  if (g_surface) {
    vkDestroySurfaceKHR(g_instance, g_surface, nullptr);
    g_surface = VK_NULL_HANDLE;
  }
  if (g_instance) {
    vkDestroyInstance(g_instance, nullptr);
    g_instance = VK_NULL_HANDLE;
  }
}

static int run_windowed() {
  HINSTANCE inst = GetModuleHandleW(nullptr);
  if (!create_window(inst, 1920, 1080)) return 1;

  if (create_instance_result() != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] instance creation failed\n");
    destroy_windowed_resources();
    return 1;
  }

  VkWin32SurfaceCreateInfoKHR wsci{};
  wsci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  wsci.hinstance = inst;
  wsci.hwnd = g_hwnd;
  if (vkCreateWin32SurfaceKHR(g_instance, &wsci, nullptr, &g_surface) != VK_SUCCESS) {
    std::fprintf(stderr, "[astra] vkCreateWin32SurfaceKHR failed\n");
    destroy_windowed_resources();
    return 1;
  }

  if (!select_physical_device() || !create_device(find_graphics_queue_family())) {
    destroy_windowed_resources();
    return 1;
  }

  VkExtent2D ext{};
  VkFormat sc_format = VK_FORMAT_B8G8R8A8_SRGB;
  if (!create_swapchain(&ext, &sc_format) || !create_render_pass(sc_format) ||
      !create_framebuffers(ext) || !create_pipeline(ext) || !create_vertex_buffer() ||
      !create_command_resources()) {
    destroy_windowed_resources();
    return 1;
  }

  std::printf("[astra] swapchain %ux%u, images=%zu — ESC to quit\n",
              ext.width, ext.height, g_images.size());
  std::fflush(stdout);

  // -----------------------------------------------------------------
  // Frame loop: vsynced, fence-guarded, one triangle.
  // -----------------------------------------------------------------
  float angle = 0.0f;
  bool running = true;
  while (running) {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT ||
          (msg.message == WM_KEYDOWN && static_cast<WORD>(msg.wParam) == VK_ESCAPE)) {
        running = false;
      }
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
      if (!running) break;
    }
    if (!running) break;

    uint32_t idx = 0;
    if (vkAcquireNextImageKHR(g_device, g_swapchain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &idx) != VK_SUCCESS) {
      continue;  // surface busy — skip frame
    }

    // Record the (tiny) frame into a command buffer.
    VkCommandBufferBeginInfo cbi{};
    cbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(g_cmd, &cbi) != VK_SUCCESS) continue;

    VkClearValue cv{};
    cv.color.float32[0] = 0.02f;   // deep-space clear color
    cv.color.float32[1] = 0.02f;
    cv.color.float32[2] = 0.045f;
    cv.color.float32[3] = 1.0f;
    VkRenderPassBeginInfo rpi{};
    rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpi.renderPass = g_render_pass;
    rpi.framebuffer = g_framebuffers[idx];
    rpi.renderArea = {{0, 0}, {static_cast<int32_t>(ext.width),
                               static_cast<int32_t>(ext.height)}};
    rpi.clearValueCount = 1;
    rpi.pClearValues = &cv;
    vkCmdBeginRenderPass(g_cmd, &rpi, nullptr);
    vkCmdBindPipeline(g_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline);
    VkViewport vp{0.0f, 0.0f, static_cast<float>(ext.width),
                  static_cast<float>(ext.height), 0.0f, 1.0f};
    VkRect2D sc{{0, 0}, {static_cast<uint32_t>(ext.width),
                         static_cast<uint32_t>(ext.height)}};
    vkCmdSetViewport(g_cmd, 0, 1, &vp);
    vkCmdSetScissor(g_cmd, 0, 1, &sc);
    VkBuffer vb = g_vertex_buffer;
    VkDeviceSize off = 0;
    vkCmdBindVertexBuffers(g_cmd, 0, 1, &vb, &off);
    PushConstants pc{angle};
    vkCmdPushConstants(g_cmd, g_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(pc), &pc);
    vkCmdDraw(g_cmd, 3, 1, 0, 0);
    vkCmdEndRenderPass(g_cmd);
    vkEndCommandBuffer(g_cmd);

    VkPipelineStageFlags wait_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &g_acquire;
    si.pWaitDstStageMask = &wait_mask;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &g_cmd;

    if (vkQueueSubmit(g_queue, 1, &si, g_fence) != VK_SUCCESS) continue;

    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.swapchainCount = 1;
    pi.pSwapchains = &g_swapchain;
    pi.pImageIndices = &idx;
    if (vkQueuePresentKHR(g_queue, &pi) != VK_SUCCESS) {
      // Present failed (e.g. window minimized) — still drain the fence.
    }

    vkWaitForFences(g_device, 1, &g_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(g_device, 1, &g_fence);
    angle += 0.02f;
  }

  destroy_windowed_resources();
  return 0;
}

#endif  // _WIN32

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
  bool headless = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--headless") == 0) headless = true;
    // Other flags (e.g. --cpu-priority=high from .start.bat) are accepted and
    // ignored in Phase 0; they become real flags in Phase 1.
  }

  const char* err = nullptr;
  if (!vk_load_library(&err)) {
    std::fprintf(stderr, "[astra] %s\n", err ? err : "Vulkan load failed");
    return 1;
  }

  int rc;
  if (headless) {
    rc = run_headless();
  } else {
#ifdef _WIN32
    rc = run_windowed();
#else
    std::fprintf(stderr,
                 "[astra] window mode requires Windows (vulkan-1.dll). "
                 "In this environment use --headless.\n");
    rc = 1;
#endif
  }

  vk_unload_library();
  return rc;
}
