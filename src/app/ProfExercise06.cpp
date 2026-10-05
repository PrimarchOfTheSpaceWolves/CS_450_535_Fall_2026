#include <iostream>
#include <string>
#define PRO_USE_GLFW
#include "pro/Prometheus.hpp"
using namespace std;

struct ForgeVertex {
    glm::vec3 pos;
};

bool didWindowResize = false;

static void window_resize_callback( GLFWwindow *window, 
                                    int width, int height) {
    didWindowResize = true;
}

int main(int argc, char **argv) {
    cout << "BEGIN VULKAN EXERCISE" << endl;

    if(!glfwInit()) {
        cerr << "Error: GLFW did not init!" << endl;
        exit(1);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, true);

    string appName = "ProfExercise06";
    string windowTitle = appName + ": realemj";
    GLFWwindow *window = glfwCreateWindow(800, 600, windowTitle.c_str(),
                                            nullptr, nullptr);

    if(!window) {
        cerr << "Error: Could not create window!" << endl;
        glfwTerminate();
        exit(1);
    }

    glfwSetFramebufferSizeCallback(window, window_resize_callback);

    {
        pro::VulkanCoreCreateInfo coreCreateInfo {};
        coreCreateInfo.appName = appName;

        pro::prepareVulkanInitGLFWFunctions(coreCreateInfo, window);

        pro::VulkanCore vkCore(coreCreateInfo);

        pro::CommandData frameCmd = pro::createFrameCommandData(vkCore);

        pro::VulkanPipelineCreateInfo pipelineCreateInfo(vkCore);

        pipelineCreateInfo.shaderInfo = {
            pro::VulkanShaderCreateInfo(
                "build/compiledshaders/" + appName + "/shader.vert.spv",
                vk::ShaderStageFlagBits::eVertex
            ),
            pro::VulkanShaderCreateInfo(
                "build/compiledshaders/" + appName + "/shader.frag.spv",
                vk::ShaderStageFlagBits::eFragment
            )
        };

        pipelineCreateInfo.bindDesc = vk::VertexInputBindingDescription(
            0, sizeof(ForgeVertex), vk::VertexInputRate::eVertex
        );

        pipelineCreateInfo.attribDesc = {
            vk::VertexInputAttributeDescription(
                0, // location
                0, // binding
                vk::Format::eR32G32B32Sfloat, // (x,y,z)
                offsetof(ForgeVertex, pos)
            )
        };

        pro::VulkanPipelineData pipelineData
         = pro::createVulkanPipeline(vkCore.device(), pipelineCreateInfo);

        uint32_t framesRendered = 0;
        int numberFramesInFlight = 1;

        vk::QueryPoolCreateInfo qi {};
        qi.queryType = vk::QueryType::eTimestamp;
        qi.queryCount = 2;
        auto qPool = vk::raii::QueryPool(vkCore.device(), qi);

        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            if(didWindowResize) {
                cout << "Resized!" << endl;
                vkCore.doWindowResize();
                didWindowResize = false;
            }

            uint32_t flightIndex = framesRendered % numberFramesInFlight;
            uint32_t swapIndex = pro::acquireNextSwapImage(vkCore, frameCmd);
            frameCmd.beginRecording();
            frameCmd.buffer().resetQueryPool(qPool, 0, 2);
            frameCmd.buffer().writeTimestamp2(
                vk::PipelineStageFlagBits2::eTopOfPipe, qPool, 0);

            pro::performImageTransition(
                frameCmd.buffer(),
                vkCore.swapchain().swaps[swapIndex].image,
                pro::IMAGE_STATE_TYPE::UNDEF,
                pro::IMAGE_STATE_TYPE::COLOR
            );

            auto colorAtt = pro::createColorAttachment(
                vkCore.swapchain().swaps[swapIndex].view,
                {1.0f, 1.0f, 0.0f, 1.0f}
            );

            vk::RenderingInfoKHR ri {};
            ri.setRenderArea(vk::Rect2D({0,0}, vkCore.swapchain().extent));
            ri.setLayerCount(1);
            ri.setColorAttachments(colorAtt);
            frameCmd.buffer().beginRendering(ri);
            frameCmd.buffer().bindPipeline(
                vk::PipelineBindPoint::eGraphics,
                pipelineData.pipeline
            );

            vk::Viewport vps[] = { pro::makeDefaultViewport(vkCore) };
            frameCmd.buffer().setViewport(0, vps);

            vk::Rect2D scs[] = { pro::makeDefaultScissors(vkCore) };
            frameCmd.buffer().setScissor(0, scs);

            // TODO: Drawing commands

            frameCmd.buffer().endRendering();

            pro::performImageTransition(
                frameCmd.buffer(),
                vkCore.swapchain().swaps[swapIndex].image,
                pro::IMAGE_STATE_TYPE::COLOR,
                pro::IMAGE_STATE_TYPE::PRESENT
            );

            frameCmd.buffer().writeTimestamp2(
                vk::PipelineStageFlagBits2::eBottomOfPipe, qPool, 1);

            frameCmd.endRecording();
            pro::submitForFrame(vkCore, frameCmd, swapIndex);

            if(!pro::presentSwapImage(vkCore, swapIndex)) {
                cerr << "Warning: Present failed." << endl;
            }

            framesRendered++;

            auto poolResult = qPool.getResults<uint64_t>(
                0, 2, 2*sizeof(uint64_t), sizeof(uint64_t),
                vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait
            );
            vector<uint64_t> results = poolResult.value;
            auto props = vkCore.physicalDevice().getProperties();
            double nsPerTick = props.limits.timestampPeriod;
            double timePassed = (results[1] - results[0])*nsPerTick;
            //cout << "TIME PASSED: " << timePassed << endl;
        }
        
        vkCore.device().waitIdle();
    } // Cleanup starts here

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
