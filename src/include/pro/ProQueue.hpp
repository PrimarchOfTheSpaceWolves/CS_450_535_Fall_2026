#pragma once
#include "ProBase.hpp"

namespace pro { 

    ///////////////////////////////////////////////////////////////////////////
    // CONSTANTS 
    ///////////////////////////////////////////////////////////////////////////

    constexpr int INVALID_QUEUE = -1;

    ///////////////////////////////////////////////////////////////////////////
    // STRUCTS 
    ///////////////////////////////////////////////////////////////////////////

    struct VulkanQueue {
        vk::raii::Queue queue = nullptr;
        unsigned int index = 0;
        bool is_valid = false;

        VulkanQueue() {};
        
        VulkanQueue(const vk::raii::Device &device, unsigned int familyIndex) {
            queue = vk::raii::Queue(device, familyIndex, 0);
            index = familyIndex;
            is_valid = true;
        }; 
    };
      
    ///////////////////////////////////////////////////////////////////////////
    // HELPER FUNCTIONS
    ///////////////////////////////////////////////////////////////////////////    

    inline bool queueFamilyHasFlags(const vk::QueueFamilyProperties &family,
                                    const vector<vk::QueueFlagBits> &desiredFlags) {
        // Get family flags
        auto flags = family.queueFlags;
        for(auto desired : desiredFlags) {
            if(!(flags & desired)) {
                // Does NOT have this desired flag
                return false;
            }
        }
        // Survived
        return true;
    };

    inline bool queueFamilyAvoidsFlags( const vk::QueueFamilyProperties &family,
                                        const vector<vk::QueueFlagBits> &avoidFlags) {
        // Get family flags
        auto flags = family.queueFlags;
        for(auto avoid : avoidFlags) {
            if(flags & avoid) {
                // HAS undesirable flag
                return false;
            }
        }
        // Survived
        return true;
    };
    
    inline int findQueueIndex(  vk::raii::PhysicalDevice &physicalDevice,
                                vk::raii::SurfaceKHR &surface,
                                const vector<vk::QueueFlagBits> &desiredFlags,
                                const vector<vk::QueueFlagBits> &avoidFlags,
                                bool checkForPresent = false) {

        // Get queue family properties
        auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        // Cycle through families to find queue matching description
        int queueIndex = INVALID_QUEUE;
        //for(auto family : queueFamilies) {
        for(int i = 0; i < queueFamilies.size(); i++) {
            auto family = queueFamilies[i];
            if(queueFamilyHasFlags(family, desiredFlags)
                && queueFamilyAvoidsFlags(family, avoidFlags)
                && (!checkForPresent || physicalDevice.getSurfaceSupportKHR(i, *surface))) {
                // MATCH!
                queueIndex = i;
                break;
            }
        }

        // Return what we got (which might be -1)
        return queueIndex;
    };

    inline bool checkForQueue(  vk::raii::PhysicalDevice &physicalDevice,
                                vk::raii::SurfaceKHR &surface,
                                const vector<vk::QueueFlagBits> &desiredFlags,
                                const vector<vk::QueueFlagBits> &avoidFlags,
                                bool checkForPresent = false) {
        return (findQueueIndex(physicalDevice, surface, desiredFlags, avoidFlags, checkForPresent)
                 != INVALID_QUEUE);
    };
}
