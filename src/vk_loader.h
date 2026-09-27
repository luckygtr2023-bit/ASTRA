// ===========================================================================
// Project Astra Cosmos — src/vk_loader.h
// Phase 0: runtime Vulkan loader (VK_NO_PROTOTYPES).
//
// The linker never sees a Vulkan import library. Every entry point is
// resolved at runtime from:
//   - Windows : vulkan-1.dll   (LoadLibraryA + GetProcAddress)
//   - Linux   : libvulkan.so.1 (dlopen + dlsym)  [sandbox smoke test only]
//
// Instance-level functions are resolved from the loader library directly;
// device-level functions are resolved via vkGetDeviceProcAddr with a
// fallback to the loader library (standard loader contract).
//
// Single-translation-unit header (included once from main.cpp).
// ===========================================================================
#pragma once

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#include <vulkan/vulkan.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
// ---------------------------------------------------------------------------
// Windows: LoadLibraryA("vulkan-1.dll") + GetProcAddress
// ---------------------------------------------------------------------------
#include <windows.h>
#define VKL_LIB_NAME "vulkan-1.dll"
typedef void* VkLibraryHandle;

static inline VkLibraryHandle vklib_open(void) {
  return LoadLibraryA(VKL_LIB_NAME);
}
static inline void* vklib_sym(VkLibraryHandle lib, const char* name) {
  return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(lib), name));
}
static inline void vklib_close(VkLibraryHandle) {
  // Intentionally no FreeLibrary: the Vulkan loader may keep state (e.g.
  // validation layers) that outlives our handles; the process teardown
  // reclaims the module. Frequent free/reload can crash on some runtimes.
}
#else
// ---------------------------------------------------------------------------
// POSIX (sandbox smoke test only; the shipped binary is Windows).
// ---------------------------------------------------------------------------
#include <dlfcn.h>
#define VKL_LIB_NAME "libvulkan.so.1"
typedef void* VkLibraryHandle;

static inline VkLibraryHandle vklib_open(void) {
  return dlopen(VKL_LIB_NAME, RTLD_NOW | RTLD_GLOBAL);
}
static inline void* vklib_sym(VkLibraryHandle lib, const char* name) {
  return dlsym(lib, name);
}
static inline void vklib_close(VkLibraryHandle lib) {
  if (lib) dlclose(lib);
}
#endif

// ---------------------------------------------------------------------------
// Entry points — with VK_NO_PROTOTYPES these are plain globals we fill in.
// ---------------------------------------------------------------------------
static PFN_vkCreateInstance vkCreateInstance;
static PFN_vkDestroyInstance vkDestroyInstance;
static PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
static PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
static PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
static PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
static PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
static PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
static PFN_vkCreateDevice vkCreateDevice;
static PFN_vkDestroyDevice vkDestroyDevice;
static PFN_vkGetDeviceQueue vkGetDeviceQueue;
static PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

// Surface / platform (Win32)
static PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
static PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#endif
static PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
static PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;

// Device scope (windowed path only)
#ifdef _WIN32
static PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
static PFN_vkQueueWaitIdle vkQueueWaitIdle;
static PFN_vkQueueSubmit vkQueueSubmit;
static PFN_vkQueuePresentKHR vkQueuePresentKHR;
static PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
static PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
static PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
static PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
static PFN_vkCreateRenderPass vkCreateRenderPass;
static PFN_vkDestroyRenderPass vkDestroyRenderPass;
static PFN_vkCreateFramebuffer vkCreateFramebuffer;
static PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
static PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
static PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
static PFN_vkCreateShaderModule vkCreateShaderModule;
static PFN_vkDestroyShaderModule vkDestroyShaderModule;
static PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
static PFN_vkDestroyPipeline vkDestroyPipeline;
static PFN_vkCreateCommandPool vkCreateCommandPool;
static PFN_vkDestroyCommandPool vkDestroyCommandPool;
static PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
static PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
static PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
static PFN_vkEndCommandBuffer vkEndCommandBuffer;
static PFN_vkCreateSemaphore vkCreateSemaphore;
static PFN_vkDestroySemaphore vkDestroySemaphore;
static PFN_vkCreateFence vkCreateFence;
static PFN_vkDestroyFence vkDestroyFence;
static PFN_vkWaitForFences vkWaitForFences;
static PFN_vkResetFences vkResetFences;
static PFN_vkCreateBuffer vkCreateBuffer;
static PFN_vkDestroyBuffer vkDestroyBuffer;
static PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
static PFN_vkAllocateMemory vkAllocateMemory;
static PFN_vkFreeMemory vkFreeMemory;
static PFN_vkMapMemory vkMapMemory;
static PFN_vkUnmapMemory vkUnmapMemory;
static PFN_vkBindBufferMemory vkBindBufferMemory;
static PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
static PFN_vkCmdBindPipeline vkCmdBindPipeline;
static PFN_vkCmdSetViewport vkCmdSetViewport;
static PFN_vkCmdSetScissor vkCmdSetScissor;
static PFN_vkCmdPushConstants vkCmdPushConstants;
static PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
static PFN_vkCmdDraw vkCmdDraw;
static PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
#endif  // _WIN32 (device scope)

// ---------------------------------------------------------------------------
// Loader state
// ---------------------------------------------------------------------------
static VkLibraryHandle g_vklib = nullptr;

#define VKL_MISSING(name) \
  std::fprintf(stderr, "[vk_loader] FATAL: entry point vk" #name " not found in %s\n", VKL_LIB_NAME)

#define VKL_LOAD_LIB(name) \
  do { \
    void* p_ = vklib_sym(g_vklib, "vk" #name); \
    if (!p_) { \
      VKL_MISSING(name); \
      return false; \
    } \
    vk##name = reinterpret_cast<PFN_vk##name>(p_); \
  } while (0)

static bool vk_load_instance_funcs(void) {
  VKL_LOAD_LIB(CreateInstance);
  VKL_LOAD_LIB(DestroyInstance);
  VKL_LOAD_LIB(EnumerateInstanceLayerProperties);
  VKL_LOAD_LIB(EnumerateInstanceExtensionProperties);
  VKL_LOAD_LIB(EnumeratePhysicalDevices);
  VKL_LOAD_LIB(GetPhysicalDeviceProperties);
  VKL_LOAD_LIB(GetPhysicalDeviceMemoryProperties);
  VKL_LOAD_LIB(GetPhysicalDeviceQueueFamilyProperties);
  VKL_LOAD_LIB(CreateDevice);
  VKL_LOAD_LIB(DestroyDevice);
  VKL_LOAD_LIB(GetDeviceQueue);
  VKL_LOAD_LIB(GetDeviceProcAddr);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
  VKL_LOAD_LIB(CreateWin32SurfaceKHR);
#endif
  VKL_LOAD_LIB(DestroySurfaceKHR);
  VKL_LOAD_LIB(GetPhysicalDeviceSurfaceSupportKHR);
  VKL_LOAD_LIB(GetPhysicalDeviceSurfaceCapabilitiesKHR);
  VKL_LOAD_LIB(GetPhysicalDeviceSurfaceFormatsKHR);
  VKL_LOAD_LIB(GetPhysicalDeviceSurfacePresentModesKHR);
  return true;
}

// Device-level entry points: ask the driver first, fall back to the loader.
#ifdef _WIN32
static bool vk_load_device_funcs(VkDevice dev) {
#define VKL_LOAD_DEV(name) \
  do { \
    void* p_ = reinterpret_cast<void*>(vkGetDeviceProcAddr(dev, "vk" #name)); \
    if (!p_) p_ = vklib_sym(g_vklib, "vk" #name); \
    if (!p_) { \
      VKL_MISSING(name); \
      return false; \
    } \
    vk##name = reinterpret_cast<PFN_vk##name>(p_); \
  } while (0)

  VKL_LOAD_DEV(DeviceWaitIdle);
  VKL_LOAD_DEV(QueueWaitIdle);
  VKL_LOAD_DEV(QueueSubmit);
  VKL_LOAD_DEV(QueuePresentKHR);
  VKL_LOAD_DEV(CreateSwapchainKHR);
  VKL_LOAD_DEV(DestroySwapchainKHR);
  VKL_LOAD_DEV(GetSwapchainImagesKHR);
  VKL_LOAD_DEV(AcquireNextImageKHR);
  VKL_LOAD_DEV(CreateRenderPass);
  VKL_LOAD_DEV(DestroyRenderPass);
  VKL_LOAD_DEV(CreateFramebuffer);
  VKL_LOAD_DEV(DestroyFramebuffer);
  VKL_LOAD_DEV(CreatePipelineLayout);
  VKL_LOAD_DEV(DestroyPipelineLayout);
  VKL_LOAD_DEV(CreateShaderModule);
  VKL_LOAD_DEV(DestroyShaderModule);
  VKL_LOAD_DEV(CreateGraphicsPipelines);
  VKL_LOAD_DEV(DestroyPipeline);
  VKL_LOAD_DEV(CreateCommandPool);
  VKL_LOAD_DEV(DestroyCommandPool);
  VKL_LOAD_DEV(AllocateCommandBuffers);
  VKL_LOAD_DEV(FreeCommandBuffers);
  VKL_LOAD_DEV(BeginCommandBuffer);
  VKL_LOAD_DEV(EndCommandBuffer);
  VKL_LOAD_DEV(CreateSemaphore);
  VKL_LOAD_DEV(DestroySemaphore);
  VKL_LOAD_DEV(CreateFence);
  VKL_LOAD_DEV(DestroyFence);
  VKL_LOAD_DEV(WaitForFences);
  VKL_LOAD_DEV(ResetFences);
  VKL_LOAD_DEV(CreateBuffer);
  VKL_LOAD_DEV(DestroyBuffer);
  VKL_LOAD_DEV(GetBufferMemoryRequirements);
  VKL_LOAD_DEV(AllocateMemory);
  VKL_LOAD_DEV(FreeMemory);
  VKL_LOAD_DEV(MapMemory);
  VKL_LOAD_DEV(UnmapMemory);
  VKL_LOAD_DEV(BindBufferMemory);
  VKL_LOAD_DEV(CmdBeginRenderPass);
  VKL_LOAD_DEV(CmdBindPipeline);
  VKL_LOAD_DEV(CmdSetViewport);
  VKL_LOAD_DEV(CmdSetScissor);
  VKL_LOAD_DEV(CmdBindVertexBuffers);
  VKL_LOAD_DEV(CmdPushConstants);
  VKL_LOAD_DEV(CmdDraw);
  VKL_LOAD_DEV(CmdEndRenderPass);
#undef VKL_LOAD_DEV
  return true;
}
#endif  // _WIN32 (windowed path only)

// Open the Vulkan loader library and resolve all instance-level entry points.
// Returns false (with a diagnostic on stderr) if the runtime is missing.
static bool vk_load_library(const char** err) {
  if (err) *err = nullptr;
  g_vklib = vklib_open();
  if (!g_vklib) {
  #ifdef _WIN32
    if (err) *err = "Vulkan loader library not found (vulkan-1.dll). "
                    "Install the Vulkan runtime: https://vulkan.lunarg.com";
#else
    if (err) *err = "Vulkan loader library not found (libvulkan.so.1). "
                    "Install the Vulkan runtime: https://vulkan.lunarg.com";
#endif
    return false;
  }
  if (!vk_load_instance_funcs() || !vkGetDeviceProcAddr) {
    if (err) *err = "Vulkan loader library loaded, but entry points are missing.";
    return false;
  }
  return true;
}

static void vk_unload_library(void) {
  vklib_close(g_vklib);
  g_vklib = nullptr;
}
