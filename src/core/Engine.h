#pragma once

#include <memory>
#include <string>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct GLFWwindow;
class VulkanContext;
class Swapchain;
class Pipeline;
class Mesh;
class Camera;

class Engine {
public:
    Engine();
    ~Engine();

    void init();
    void run();
    void cleanup();

private:
    void initWindow();
    
    int width{1280};
    int height{720};
    std::string windowTitle{"Platformer 3D"};
    
    GLFWwindow* window{nullptr};
    std::unique_ptr<VulkanContext> vulkanContext;
    std::unique_ptr<Swapchain> swapchain;
    std::unique_ptr<Pipeline> pipeline;
    
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Mesh> groundMesh;
    std::unique_ptr<Mesh> playerMesh;
    
    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};

    // Game State
    glm::vec3 playerPosition{0.0f, 0.0f, 0.0f};
    float playerRotation{0.0f};

    // Camera State
    float cameraYaw{0.0f};   // Angle around Y axis
    float cameraPitch{20.0f}; // Angle up/down (starts looking slightly down)
    float cameraDistance{8.0f};
    
    // Input State
    double lastMouseX{0.0};
    double lastMouseY{0.0};
    bool firstMouse{true};

    void processInput();
    void createPipeline();
    void createCommandBuffer();
    void createScene();
    void updateCamera();
    void drawFrame();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    bool isInitialized{false};
};
