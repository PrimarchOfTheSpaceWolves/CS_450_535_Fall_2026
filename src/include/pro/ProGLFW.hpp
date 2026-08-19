#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "ProCore.hpp"

namespace pro {    
    ///////////////////////////////////////////////////////////////////////////
    // HELPER FUNCTIONS
    ///////////////////////////////////////////////////////////////////////////

    inline void prepareVulkanInitGLFWFunctions(
        pro::VulkanCoreCreateInfo &createInfo,
        GLFWwindow *window) {

        createInfo.createSurfaceFunc = [window](VkInstance instance, VkSurfaceKHR& surface) {            
            return glfwCreateWindowSurface(instance, window, nullptr, &surface);
        };

        createInfo.waitUntilWindowEventFunc = []() {
            glfwWaitEvents();
        };

        createInfo.getCurrentWindowSizeFunc = [window](int &width, int &height) {
            glfwGetFramebufferSize(window, &width, &height);
        };

        createInfo.getWindowExtensionsFunc = [window](const vk::raii::Context &context) {
            // Taken from: https://docs.vulkan.org/tutorial/latest
            uint32_t glfwExtensionCount = 0;
            auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            vector<string> extensions(glfwExtensionCount);        
            for(int i = 0; i < glfwExtensionCount; i++) {
                extensions[i] = glfwExtensions[i];
            }
            return extensions;
        };
    };

};

