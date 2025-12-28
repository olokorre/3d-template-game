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
#include <stdexcept>

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

    // 2. Player Mesh (Cube)
    // Simplified cube (just front/back/left/right/top/bottom faces? actually just a few faces for now to prove it)
    // Let's do a simple box
    std::vector<Vertex> boxVertices;
    
    // Helper to add quad
    auto addQuad = [&](glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, glm::vec3 color) {
        boxVertices.push_back({p1, {0,1,0}, color});
        boxVertices.push_back({p2, {0,1,0}, color});
        boxVertices.push_back({p3, {0,1,0}, color});
        boxVertices.push_back({p3, {0,1,0}, color});
        boxVertices.push_back({p2, {0,1,0}, color});
        boxVertices.push_back({p4, {0,1,0}, color});
    };

    glm::vec3 c = {1.0f, 0.0f, 0.0f}; // Red
    // Front face
    addQuad({-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, c);
    // Back face
    addQuad({0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, c);
    
    playerMesh = std::make_unique<Mesh>(vulkanContext.get(), boxVertices);
}


void Engine::createPipeline() {
    PipelineConfigInfo pipelineConfig{};
    Pipeline::defaultPipelineConfigInfo(pipelineConfig);
    
    pipelineConfig.renderPass = swapchain->getRenderPass();
    pipelineConfig.pipelineLayout = VK_NULL_HANDLE; // TODO: Criar layout para push constants

    // Create simple layout just for now to avoid validation errors if strictly checked
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // Push constant range for simple vertex shader
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4); // renderMatrix
    
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    // Vazando pipeline layout por enquanto (cleanup necessario), ou mover para Pipeline class
    VkPipelineLayout layout;
    vkCreatePipelineLayout(vulkanContext->getDevice(), &pipelineLayoutInfo, nullptr, &layout);
    pipelineConfig.pipelineLayout = layout;

    pipeline = std::make_unique<Pipeline>(
        vulkanContext.get(),
        "shaders/simple_shader.vert.spv",
        "shaders/simple_shader.frag.spv",
        pipelineConfig
    );
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

void Engine::processInput() {
    float speed = 0.05f; // Simple constant speed for now (TODO: DeltaTime)
    
    // Calculate Forward and Right vectors based on Camera Yaw
    // Yaw 0 = Camera at +Z Looking -Z. Forward should be -Z.
    float yawRad = glm::radians(cameraYaw);
    glm::vec3 forwardDir = glm::normalize(glm::vec3(-sin(yawRad), 0.0f, -cos(yawRad)));
    glm::vec3 rightDir   = glm::normalize(glm::vec3(cos(yawRad), 0.0f, -sin(yawRad)));

    glm::vec3 moveDir{0.0f};

    // Forward / Backward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += forwardDir;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= forwardDir;
    
    // Left / Right
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += rightDir;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= rightDir;

    if (glm::length(moveDir) > 0.0f) {
        moveDir = glm::normalize(moveDir);
        playerPosition += moveDir * speed;
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
    float yoffset = static_cast<float>(lastMouseY - ypos); // Reversed since y-coordinates go from bottom to top check
    
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

void Engine::createCommandBuffer() {
// ... existing createCommandBuffer implementation ... 
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
    clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}}; 
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
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(playerPosition.x, playerPosition.y - 0.5f, playerPosition.z)); 
        glm::mat4 push = projectionView * model;

        vkCmdPushConstants(buffer, pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &push);
        playerMesh->bind(buffer);
        playerMesh->draw(buffer);
    }

    vkCmdEndRenderPass(buffer);

    if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
        throw std::runtime_error("Falha ao finalizar gravacao do command buffer!");
    }
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
    float offsetY = -vDistance; // Negative because -Y is Up
    
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

        groundMesh.reset();
        playerMesh.reset();
        pipeline.reset(); 

        vulkanContext->cleanup(); // Destroys allocator
        
        glfwDestroyWindow(window);
        glfwTerminate();
        isInitialized = false;
    }
}
