#pragma once

#include <memory>
#include <string>
#include <vector>
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
    std::unique_ptr<Mesh> obstacleMesh;
    std::unique_ptr<Mesh> exitMesh;
    std::unique_ptr<Mesh> enemyMesh;
    std::unique_ptr<Mesh> followerMesh;
    
    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};

    enum class GameState { MAIN_MENU, PLAYING, GAME_OVER, VICTORY };
    GameState currentState{GameState::MAIN_MENU};

    // Game State
    float playerHealth{100.0f};
    float maxHealth{100.0f};
    glm::vec3 playerPosition{0.0f, 1.0f, 0.0f}; // Start slightly above ground
    float playerVelocityY{0.0f};
    bool isGrounded{false};
    float playerRotation{0.0f};
    glm::vec3 playerKnockback{0.0f};

    // Physics State
    struct AABB {
        glm::vec3 min;
        glm::vec3 max;
    };
    
    struct Enemy {
        AABB box;
        glm::vec3 position;
        char type; // 'X' or 'F'
    };
    
    std::vector<AABB> obstacles;
    std::vector<AABB> exits;
    std::vector<Enemy> enemies;
    float minX{0}, maxX{0}, minZ{0}, maxZ{0};
    int currentLevelIndex = 0;
    bool checkCollision(const glm::vec3& pos, const AABB& playerBox, const AABB& obstacle);
    bool hasLineOfSight(const glm::vec3& start, const glm::vec3& end);
    void takeDamage(float amount, const glm::vec3& sourcePos = glm::vec3(0.0f));
    void restartLevel();

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
    void loadLevel(int levelIndex);
    void updateCamera();
    void drawFrame();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    bool isInitialized{false};
};
