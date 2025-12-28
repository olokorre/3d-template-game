#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class VulkanContext;

struct PipelineConfigInfo {
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
    std::vector<VkDynamicState> dynamicStateEnables;
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
};

class Pipeline {
public:
    Pipeline(VulkanContext* context, const std::string& vertPath, const std::string& fragPath, const PipelineConfigInfo& configInfo);
    ~Pipeline();

    void bind(VkCommandBuffer commandBuffer);

    static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    
private:
    VkShaderModule createShaderModule(const std::vector<char>& code);
    static std::vector<char> readFile(const std::string& filepath);

    VulkanContext* context;
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout; // Store layout
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
};
