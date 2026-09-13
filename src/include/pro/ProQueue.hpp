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
        unsigned int familyIndex = 0;
        unsigned int queueIndex = 0;
        
        VulkanQueue() {};
        
        VulkanQueue(const vk::raii::Device &device, 
                    unsigned int familyIndex, 
                    unsigned int queueIndex = 0) {

            this->queue = vk::raii::Queue(device, familyIndex, queueIndex);
            this->familyIndex = familyIndex;
            this->queueIndex = queueIndex;
        }; 

        bool fromSameFamily(const VulkanQueue &other) const {
            return familyIndex == other.familyIndex;
        };

        friend std::ostream& operator<<(std::ostream& os, const VulkanQueue &queue) {            
            return os << "{ family = " << queue.familyIndex
                        << ", queue = " << queue.queueIndex << " }";                  
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
    
    inline int findQueueFamilyIndex(    vk::raii::PhysicalDevice &physicalDevice,
                                        vk::raii::SurfaceKHR &surface,
                                        const vector<vk::QueueFlagBits> &desiredFlags,
                                        const vector<vk::QueueFlagBits> &avoidFlags,
                                        bool checkForPresent = false) {

        // Get queue family properties
        auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        // Cycle through families to find queue matching description
        int queueFamilyIndex = INVALID_QUEUE;
        //for(auto family : queueFamilies) {
        for(int i = 0; i < queueFamilies.size(); i++) {
            auto family = queueFamilies[i];
            if(queueFamilyHasFlags(family, desiredFlags)
                && queueFamilyAvoidsFlags(family, avoidFlags)
                && (!checkForPresent || physicalDevice.getSurfaceSupportKHR(i, *surface))) {
                // MATCH!
                queueFamilyIndex = i;
                break;
            }
        }

        // Return what we got (which might be -1)
        return queueFamilyIndex;
    };

    inline bool checkForQueueFamily(    vk::raii::PhysicalDevice &physicalDevice,
                                        vk::raii::SurfaceKHR &surface,
                                        const vector<vk::QueueFlagBits> &desiredFlags,
                                        const vector<vk::QueueFlagBits> &avoidFlags,
                                        bool checkForPresent = false) {
        return (findQueueFamilyIndex(physicalDevice, surface, desiredFlags, avoidFlags, checkForPresent)
                 != INVALID_QUEUE);
    };

    inline uint32_t getQueueCount(  vk::raii::PhysicalDevice &physicalDevice,
                                    int queueFamilyIndex) {

        // Get queue family properties
        auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        // Is this out of bounds?
        if(queueFamilyIndex < 0 || queueFamilyIndex >= queueFamilies.size()) {
            print_and_throw_error("getQueueCount", "Invalid queue family index: " + queueFamilyIndex);
        }

        // Get the specific queue family
        auto family = queueFamilies[queueFamilyIndex];

        // Return the number of queues
        return family.queueCount;
    };
}
