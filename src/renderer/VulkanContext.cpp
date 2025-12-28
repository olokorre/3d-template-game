#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VulkanContext.h"
#include <iostream>

void VulkanContext::init(GLFWwindow* window, const char* appName) {
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name(appName)
        .request_validation_layers(true)
        .require_api_version(1, 3, 0)
        .use_default_debug_messenger()
        .build();

    if (!inst_ret) {
        std::cerr << "Falha ao criar Vulkan Instance: " << inst_ret.error().message() << "\n";
        return;
    }
    instance = inst_ret.value();

    if (glfwCreateWindowSurface(instance.instance, window, nullptr, &surface) != VK_SUCCESS) {
        std::cerr << "Falha ao criar Window Surface\n";
        return;
    }

    vkb::PhysicalDeviceSelector selector{instance};
    auto phys_ret = selector.set_surface(surface)
        .set_minimum_version(1, 3)
        .select();

    if (!phys_ret) {
        std::cerr << "Falha ao selecionar Physical Device: " << phys_ret.error().message() << "\n";
        return;
    }
    physicalDevice = phys_ret.value();

    vkb::DeviceBuilder deviceBuilder{physicalDevice};
    auto dev_ret = deviceBuilder.build();
    if (!dev_ret) {
        std::cerr << "Falha ao criar Vulkan Device: " << dev_ret.error().message() << "\n";
        return;
    }
    device = dev_ret.value();

    graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
    presentQueue = device.get_queue(vkb::QueueType::present).value();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = device.get_queue_index(vkb::QueueType::graphics).value();

    if (vkCreateCommandPool(device.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        std::cerr << "Falha ao criar Command Pool\n";
        return;
    }

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice.physical_device;
    allocatorInfo.device = device.device;
    allocatorInfo.instance = instance.instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    
    vmaCreateAllocator(&allocatorInfo, &allocator);

    std::cout << "Vulkan inicializado com sucesso! GPU: " << physicalDevice.name << "\n";
}

void VulkanContext::cleanup() {
    if (allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(allocator);
    }
    
    vkDestroyCommandPool(device.device, commandPool, nullptr);
    vkb::destroy_device(device);
    vkDestroySurfaceKHR(instance.instance, surface, nullptr);
    vkb::destroy_instance(instance);
}
