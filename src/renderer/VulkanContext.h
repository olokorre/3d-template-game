#pragma once

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

struct GLFWwindow;

class VulkanContext {
public:
    VulkanContext() = default;
    ~VulkanContext() = default;

    void init(GLFWwindow* window, const char* appName);
    void cleanup();

    VkDevice getDevice() const { return device.device; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice.physical_device; }
    VkInstance getInstance() const { return instance.instance; }
    VkSurfaceKHR getSurface() const { return surface; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    VkCommandPool getCommandPool() const { return commandPool; }
    uint32_t getGraphicsQueueFamily() const { return device.get_queue_index(vkb::QueueType::graphics).value(); }
    VmaAllocator getAllocator() const { return allocator; }

private:
    vkb::Instance instance;
    vkb::PhysicalDevice physicalDevice;
    vkb::Device device;
    VkSurfaceKHR surface{VK_NULL_HANDLE};
    
    VkQueue graphicsQueue{VK_NULL_HANDLE};
    VkQueue presentQueue{VK_NULL_HANDLE};

    VkCommandPool commandPool{VK_NULL_HANDLE};
    VmaAllocator allocator{VK_NULL_HANDLE};
};
