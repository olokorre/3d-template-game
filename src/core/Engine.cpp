#include <GLFW/glfw3.h>
#include "Engine.h"
#include "../renderer/VulkanContext.h"
#include "../renderer/Swapchain.h"
#include "../renderer/Pipeline.h"
#include "../renderer/Mesh.h"
#include "Camera.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include "../assets/levels/AllLevels.h"

Engine::Engine() {
    vulkanContext = std::make_unique<VulkanContext>();
}

Engine::~Engine() {
    cleanup();
}



void Engine::init() {
    initWindow();
    vulkanContext->init(window, windowTitle.c_str());
    
    // Swapchain init
    swapchain = std::make_unique<Swapchain>();
    swapchain->init(vulkanContext.get(), width, height);

    createPipeline();
    
    camera = std::make_unique<Camera>();
    createScene();

    createCommandBuffer();

    isInitialized = true;
}

void Engine::createScene() {
    // 1. Ground Mesh (Quad on XZ plane)
    std::vector<Vertex> groundVertices = {
        {{-5.0f, 0.0f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.3f, 0.3f}},
        {{-5.0f, 0.0f,  5.0f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.3f, 0.3f}},
        {{ 5.0f, 0.0f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.3f, 0.3f}},
        
        {{ 5.0f, 0.0f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.3f, 0.3f}},
        {{-5.0f, 0.0f,  5.0f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.3f, 0.3f}},
        {{ 5.0f, 0.0f,  5.0f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.3f, 0.3f}}
    };
    groundMesh = std::make_unique<Mesh>(vulkanContext.get(), groundVertices);

    // 2. Helper for Cube
    auto createCubeVertices = [&](glm::vec3 color) -> std::vector<Vertex> {
        std::vector<Vertex> v;
        auto addQuad = [&](glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, glm::vec3 c) {
            v.push_back({p1, {0,1,0}, c});
            v.push_back({p2, {0,1,0}, c});
            v.push_back({p3, {0,1,0}, c});
            v.push_back({p3, {0,1,0}, c});
            v.push_back({p2, {0,1,0}, c});
            v.push_back({p4, {0,1,0}, c});
        };
        // Front
        addQuad({-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, color);
        // Back
        addQuad({0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, color);
        // Left
        addQuad({-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, color);
        // Right
        addQuad({0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, color);
        // Top
        addQuad({-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, -0.5f}, color);
        // Bottom
        addQuad({-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f}, color);
        return v;
    };

    // Player Mesh (Cyan/Blue)
    playerMesh = std::make_unique<Mesh>(vulkanContext.get(), createCubeVertices({0.0f, 0.8f, 1.0f}));

    // Obstacle Mesh (Red)
    obstacleMesh = std::make_unique<Mesh>(vulkanContext.get(), createCubeVertices({1.0f, 0.2f, 0.2f}));

    // Enemy Mesh (Magenta)
    enemyMesh = std::make_unique<Mesh>(vulkanContext.get(), createCubeVertices({1.0f, 0.0f, 1.0f}));

    // Follower Mesh (Orange)
    followerMesh = std::make_unique<Mesh>(vulkanContext.get(), createCubeVertices({1.0f, 0.5f, 0.0f}));

    // Exit Mesh (Green)
    exitMesh = std::make_unique<Mesh>(vulkanContext.get(), createCubeVertices({0.0f, 1.0f, 0.0f}));

    // 3. Load Level
    loadLevel(currentLevelIndex);
}

void Engine::loadLevel(int levelIndex) {
    if (levelIndex >= Assets::ALL_LEVELS.size()) {
        std::cout << "Parabéns! Você completou todas as fases!\n";
        currentLevelIndex = 0; // Reset
        levelIndex = 0;
        currentState = GameState::VICTORY;
        return;
    }

    std::string levelData = Assets::ALL_LEVELS[levelIndex];
    std::stringstream file(levelData);
    
    obstacles.clear();
    exits.clear();
    enemies.clear();
    
    // Reset Physics State
    playerVelocityY = 0.0f;
    isGrounded = false;
    playerHealth = maxHealth;
    playerKnockback = glm::vec3(0.0f);
    currentState = GameState::PLAYING;
    
    std::string line;
    int row = 0;
    
    // Grid: Z increases with rows (down), X increases with columns (right)
    // Map center offset could be added later. For now, top-left of file is (0,0) in world?
    // Let's center it a bit. Let's say top-left is (-10, -10) or similar.
    // Or just start at (0,0) and let the player figure it out.
    // Let's use (x, z) = (col, row). 1 unit per char.
    
    float offsetZ = -5.0f; // Center grid roughly
    float offsetX = -10.0f;
    
    int maxCols = 0;
    while (std::getline(file, line)) {
        if (line.length() > maxCols) maxCols = line.length();
        for (int col = 0; col < line.length(); col++) {
            char c = line[col];
            float x = static_cast<float>(col) + offsetX;
            float z = static_cast<float>(row) + offsetZ;
            
            if (c == '#') {
                // Obstacle (1x1x1)
                // Position is centered at x, 0.5 (on ground), z
                // AABB Min/Max
                glm::vec3 min{x - 0.5f, 0.0f, z - 0.5f};
                glm::vec3 max{x + 0.5f, 1.0f, z + 0.5f};
                obstacles.push_back({min, max});
            } else if (c == 'P') {
                // Player Start
                playerPosition = {x, 1.0f, z}; // Slightly above ground
            } else if (c == 'E') {
                // Exit Block
                glm::vec3 min{x - 0.5f, 0.0f, z - 0.5f};
                glm::vec3 max{x + 0.5f, 1.0f, z + 0.5f};
                exits.push_back({min, max});
            } else if (c == 'X') {
                // Static Enemy
                glm::vec3 pos{x, 0.5f, z};
                AABB box{{x - 0.5f, 0.0f, z - 0.5f}, {x + 0.5f, 1.0f, z + 0.5f}};
                enemies.push_back({box, pos, 'X'});
            } else if (c == 'F') {
                // Follower Enemy
                glm::vec3 pos{x, 0.5f, z};
                AABB box{{x - 0.5f, 0.0f, z - 0.5f}, {x + 0.5f, 1.0f, z + 0.5f}};
                enemies.push_back({box, pos, 'F'});
            }
        }
        row++;
    }

    // Set Boundaries
    minX = offsetX - 0.5f;
    maxX = offsetX + static_cast<float>(maxCols) - 0.5f;
    minZ = offsetZ - 0.5f;
    maxZ = offsetZ + static_cast<float>(row) - 0.5f;
}

void Engine::takeDamage(float amount, const glm::vec3& sourcePos) {
    if (currentState != GameState::PLAYING) return;
    
    playerHealth -= amount;
    // std::cout << "Vida: " << playerHealth << "/" << maxHealth << "\n";
    
    // Apply Knockback
    if (amount > 0.0f) {
        glm::vec3 knockDir = playerPosition - sourcePos;
        knockDir.y = 0.0f; // Only horizontal knockback for now
        if (glm::length(knockDir) < 0.001f) {
            knockDir = glm::vec3(0.0f, 0.0f, 1.0f); // Default dir
        }
        playerKnockback = glm::normalize(knockDir) * 0.3f;
    }

    if (playerHealth <= 0) {
        playerHealth = 0;
        currentState = GameState::GAME_OVER;
        std::cout << "GAME OVER! Pressione Enter para tentar novamente.\n";
    }
}

void Engine::restartLevel() {
    loadLevel(currentLevelIndex);
}

bool Engine::hasLineOfSight(const glm::vec3& start, const glm::vec3& end) {
    glm::vec3 dir = end - start;
    float dist = glm::length(dir);
    if (dist < 0.001f) return true;
    dir = glm::normalize(dir);

    // Simple raycast: check points along the line
    int steps = static_cast<int>(dist * 2.0f); // 0.5 unit steps
    for (int i = 1; i <= steps; i++) {
        glm::vec3 p = start + dir * (static_cast<float>(i) * 0.5f);
        if (p.y < 0.0f) continue; // Below ground check?
        
        // Check against obstacles
        for (const auto& obs : obstacles) {
            if (p.x >= obs.min.x && p.x <= obs.max.x &&
                p.y >= obs.min.y && p.y <= obs.max.y &&
                p.z >= obs.min.z && p.z <= obs.max.z) {
                return false; // Hitting a wall
            }
        }
    }
    return true;
}


void Engine::createPipeline() {
    PipelineConfigInfo pipelineConfig{};
    Pipeline::defaultPipelineConfigInfo(pipelineConfig);
    
    pipelineConfig.renderPass = swapchain->getRenderPass();
    pipelineConfig.pipelineLayout = VK_NULL_HANDLE; 

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4); 
    
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    VkPipelineLayout layout;
    if (vkCreatePipelineLayout(vulkanContext->getDevice(), &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("Falha ao criar pipeline layout!");
    }
    pipelineConfig.pipelineLayout = layout;

    pipeline = std::make_unique<Pipeline>(
        vulkanContext.get(),
        "shaders/simple_shader.vert.spv",
        "shaders/simple_shader.frag.spv",
        pipelineConfig
    );
}

bool Engine::checkCollision(const glm::vec3& pos, const AABB& playerBox, const AABB& obstacle) {
    // Player AABB at new position 'pos'
    // Assuming player is 1x1x1 centered at bottom (0.5 extents)
    // Actually player origin is center so bounds are pos +/- 0.5
    
    glm::vec3 pMin = pos - glm::vec3(0.5f);
    glm::vec3 pMax = pos + glm::vec3(0.5f);
    
    // Check overlap
    bool xOverlap = pMin.x <= obstacle.max.x && pMax.x >= obstacle.min.x;
    bool yOverlap = pMin.y <= obstacle.max.y && pMax.y >= obstacle.min.y;
    bool zOverlap = pMin.z <= obstacle.max.z && pMax.z >= obstacle.min.z;
    
    return xOverlap && yOverlap && zOverlap;
}

void Engine::processInput() {
    if (currentState == GameState::MAIN_MENU) {
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            currentState = GameState::PLAYING;
            restartLevel(); // Force fresh start
        }
        return;
    }

    if (currentState == GameState::VICTORY) {
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            currentState = GameState::MAIN_MENU;
        }
        return;
    }

    if (currentState == GameState::GAME_OVER) {
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            restartLevel();
        }
        return; // Don't move while game over
    }

    float speed = 0.05f; 
    
    float yawRad = glm::radians(cameraYaw);
    glm::vec3 forwardDir = glm::normalize(glm::vec3(-sin(yawRad), 0.0f, -cos(yawRad)));
    glm::vec3 rightDir   = glm::normalize(glm::vec3(cos(yawRad), 0.0f, -sin(yawRad)));

    glm::vec3 moveDir{0.0f};

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += forwardDir;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= forwardDir;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir -= rightDir; 
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir += rightDir; 

    // PHYSICS & MOVEMENT
    float gravity = 0.005f;
    float jumpForce = 0.15f;
    
    // Apply Friction to knockback
    if (glm::length(playerKnockback) > 0.001f) {
        playerKnockback *= 0.92f; // Decay
    } else {
        playerKnockback = glm::vec3(0.0f);
    }

    // Combine player movement and knockback
    glm::vec3 finalMove = moveDir * speed + playerKnockback;

    // Apply XZ Movement
    if (glm::length(finalMove) > 0.0f) {
        glm::vec3 nextPos = playerPosition + finalMove;
        nextPos.y = playerPosition.y; // Preserve Y for now

        bool collided = false;
        AABB pBox{{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
        for (const auto& obs : obstacles) {
            if (checkCollision(nextPos, pBox, obs)) {
                collided = true;
                break;
            }
        }
        if (!collided) {
            playerPosition.x = nextPos.x;
            playerPosition.z = nextPos.z;
        }
    }

    // Apply Boundaries
    if (playerPosition.x < minX) playerPosition.x = minX;
    if (playerPosition.x > maxX) playerPosition.x = maxX;
    if (playerPosition.z < minZ) playerPosition.z = minZ;
    if (playerPosition.z > maxZ) playerPosition.z = maxZ;

    // Apply Gravity / Jump
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded) {
        playerVelocityY = jumpForce;
        isGrounded = false;
    }

    playerVelocityY -= gravity;
    float nextY = playerPosition.y + playerVelocityY;
    
    // Ground Collision (Y Plane at 0.0)
    // Player Half-Height is 0.5. So simple ground check is y < 0.5
    if (nextY < 0.5f) {
        nextY = 0.5f;
        playerVelocityY = 0.0f;
        isGrounded = true;
    } else {
        isGrounded = false;
        
        // Simple Obstacle Collision for Y?
        // If falling onto an obstacle...
        AABB pBox{{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
        glm::vec3 testPos = playerPosition;
        testPos.y = nextY;
        
        for (const auto& obs : obstacles) {
            if (checkCollision(testPos, pBox, obs)) {
                // Collision detected on Y axis change
                // Determine if landing on top or hitting head
                if (playerVelocityY < 0.0f) {
                    // Landing on top
                     // Approximate: Set Y to box max + 0.5
                     // obs.max.y + 0.5?
                     // Let's just stop for now
                     // But we need to distinguish Y collision from XZ collision
                }
                // Revert Y change (simple response)
                nextY = playerPosition.y; 
                playerVelocityY = 0.0f;
                // Ideally check normal, but AABB:
                // If we were above before...
                 if (playerPosition.y >= obs.max.y + 0.5f - 0.01f) {
                     nextY = obs.max.y + 0.5f + 0.001f; // Add epsilon to be cleanly "above"
                     isGrounded = true;
                     playerVelocityY = 0.0f; // Stop falling
                 }
                break;
            }
        }
    }
    
    playerPosition.y = nextY;

    // Enemy Update (Movement & Damage)
    AABB pBox{{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
    float enemySpeed = 0.02f;

    for (size_t i = 0; i < enemies.size(); i++) {
        auto& enemy = enemies[i];
        if (enemy.type == 'F') {
            // Follower Logic: check LOS
            if (hasLineOfSight(enemy.position, playerPosition)) {
                glm::vec3 toPlayer = playerPosition - enemy.position;
                toPlayer.y = 0.0f; // Only move on XZ
                if (glm::length(toPlayer) > 0.1f) {
                    glm::vec3 nextEnemyPos = enemy.position + glm::normalize(toPlayer) * enemySpeed;
                    
                    // Collision check for enemy
                    bool enemyCollided = false;
                    for (const auto& obs : obstacles) {
                        if (nextEnemyPos.x + 0.5f > obs.min.x && nextEnemyPos.x - 0.5f < obs.max.x &&
                            nextEnemyPos.z + 0.5f > obs.min.z && nextEnemyPos.z - 0.5f < obs.max.z) {
                            enemyCollided = true;
                            break;
                        }
                    }

                    // Enemy-Enemy Collision
                    if (!enemyCollided) {
                        for (size_t j = 0; j < enemies.size(); j++) {
                            if (i == j) continue;
                            const auto& other = enemies[j];
                            float dist = glm::distance(nextEnemyPos, other.position);
                            if (dist < 0.8f) { // Slightly less than 1.0 to avoid sticking
                                enemyCollided = true;
                                break;
                            }
                        }
                    }

                    if (!enemyCollided) {
                        enemy.position = nextEnemyPos;
                        enemy.box.min = enemy.position - glm::vec3(0.5f, 0.5f, 0.5f);
                        enemy.box.max = enemy.position + glm::vec3(0.5f, 0.5f, 0.5f);
                    }
                }
            }
        }

        // Damage Check
        if (checkCollision(playerPosition, pBox, enemy.box)) {
            takeDamage(0.5f, enemy.position); // Pass position for knockback
        }
    }

    // Check Exit Collision
    for (const auto& exit : exits) {
        if (checkCollision(playerPosition, pBox, exit)) {
            std::cout << "Fase completada! Carregando próxima fase...\n";
            currentLevelIndex++;
            loadLevel(currentLevelIndex);
            break;
        }
    }

    // Mouse Input
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - lastMouseX);
    float yoffset = static_cast<float>(lastMouseY - ypos);
    
    lastMouseX = xpos;
    lastMouseY = ypos;

    float sensitivity = 0.3f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    cameraYaw   += xoffset;
    cameraPitch += yoffset;

    // Constrain Pitch
    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;
}

void Engine::recordCommandBuffer(VkCommandBuffer buffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Falha ao iniciar gravacao do command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapchain->getRenderPass();
    renderPassInfo.framebuffer = swapchain->getFramebuffers()[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchain->getExtent();

    VkClearValue clearValues[2];
    if (currentState == GameState::MAIN_MENU) {
        clearValues[0].color = {{0.0f, 0.2f, 0.4f, 1.0f}}; 
    } else if (currentState == GameState::GAME_OVER) {
        clearValues[0].color = {{0.5f, 0.0f, 0.0f, 1.0f}}; 
    } else if (currentState == GameState::VICTORY) {
        clearValues[0].color = {{0.5f, 0.5f, 0.0f, 1.0f}}; 
    } else {
        clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}}; 
    }
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain->getExtent().width);
    viewport.height = static_cast<float>(swapchain->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain->getExtent();
    vkCmdSetScissor(buffer, 0, 1, &scissor);

    pipeline->bind(buffer);
    
    updateCamera();
    
    glm::mat4 projectionView = camera->getProjection() * camera->getView();

    if (groundMesh) {
        glm::mat4 model = glm::mat4(1.0f); 
        glm::mat4 push = projectionView * model;
        
        vkCmdPushConstants(buffer, pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &push);
        groundMesh->bind(buffer);
        groundMesh->draw(buffer);
    }

    if (playerMesh) {
        // Draw Player
        // Remove magic -0.5f offset, treat playerPosition as Center
        glm::mat4 model = glm::translate(glm::mat4(1.0f), playerPosition);   
        glm::mat4 push = projectionView * model;

        vkCmdPushConstants(buffer, pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &push);
        playerMesh->bind(buffer);
        playerMesh->draw(buffer);
    }
        
    if (obstacleMesh) {
        obstacleMesh->bind(buffer);
        // Draw Obstacles
        for (const auto& obs : obstacles) {
            // Calculate center from min/max
            glm::vec3 center = (obs.min + obs.max) * 0.5f;
            // Assuming 1x1x1 box mesh, scale it? bounds is 1 unit size?
            // mesh is 1 width (-0.5 to 0.5).
            // obstacle is {2, -1, 2} to {3, 0, 3} -> Size 1, Center 2.5, -0.5, 2.5
            glm::mat4 obsModel = glm::translate(glm::mat4(1.0f), center);
            glm::mat4 obsPush = projectionView * obsModel;
            
            vkCmdPushConstants(buffer, pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &obsPush);
            obstacleMesh->draw(buffer);
        }
    }

    if (exitMesh) {
        exitMesh->bind(buffer);
        for (const auto& exit : exits) {
            glm::vec3 center = (exit.min + exit.max) * 0.5f;
            glm::mat4 exitModel = glm::translate(glm::mat4(1.0f), center);
            glm::mat4 exitPush = projectionView * exitModel;
            
            vkCmdPushConstants(buffer, pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &exitPush);
            exitMesh->draw(buffer);
        }
    }

    if (enemyMesh) {
        enemyMesh->bind(buffer);
        for (const auto& enemy : enemies) {
            if (enemy.type == 'X') {
                glm::mat4 enemyModel = glm::translate(glm::mat4(1.0f), enemy.position);
                glm::mat4 enemyPush = projectionView * enemyModel;
                vkCmdPushConstants(buffer, pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &enemyPush);
                enemyMesh->draw(buffer);
            }
        }
    }

    if (followerMesh) {
        followerMesh->bind(buffer);
        for (const auto& enemy : enemies) {
            if (enemy.type == 'F') {
                glm::mat4 enemyModel = glm::translate(glm::mat4(1.0f), enemy.position);
                glm::mat4 enemyPush = projectionView * enemyModel;
                vkCmdPushConstants(buffer, pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &enemyPush);
                followerMesh->draw(buffer);
            }
        }
    }

    vkCmdEndRenderPass(buffer);

    if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
        throw std::runtime_error("Falha ao finalizar gravacao do command buffer!");
    }
}


void Engine::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, windowTitle.c_str(), nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar janela GLFW\n";
        glfwTerminate();
        exit(1);
    }
    
    // Capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Engine::run() {
    if (!isInitialized) return;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        processInput();
        
        drawFrame();
    }
    vkDeviceWaitIdle(vulkanContext->getDevice());
}


void Engine::createCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vulkanContext->getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(vulkanContext->getDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Falha ao alocar command buffers!");
    }
}

void Engine::drawFrame() {
    vkb::Swapchain vkbSwapchain = swapchain->getSwapchain();
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(vulkanContext->getDevice(), vkbSwapchain.swapchain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return;
    }

    vkResetCommandBuffer(commandBuffer, 0);
    recordCommandBuffer(commandBuffer, imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(vulkanContext->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    
    vkQueueWaitIdle(vulkanContext->getGraphicsQueue());

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vkbSwapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(vulkanContext->getGraphicsQueue(), &presentInfo);
}

void Engine::updateCamera() {
    float aspectRatio = swapchain->getExtent().width / (float)swapchain->getExtent().height;
    camera->setPerspectiveProjection(glm::radians(50.0f), aspectRatio, 0.1f, 100.0f);
    
    // Calculate Camera Offset based on Yaw/Pitch (Spherical to Cartesian)
    // Yaw 0 = +Z, Yaw 90 = +X
    float yawRad = glm::radians(cameraYaw);
    float pitchRad = glm::radians(cameraPitch);

    // Standard Math:
    // x = r * cos(pitch) * sin(yaw)
    // y = r * sin(pitch)
    // z = r * cos(pitch) * cos(yaw)
    
    // Adjusting for our coordinate system where -Y is UP (if applicable) or +Y is UP.
    // Let's assume +Y is DOWN (Vulkan) but we want world-space Y-UP logic?
    // In previous steps, ground is at Y=0. Objects are on it.
    // If I used (0, -2, 4) to look at (0,0,0) and that worked as "Up and Behind":
    // -2 Y is "Up". So we have a Y-Up world (or Y-Down camera logic inverting it).
    // Let's stick to -Y is Up for world space for now.
    
    float hDistance = cameraDistance * cos(pitchRad);
    float vDistance = cameraDistance * sin(pitchRad); 
    
    float offsetX = hDistance * sin(yawRad);
    float offsetZ = hDistance * cos(yawRad);
    float offsetY = vDistance; // Positive Y is Up now
    
    // If Pitch is positive (looking down), we want camera to be "up" (negative Y).
    // So if Pitch=20, vDistance is +; offsetY becomes - (Up). Correct.
    
    glm::vec3 cameraOffset = {offsetX, offsetY, offsetZ};
    glm::vec3 target = playerPosition;
    glm::vec3 position = target + cameraOffset;
    
    camera->setViewTarget(position, target);
}

void Engine::cleanup() {
    if (isInitialized) {
        vkDeviceWaitIdle(vulkanContext->getDevice());
        
        if (swapchain) {
            swapchain->cleanup();
        }

        obstacleMesh.reset();
        enemyMesh.reset();
        followerMesh.reset();
        exitMesh.reset();
        pipeline.reset(); 
        camera.reset();
        groundMesh.reset();
        playerMesh.reset();
        vulkanContext->cleanup(); // Destroys allocator
        
        glfwDestroyWindow(window);
        glfwTerminate();
        isInitialized = false;
    }
}
