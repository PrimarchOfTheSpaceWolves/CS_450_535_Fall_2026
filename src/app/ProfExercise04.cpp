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

        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            if(didWindowResize) {
                cout << "Resized!" << endl;
                vkCore.doWindowResize();
                didWindowResize = false;
            }

            uint32_t flightIndex = framesRendered % numberFramesInFlight;
            uint32_t swapIndex = pro::acquireNextSwapImage(vkCore, frameCmd);

            // TODO

            pro::submitForFrame(vkCore, frameCmd, swapIndex);

            if(!pro::presentSwapImage(vkCore, swapIndex)) {
                cerr << "Warning: Present failed." << endl;
            }

            framesRendered++;
        }
        
        vkCore.device().waitIdle();
    } // Cleanup starts here

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
