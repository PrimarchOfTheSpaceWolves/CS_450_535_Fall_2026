#pragma once
#include "ProBase.hpp"

namespace pro { 
    ///////////////////////////////////////////////////////////////////////////
    // FUNCTION POINTERS 
    ///////////////////////////////////////////////////////////////////////////

    using GetCurrentWindowSizeFunc = std::function<void(int&, int&)>;
    
    ///////////////////////////////////////////////////////////////////////////
    // STRUCTS
    ///////////////////////////////////////////////////////////////////////////
        
    struct VulkanSwapImage {
        vk::Image image {};         // Non-owning handle, since it gets destroyed when swapchain dies.
        vk::raii::ImageView view = nullptr;
        vk::raii::Semaphore renderDone = nullptr;
    };

    struct VulkanSwapChain {
        vk::raii::SwapchainKHR chain = nullptr;
        vector<VulkanSwapImage> swaps {};        
        vk::Extent2D extent {};
        vk::Format format {};
    };

    ///////////////////////////////////////////////////////////////////////////
    // HELPER FUNCTIONS
    ///////////////////////////////////////////////////////////////////////////

    inline bool checkSwapSurfaceFormat( const vk::raii::PhysicalDevice &physicalDevice,
                                        const vk::raii::SurfaceKHR &surface,
                                        vk::SurfaceFormatKHR &desiredFormat) {
        // Get all available formats
        auto availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);

        for(auto supported : availableFormats) {
            if(supported.format == desiredFormat.format && supported.colorSpace == desiredFormat.colorSpace) {
                // FOUND IT!
                return true;
            }
        }
        
        return false;
    };

    inline vk::PresentModeKHR chooseSwapPresentMode(const vk::raii::PhysicalDevice &physicalDevice,
                                                    const vk::raii::SurfaceKHR &surface) {
        // Get supported list
        auto availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

        // Looking for VK_PRESENT_MODE_MAILBOX_KHR;
        // failing that, use VK_PRESENT_MODE_FIFO_KHR (always guaranteed)
        vk::PresentModeKHR chosenPresent = vk::PresentModeKHR::eFifo;

        for(auto supported : availablePresentModes) {
            if(supported == vk::PresentModeKHR::eMailbox) {
                chosenPresent = vk::PresentModeKHR::eMailbox;
                break;
            }
        }

        return chosenPresent;
    };

    // Partially adapted from: https://docs.vulkan.org/tutorial/latest
    inline vk::Extent2D chooseSwapExtent(   const vk::raii::PhysicalDevice &physicalDevice,
                                            const vk::raii::SurfaceKHR &surface,
                                            GetCurrentWindowSizeFunc &getCurrentWindowSizeFunc) {
        // Get surface capabilities
        auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);

        // If it isn't the weird max value thing...
        if (surfaceCapabilities.currentExtent.width != numeric_limits<uint32_t>::max()) {
            // ...just return what we have
            return surfaceCapabilities.currentExtent;
        }

        // Otherwise, we have to get the actual pixel size of the window...
        int width, height;
        getCurrentWindowSizeFunc(width, height);

        return {
            std::clamp<uint32_t>(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
        };
    };

    // Partially adapted from: https://docs.vulkan.org/tutorial/latest
    uint32_t chooseSwapMinImageCount(   const vk::raii::PhysicalDevice &physicalDevice,
                                        const vk::raii::SurfaceKHR &surface) {

        // Get surface capabilities
        auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);

        // Get the minimum number of images (but try to get at least 3)
        auto minImageCount = max(3u, surfaceCapabilities.minImageCount);

        // Make sure we're not over the maximum number of images
        // (noting that 0 means unlimited)
        if ((0 < surfaceCapabilities.maxImageCount) 
            && (surfaceCapabilities.maxImageCount < minImageCount)) {
    
            minImageCount = surfaceCapabilities.maxImageCount;
        }

        return minImageCount;
    };
}
