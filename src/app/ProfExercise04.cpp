#include <iostream>
#include <string>
#define PRO_USE_GLFW
#include "pro/Prometheus.hpp"
using namespace std;

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

    string appName = "ProfExercise04";
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

            // TODO: Drawing commands

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
            cout << results[0] << " " << results[1] << endl;

        }
        
        vkCore.device().waitIdle();
    } // Cleanup starts here

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
