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
    cout << "BEGIN GLM EXERCISE" << endl;
    glm::vec3 a = {1,4,0};
    glm::vec3 b = {2,3,2};
    cout << "a.x/r: " << a.x << " " << a.r << endl;
    cout << "a: " << glm::to_string(a) << endl;
    cout << "b: " << glm::to_string(b) << endl;
    glm::vec3 c = b - a;
    cout << "c: " << glm::to_string(c) << endl;

    float alen = glm::length(a);
    cout << "Length a: " << alen << endl;

    a = 5.0f*a;
    alen = glm::length(a);
    cout << "New a: " << glm::to_string(a) << endl;
    cout << "New length a: " << alen << endl;
    
    glm::vec3 normA = glm::normalize(a);
    float anormlen = glm::length(normA);
    cout << "normA: " << glm::to_string(normA) << endl;
    cout << "normA len: " << anormlen << endl;


    cout << "BEGIN VULKAN EXERCISE" << endl;

    if(!glfwInit()) {
        cerr << "Error: GLFW did not init!" << endl;
        exit(1);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, true);

    string appName = "ProfExercise07";
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

        vector<pro::HostMesh<ForgeVertex>> allHostMeshes {};
        pro::HostMesh<ForgeVertex> hostMesh {};
        hostMesh.vertices = {
            {{-0.5f, -0.5f, 0.5f}},
            {{0.5f, -0.5f, 0.5f}},
            {{0.5f, 0.5f, 0.5f}},
            {{-0.5f, 0.5f, 0.5f}}
        };
        hostMesh.indices = {
            0,2,3,
            2,0,1
        };
        allHostMeshes.push_back(hostMesh);

        vector<pro::VulkanMesh> allMeshes {};
        allMeshes.resize(allHostMeshes.size());

        // Host-visible version
        for(int i = 0; i < allHostMeshes.size(); i++) {
            allMeshes[i] = pro::createVulkanMesh(
                vkCore.allocator(),
                allHostMeshes[i],
                false
            );
            pro::copyToHostVisibleVulkanMesh(
                vkCore.allocator(),
                allMeshes[i],
                allHostMeshes[i]
            );
        }

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

            for(int i = 0; i < allMeshes.size(); i++) {
                pro::recordDrawVulkanMesh(frameCmd.buffer(), allMeshes[i]);
            }

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
