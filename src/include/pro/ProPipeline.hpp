#pragma once
#include "ProCore.hpp"

namespace pro {

    ///////////////////////////////////////////////////////////////////////////
    // HELPER FUNCTIONS
    ///////////////////////////////////////////////////////////////////////////

    inline vk::Viewport makeDefaultViewport(const VulkanCore &vkCore, 
                                            bool flipViewportY = true) { 
        vk::Viewport viewport {};
        float width = (float)vkCore.swapchain().extent.width;
        float height = (float)vkCore.swapchain().extent.height;

        if(!flipViewportY) {
            // Y still downward
            // Will make problems for winding order
            viewport = vk::Viewport (0, 
                                     0, 
                                     width, 
                                     height, 
                                     0.0f, 1.0f);
        }
        else {
            // Y upward now
            viewport = vk::Viewport (0, 
                                     height,        // Start at bottom
                                     width, 
                                     -height,       // Using NEGATIVE height
                                     0.0f, 1.0f);
        }

        return viewport;
    };

    inline vk::Rect2D makeDefaultScissors(const VulkanCore &vkCore) {
        return vk::Rect2D({0,0}, vkCore.swapchain().extent);
    };

    inline void setNoBlendInfo( vk::PipelineColorBlendAttachmentState &colorBlendAttachment,
                                vk::PipelineColorBlendStateCreateInfo &colorBlendInfo) {
        
        colorBlendAttachment = vk::PipelineColorBlendAttachmentState {};             
        colorBlendAttachment.colorWriteMask = 
            vk::ColorComponentFlagBits::eR 
            | vk::ColorComponentFlagBits::eG 
            | vk::ColorComponentFlagBits::eB 
            | vk::ColorComponentFlagBits::eA;
        colorBlendInfo = vk::PipelineColorBlendStateCreateInfo({}, false, vk::LogicOp::eCopy, colorBlendAttachment);
    };

    inline void setAlphaBlendInfo(  vk::PipelineColorBlendAttachmentState &colorBlendAttachment,
                                    vk::PipelineColorBlendStateCreateInfo &colorBlendInfo) {

        // finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
        // finalColor.a = newAlpha.a;
        // In this case:
        // - src --> new
        // - dst --> old
        colorBlendAttachment = vk::PipelineColorBlendAttachmentState()
                                        .setBlendEnable(true)
                                        .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                                        .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                                        .setColorBlendOp(vk::BlendOp::eAdd)
                                        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                                        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                                        .setAlphaBlendOp(vk::BlendOp::eAdd)
                                        .setColorWriteMask(
                                            vk::ColorComponentFlagBits::eR 
                                            | vk::ColorComponentFlagBits::eG 
                                            | vk::ColorComponentFlagBits::eB 
                                            | vk::ColorComponentFlagBits::eA
                                        );
        colorBlendInfo = vk::PipelineColorBlendStateCreateInfo({}, false, vk::LogicOp::eCopy, colorBlendAttachment);
    };

    ///////////////////////////////////////////////////////////////////////////
    // STRUCTS 
    ///////////////////////////////////////////////////////////////////////////

    struct VulkanShaderCreateInfo {
        string filename {};
        vk::ShaderStageFlagBits stage {};
                
        VulkanShaderCreateInfo(string filename, vk::ShaderStageFlagBits stage) {
            this->filename = filename;
            this->stage = stage;
        };
    };

    struct VulkanPipelineCreateInfo {
        // Shaders
        vector<VulkanShaderCreateInfo> shaderInfo {};
        
        // Vertex data
        vk::VertexInputBindingDescription bindDesc {};
        vector<vk::VertexInputAttributeDescription> attribDesc {};

        // Assembly type
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo {};

        // Dynamic rendering info
        vk::Format colorFormat {};
        vk::PipelineRenderingCreateInfo renderInfo {};

        // Uniform and layout info
        vector<vk::PushConstantRange> pushConstantRanges {};
        vector<vk::DescriptorSetLayout> allDescSetLayouts {}; 
        
        // Viewport and scissors
        vk::Viewport viewport {};
        vk::Rect2D scissor {};

        // Rasterizer info
        vk::PipelineRasterizationStateCreateInfo rasterizerInfo {};        
        
        // Color blend info  
        vk::PipelineColorBlendAttachmentState colorBlendAttachment {};         
        vk::PipelineColorBlendStateCreateInfo colorBlendInfo {};

        // Depth and stencil info
        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo {};

        // MSAA info
        vk::PipelineMultisampleStateCreateInfo multisampleInfo {};
        
        // No-arg constructor
        VulkanPipelineCreateInfo(VulkanCore &vkCore, bool flipViewportY = true) {
            // Assembly defaults to triangle list
            inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList, false);

            // Default rendering info            
            renderInfo.colorAttachmentCount = 1;  
            colorFormat = vkCore.swapchain().format;   
            renderInfo.pColorAttachmentFormats = &colorFormat; 
            renderInfo.depthAttachmentFormat = vk::Format::eD32Sfloat; 
            
            // Default rasterizer options            
            rasterizerInfo.lineWidth = 1.0f;
            rasterizerInfo.cullMode = vk::CullModeFlagBits::eBack;
            rasterizerInfo.frontFace = vk::FrontFace::eCounterClockwise; 

            // Default viewport and scissors
            viewport = makeDefaultViewport(vkCore, flipViewportY);
            scissor = makeDefaultScissors(vkCore);
            
            // Default color blend info (basically no color blending)              
            setNoBlendInfo(colorBlendAttachment, colorBlendInfo);

            // Default depth testing info
            depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo(
                                {},
                                true,                   // Enable depth testing    
                                true,                   // Enable depth writing    
                                vk::CompareOp::eLess,   // Lower depth = closer = keep
                                false,                  // Not putting bounds on depth test
                                false, {}, {}           // Not using stencil test
            );

            // Default MSAA info (basically off)
            multisampleInfo = vk::PipelineMultisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1);         
        };
    };

    struct VulkanPipelineData {
        vk::raii::PipelineCache cache = nullptr;
        vk::raii::PipelineLayout layout = nullptr;
        vk::raii::Pipeline pipeline = nullptr;
    };

    ///////////////////////////////////////////////////////////////////////////
    // FUNCTIONS 
    ///////////////////////////////////////////////////////////////////////////

    inline vector<char> readBinaryFile(const string& filename) {
        ifstream file(filename, ios::ate | ios::binary);

        if (!file.is_open()) {
            print_and_throw_error("readBinaryFile", "Failed to open file: " + filename);            
        }

        size_t fileSize = (size_t) file.tellg();
        vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    inline vk::raii::ShaderModule createVulkanShaderModule( const vk::raii::Device &device, 
                                                            const vector<char>& code) {
        
        return vk::raii::ShaderModule(device, vk::ShaderModuleCreateInfo()
                                                .setCodeSize(code.size())
                                                .setPCode(reinterpret_cast<const uint32_t*>(code.data())));        
    };
    
    inline VulkanPipelineData createVulkanPipeline( const vk::raii::Device &device, 
                                                    VulkanPipelineCreateInfo &creationInfo) {

        // Create data struct
        VulkanPipelineData data {};

        // Shaders
        vector<vk::PipelineShaderStageCreateInfo> shaderStages {};
        vector<vk::raii::ShaderModule> shaderModules {};

        for(auto shaderData : creationInfo.shaderInfo) {
            auto shaderCode = readBinaryFile(shaderData.filename);
            vk::raii::ShaderModule shaderMod = createVulkanShaderModule(device, shaderCode);
            
            vk::PipelineShaderStageCreateInfo shaderStageInfo(
                {}, shaderData.stage, *shaderMod, "main");  
            shaderStages.push_back(shaderStageInfo);

            shaderModules.push_back(move(shaderMod));
        }

        // Vertex information
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
            {}, creationInfo.bindDesc, creationInfo.attribDesc);
        
        // Set viewport and scissor info
        vk::PipelineViewportStateCreateInfo viewportStateInfo({}, creationInfo.viewport, creationInfo.scissor);

        // Set dynamic state info
        vector<vk::DynamicState> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor        
        };
        vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, dynamicStates);  

        // Set all layout info
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
            {}, 
            creationInfo.allDescSetLayouts,
            creationInfo.pushConstantRanges);
        
        // Create the layouts
        data.layout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

        // Create the cache
        data.cache = vk::raii::PipelineCache(device, vk::PipelineCacheCreateInfo());    

        // Create the master info
        vk::GraphicsPipelineCreateInfo pinfo {};
        pinfo.setFlags(vk::PipelineCreateFlags());
        pinfo.setStages(shaderStages);
        pinfo.setPVertexInputState(&vertexInputInfo);
        pinfo.setPInputAssemblyState(&(creationInfo.inputAssemblyInfo));
        pinfo.setPViewportState(&viewportStateInfo);
        pinfo.setPRasterizationState(&(creationInfo.rasterizerInfo));
        pinfo.setPMultisampleState(&(creationInfo.multisampleInfo));
        pinfo.setPDepthStencilState(&(creationInfo.depthStencilInfo));
        pinfo.setPColorBlendState(&(creationInfo.colorBlendInfo));
        pinfo.setPDynamicState(&dynamicStateInfo);
        pinfo.setLayout(data.layout);
        pinfo.setPNext(&(creationInfo.renderInfo));
        pinfo.setRenderPass(nullptr);        
        data.pipeline = vk::raii::Pipeline(device, data.cache, pinfo);
                
        // Return data
        return move(data);
    };      
}
