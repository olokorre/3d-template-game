#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

class VulkanContext;

class Mesh {
public:
    struct MeshBuffer {
        VkBuffer buffer;
        VmaAllocation allocation;
    };

    Mesh(VulkanContext* context, const std::vector<Vertex>& vertices);
    ~Mesh();

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);

private:
    void createVertexBuffer(const std::vector<Vertex>& vertices);

    VulkanContext* context;
    MeshBuffer vertexBuffer;
    uint32_t vertexCount;
};
