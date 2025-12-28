#pragma once

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <vector>
#include <vk_mem_alloc.h> // Added include

class VulkanContext;

class Swapchain {
public:
    Swapchain() = default;
    ~Swapchain() = default;

    void init(VulkanContext* context, uint32_t width, uint32_t height);
    void cleanup();

    vkb::Swapchain getSwapchain() const { return swapchain; }
    VkRenderPass getRenderPass() const { return renderPass; }
    const std::vector<VkFramebuffer>& getFramebuffers() const { return framebuffers; }
    VkExtent2D getExtent() const { return swapchain.extent; }

private:
    void createSwapchain(uint32_t width, uint32_t height);
    void createImageViews();
    void createRenderPass();
    void createDepthResources();
    void createFramebuffers();

    VulkanContext* context{nullptr};
    vkb::Swapchain swapchain;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkImage depthImage{VK_NULL_HANDLE};
    VmaAllocation depthImageAllocation; // Replaced VkDeviceMemory with VmaAllocation
    VkImageView depthImageView{VK_NULL_HANDLE};
    VkFormat depthFormat{VK_FORMAT_UNDEFINED};

    VkRenderPass renderPass{VK_NULL_HANDLE};
};
