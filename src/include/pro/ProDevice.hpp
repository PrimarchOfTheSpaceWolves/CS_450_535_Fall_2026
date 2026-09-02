#pragma once
#include "ProQueue.hpp"

namespace pro { 

    ///////////////////////////////////////////////////////////////////////////
    // CONSTANTS AND ALIASES
    ///////////////////////////////////////////////////////////////////////////

    template<typename T> inline constexpr std::size_t featureCnt = 0;

    template<> inline constexpr std::size_t
    featureCnt<vk::PhysicalDeviceFeatures> = 55;

    template<> inline constexpr std::size_t
    featureCnt<vk::PhysicalDeviceVulkan11Features> = 12;

    template<> inline constexpr std::size_t
    featureCnt<vk::PhysicalDeviceVulkan12Features> = 47;

    template<> inline constexpr std::size_t
    featureCnt<vk::PhysicalDeviceVulkan13Features> = 15;

    template<> inline constexpr std::size_t
    featureCnt<vk::PhysicalDeviceVulkan14Features> = 21;

    template<> inline constexpr std::size_t
    featureCnt<vk::PhysicalDeviceAccelerationStructureFeaturesKHR> = 5;

    template<> inline constexpr std::size_t
    featureCnt<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR> = 5;    

    ///////////////////////////////////////////////////////////////////////////
    // STRUCTS
    ///////////////////////////////////////////////////////////////////////////

    struct DeviceRequirements {
        // Minimum Vulkan version
        int minMajorVersion = 1;
        int minMinorVersion = 4;

        // Require certain queues?
        bool requireComputeQueue = true;
        bool requireTransferQueue = true;

        // Required extensions
        vector<string> extensions {};

        // Required features
        vk::PhysicalDeviceFeatures featuresBase {};
        vk::PhysicalDeviceVulkan11Features features11 {};
        vk::PhysicalDeviceVulkan12Features features12 {};
        vk::PhysicalDeviceVulkan13Features features13 {};   
        vk::PhysicalDeviceVulkan14Features features14 {};   

        vk::PhysicalDeviceAccelerationStructureFeaturesKHR featuresAcc {};
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR featuresRayPipe {};                                                                   

        // Insist on discrete GPU?
        bool discreteGPU = false;

        DeviceRequirements() {
            // Need swapchain
            extensions.push_back(vk::KHRSwapchainExtensionName);

            // Set default requested features            
            features13.dynamicRendering = true;
            features13.synchronization2 = true;
        };

        uint32_t getMinVulkanVersion() {
            return vk::makeApiVersion(0, minMajorVersion, minMinorVersion, 0);
        };

        vector<const char*> getExtensions() {
            return to_const_char_list(extensions);
        };

        vk::StructureChain< vk::PhysicalDeviceFeatures2,
                            vk::PhysicalDeviceVulkan11Features,
                            vk::PhysicalDeviceVulkan12Features,
                            vk::PhysicalDeviceVulkan13Features,   
                            vk::PhysicalDeviceVulkan14Features,  
                            vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
                            vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>
            getFeatureChain() {

            // Create v2 for the base features
            vk::PhysicalDeviceFeatures2 base2Features {};
            base2Features.features = featuresBase;

            // Create the rest of the structure chain
            vk::StructureChain<
                    vk::PhysicalDeviceFeatures2,
                    vk::PhysicalDeviceVulkan11Features,
                    vk::PhysicalDeviceVulkan12Features,
                    vk::PhysicalDeviceVulkan13Features,   
                    vk::PhysicalDeviceVulkan14Features,  
                    vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
                    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>
                featureChain = {
                    base2Features,
                    features11,
                    features12,
                    features13,
                    features14,
                    featuresAcc,
                    featuresRayPipe
                };
            return featureChain;
        };           
    };

    ///////////////////////////////////////////////////////////////////////////
    // HELPER FUNCTIONS
    ///////////////////////////////////////////////////////////////////////////
    
    inline const char* getDeviceTypeString(vk::PhysicalDeviceType t) {
        switch (t) {
            case vk::PhysicalDeviceType::eIntegratedGpu: return "Integrated GPU";
            case vk::PhysicalDeviceType::eDiscreteGpu:   return "Discrete GPU";
            case vk::PhysicalDeviceType::eVirtualGpu:    return "Virtual GPU";
            case vk::PhysicalDeviceType::eCpu:           return "CPU";
            default:                                     return "Other";
        }
    };

    inline void printPhysicalDeviceProperties(const vk::raii::PhysicalDevice &physicalDevice) {
        vk::PhysicalDeviceProperties props = physicalDevice.getProperties();
        uint32_t ver = props.apiVersion;
        
        cout << "Name: " << props.deviceName.data() << endl;
        cout << "Type: " << getDeviceTypeString(props.deviceType) << endl;    
        cout << "API Version: " 
            << VK_VERSION_MAJOR(ver) << "." 
            << VK_VERSION_MINOR(ver) << "." 
            << VK_VERSION_PATCH(ver) << endl;
    };

    inline void listAvailablePhysicalDevices(vk::raii::Instance &instance) {
        auto allPhysicalDevices = instance.enumeratePhysicalDevices();
        
        print_header_line('*');
        cout << "Found " << allPhysicalDevices.size() << " physical device(s):" << endl;
        for (int i = 0; i < allPhysicalDevices.size(); i++) {
            print_header_line('*', "Device " + to_string(i));            
            printPhysicalDeviceProperties(allPhysicalDevices[i]);        
        }
        print_header_line('*');
    };

    inline uint32_t getMaxNumberDescriptors(const vk::raii::PhysicalDevice &physicalDevice) {
        const auto limits = physicalDevice.getProperties().limits;

        uint32_t maximumCapacity = std::min({
            limits.maxPerStageDescriptorSamplers,
            limits.maxPerStageDescriptorSampledImages,
            limits.maxDescriptorSetSamplers,
            limits.maxDescriptorSetSampledImages
        });

        return maximumCapacity;
    };

    template<std::size_t Offset, typename Tuple, std::size_t... Indices>
    constexpr auto tupleToArrayImpl(
        Tuple&& tuple,
        std::index_sequence<Indices...>) {

        return std::array<vk::Bool32, sizeof...(Indices)>{
            std::get<Offset + Indices>(
                std::forward<Tuple>(tuple)
            )...
        };
    };

    template<std::size_t Count, std::size_t Offset = 2, typename Tuple>
    constexpr auto tupleToArray(Tuple&& tuple) {
        return tupleToArrayImpl<Offset>(
            std::forward<Tuple>(tuple),
            std::make_index_sequence<Count>{}
        );
    };

    template<typename Features, typename AllFeatures>
    constexpr Features getSpecificFeatures(
        AllFeatures allFeatures, 
        Features required) {

        return allFeatures.template get<Features>();
    };

    template<typename AllFeatures>
    constexpr vk::PhysicalDeviceFeatures getBaseFeatures(
        AllFeatures allFeatures) {

        auto available2 = allFeatures.template get<vk::PhysicalDeviceFeatures2>();   
        return available2.features;
    };

    template<std::size_t Offset = 2, typename Features>
    constexpr bool hasRequiredDeviceFeatures(
        Features available, 
        Features required) { 

        // Convert to arrays first
        auto availArray = tupleToArray<featureCnt<Features>, Offset>(available.reflect());
        auto reqArray = tupleToArray<featureCnt<Features>, Offset>(required.reflect());

        // Loop through the required features...
        for(int i = 0; i < reqArray.size(); i++) {
            if(reqArray[i] && !availArray[i]) {
                return false;
            }
        }

        // Survived, so true
        return true;
    };

    inline bool checkPhysicalDeviceRequirements(vk::raii::PhysicalDevice &physicalDevice,
                                                vk::raii::SurfaceKHR &surface,
                                                DeviceRequirements requirements) {
                                                              
            // Get properties
            auto deviceProps = physicalDevice.getProperties();

            // Need discrete?
            if(requirements.discreteGPU) {
                if(deviceProps.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
                    return false;
                }
            }

            // API version?            
            if(deviceProps.apiVersion < requirements.getMinVulkanVersion()) {
                // DOES NOT SUPPORT REQUESTED VERSION
                return false;
            }
            
            // Check queue support...            
            bool supportsGraphics = checkForQueue(physicalDevice, surface, {vk::QueueFlagBits::eGraphics}, {});
            bool supportsPresent = checkForQueue(physicalDevice, surface, {}, {}, true);            
            bool supportsCompute = checkForQueue(physicalDevice, surface, {vk::QueueFlagBits::eCompute}, {vk::QueueFlagBits::eGraphics});
            bool supportsTransfer = checkForQueue(physicalDevice, surface, {vk::QueueFlagBits::eTransfer}, {vk::QueueFlagBits::eCompute, vk::QueueFlagBits::eGraphics});
            
            if( !supportsGraphics ||
                !supportsPresent ||
                (requirements.requireComputeQueue && !supportsCompute) ||
                (requirements.requireTransferQueue && !supportsTransfer)) {
                // Does not support requested queues!
                return false;
            }

            // Check if all required extensions are available
            auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
            for(auto reqExt : requirements.extensions) {
                bool found = false;
                for(auto availExt : availableDeviceExtensions) {
                    if(!strcmp(availExt.extensionName, reqExt.c_str())) {
                        found = true;
                        break;
                    }
                }

                if(!found) {
                    // Required extension not found!
                    return false;
                }
            }
            
            // Get all available device features
            auto allFeatures = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2,
                                                                    vk::PhysicalDeviceVulkan11Features,
                                                                    vk::PhysicalDeviceVulkan12Features,
                                                                    vk::PhysicalDeviceVulkan13Features,
                                                                    vk::PhysicalDeviceVulkan14Features,
                                                                    vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
                                                                    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
                                                                    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

            // Check each set of required features
            if(!hasRequiredDeviceFeatures<0>(getBaseFeatures(allFeatures),
                                            requirements.featuresBase)) {
                return false;
            }

            if(!hasRequiredDeviceFeatures(getSpecificFeatures(allFeatures, requirements.features11),
                                           requirements.features11)) {
                return false;
            }

            if(!hasRequiredDeviceFeatures(getSpecificFeatures(allFeatures, requirements.features12),
                                           requirements.features12)) {
                return false;
            }

            if(!hasRequiredDeviceFeatures(getSpecificFeatures(allFeatures, requirements.features13),
                                           requirements.features13)) {
                return false;
            }

            if(!hasRequiredDeviceFeatures(getSpecificFeatures(allFeatures, requirements.features14),
                                           requirements.features14)) {
                return false;
            }

            if(!hasRequiredDeviceFeatures(getSpecificFeatures(allFeatures, requirements.featuresAcc),
                                           requirements.featuresAcc)) {
                return false;
            }

            if(!hasRequiredDeviceFeatures(getSpecificFeatures(allFeatures, requirements.featuresRayPipe),
                                           requirements.featuresRayPipe)) {
                return false;
            }

            // Survived all that, so suitable...  
            return true;
        };

    ///////////////////////////////////////////////////////////////////////////
    // CLASSES 
    ///////////////////////////////////////////////////////////////////////////

}
