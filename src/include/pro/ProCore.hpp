#pragma once
#include "ProDevice.hpp"
#include "ProQueue.hpp"
#include "ProSwap.hpp"
#include "ProImage.hpp"
#include "ProCommand.hpp"

namespace pro { 

    ///////////////////////////////////////////////////////////////////////////
    // FUNCTION POINTERS 
    ///////////////////////////////////////////////////////////////////////////

    using CreateSurfaceFunc = std::function<VkResult(VkInstance, VkSurfaceKHR&)>;    
    using WaitUntilWindowEventFunc = std::function<void()>;
    using OnResizeFunc = std::function<void()>;
    using GetWindowExtensionsFunc = std::function<vector<string>(const vk::raii::Context&)>;
    using ValidationDebugCallbackFunc = vk::PFN_DebugUtilsMessengerCallbackEXT;
    
    ///////////////////////////////////////////////////////////////////////////
    // DEFAULT CALLBACKS 
    ///////////////////////////////////////////////////////////////////////////
    
    inline VKAPI_ATTR VkBool32 VKAPI_CALL defaultDebugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {

        // Adapted from: https://docs.vulkan.org/tutorial/latest
        if( severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || 
            severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {

            //cerr << "[Vulkan Validation Layer]";
            cerr << "[" << to_upper(to_string(severity)) << "]";
            //cerr << to_string(type);
            cerr << " " << pCallbackData->pMessage << endl;
        }

        return vk::False;
    };
    
    ///////////////////////////////////////////////////////////////////////////
    // STRUCTS 
    ///////////////////////////////////////////////////////////////////////////

    struct VulkanCoreCreateInfo {
        // Vulkan instance
        string appName = "ProApp";
        string engineName = "ProEngine";

        // Instance extensions and flags
        vector<string> instExtensions {};
        vk::InstanceCreateFlags instFlags {};

        
        // Vulkan physical device requirements
        DeviceRequirements deviceRequirements {};
        
        // Window size, waiting, and extensions
        GetCurrentWindowSizeFunc getCurrentWindowSizeFunc = nullptr;
        WaitUntilWindowEventFunc waitUntilWindowEventFunc = nullptr;
        GetWindowExtensionsFunc getWindowExtensionsFunc = nullptr;

        // Validation layer debug callback
        ValidationDebugCallbackFunc validationDebugCallbackFunc = defaultDebugCallback;
        
        // Surface
        CreateSurfaceFunc createSurfaceFunc = nullptr;

        // Swapchain
        vk::SurfaceFormatKHR desiredSwapchainFormat {};
            
        VulkanCoreCreateInfo() {
            // Make sure it stores values in linear space, BUT
            // does gamma correction during presentation            
            desiredSwapchainFormat.format = vk::Format::eB8G8R8A8Unorm;
            desiredSwapchainFormat.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        };
    };
        
    ///////////////////////////////////////////////////////////////////////////
    // HELPER FUNCTIONS
    ///////////////////////////////////////////////////////////////////////////    

    inline void printMaxSupportedVulkanVersion() {        
        uint32_t apiVer = vk::enumerateInstanceVersion();
        cout << "Loader supports up to Vulkan "
                << VK_VERSION_MAJOR(apiVer) << "."
                << VK_VERSION_MINOR(apiVer) << "."
                << VK_VERSION_PATCH(apiVer) << endl;
    }

    inline vector<string> getSupportedExtensionNameList(const vk::raii::Context &context) {
        auto extensionProperties = context.enumerateInstanceExtensionProperties();
        vector<string> nameList(extensionProperties.size());
        for(int i = 0; i < extensionProperties.size(); i++) {               
            nameList[i] = extensionProperties[i].extensionName.data();
        }
        return nameList;
    };

    inline vector<string> getSupportedLayersList(const vk::raii::Context &context) {
        auto layerProperties = context.enumerateInstanceLayerProperties();
        vector<string> nameList(layerProperties.size());
        for(int i = 0; i < layerProperties.size(); i++) {               
            nameList[i] = layerProperties[i].layerName.data();            
        }
        return nameList;
    };
    
    inline bool checkAllSupported(  const vector<string> &supported, 
                                    const vector<string> &requested,
                                    string typeName) {
        for(string req : requested) {
            if(find(supported.begin(), supported.end(), req) == supported.end()) {
                print_failure("checkContextForRequired", typeName + " not supported: " + req);
                return false;
            }
        }

        return true;
    };
    
    ///////////////////////////////////////////////////////////////////////////
    // CLASSES 
    ///////////////////////////////////////////////////////////////////////////
        
    class VulkanCore {
    public:
        VulkanCore(VulkanCoreCreateInfo &createInfo) {
            // Quick sanity checks...
            if(!createInfo.createSurfaceFunc) {
                print_and_throw_error("VulkanCore", "createSurfaceFunc cannot be null!");                
            }
            
            if(!createInfo.getCurrentWindowSizeFunc) {
                print_and_throw_error("VulkanCore", "getCurrentWindowSizeFunc cannot be null!");                
            }

            if(!createInfo.waitUntilWindowEventFunc) {            
                print_and_throw_error("VulkanCore", "waitUntilWindowEventFunc cannot be null!");
            }

            if(!createInfo.getWindowExtensionsFunc) {            
                print_and_throw_error("VulkanCore", "getWindowExtensionsFunc cannot be null!");
            }

            // Copy our functions for later use 
            this->getCurrentWindowSizeFunc = createInfo.getCurrentWindowSizeFunc;
            this->waitUntilWindowEventFunc = createInfo.waitUntilWindowEventFunc;

            // Extension support     
            auto supportedExtensions = getSupportedExtensionNameList(context_);
            //print_string_list("SUPPORTED", supportedExtensions);

            vector<string> requiredExtensions {};
            auto windowExtensions = createInfo.getWindowExtensionsFunc(context_);
            requiredExtensions.insert(requiredExtensions.end(), windowExtensions.begin(), windowExtensions.end());
            requiredExtensions.push_back(vk::EXTDebugUtilsExtensionName); 
            
            requiredExtensions.insert(  requiredExtensions.end(), 
                                        createInfo.instExtensions.begin(), 
                                        createInfo.instExtensions.end());

            if(!checkAllSupported(supportedExtensions, requiredExtensions, "Extension")) {
                print_and_throw_error("VulkanCore", "Cannot support requested extensions!");
            }
            
            // Layer support 
            auto supportedLayers = getSupportedLayersList(context_);
            vector<string> requiredLayers = {
                "VK_LAYER_KHRONOS_validation"
            };
            
            if(!checkAllSupported(supportedLayers, requiredLayers, "Layer")) {
                print_and_throw_error("VulkanCore", "Cannot support requested layers!");
            }

            auto extensionCharList = to_const_char_list(requiredExtensions);
            auto layerCharList = to_const_char_list(requiredLayers);
                                   
            // Create application info
            auto appInfo = vk::ApplicationInfo()
                            .setPApplicationName(createInfo.appName.c_str())
                            .setApplicationVersion(VK_MAKE_VERSION(1,0,0))
                            .setPEngineName(createInfo.engineName.c_str())
                            .setEngineVersion(VK_MAKE_VERSION(1,0,0))
                            .setApiVersion(createInfo.deviceRequirements.getMinVulkanVersion());
        
            
            // Set up validation layer and debugging callback
            auto severityFlags( vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    
            auto messageTypeFlags(  vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
                                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | 
                                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
            
            auto debugMessengerCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT()
                                                        .setMessageSeverity(severityFlags)
                                                        .setMessageType(messageTypeFlags)
                                                        .setPfnUserCallback(createInfo.validationDebugCallbackFunc);
                                                        
            // Actually create the instance
            auto instCreateInfo = vk::InstanceCreateInfo()
                                    .setPNext(&debugMessengerCreateInfo)         // Makes debug function available for instance creation
                                    .setPApplicationInfo(&appInfo)
                                    .setEnabledExtensionCount(extensionCharList.size())                                    
                                    .setPpEnabledExtensionNames(extensionCharList.data())
                                    .setEnabledLayerCount(layerCharList.size())
                                    .setPpEnabledLayerNames(layerCharList.data());

            instCreateInfo.flags |= createInfo.instFlags;

            instance_ = vk::raii::Instance(context_, instCreateInfo);

            // Create validation debug messenger
            debugMessenger_ = instance_.createDebugUtilsMessengerEXT(debugMessengerCreateInfo);
            
            // Create surface
            VkSurfaceKHR csurface = nullptr;
            VkResult surfErr = createInfo.createSurfaceFunc(*instance_, csurface);
            if(surfErr != VK_SUCCESS) {
                print_and_throw_error("VulkanCore", surfErr + ""); 
            }
            surface_ = vk::raii::SurfaceKHR(instance_, csurface);

            // Get list of physical devices
            auto physicalDevicesList = instance_.enumeratePhysicalDevices();

            if(physicalDevicesList.empty()) {
                print_and_throw_error("VulkanCore", "Failed to find GPUs with Vulkan support!");                                
            }

            listAvailablePhysicalDevices(instance_);

            // Loop through and pick first device that fits...
            for(auto physicalDevice : physicalDevicesList) {
                if(checkPhysicalDeviceRequirements(physicalDevice, surface_, createInfo.deviceRequirements)) {
                    // FOUND IT
                    physicalDevice_ = physicalDevice;
                    break;
                }
            }

            if(physicalDevice_ == nullptr) {
                print_and_throw_error("VulkanCore", "Could not find suitable device!"); 
            }
            
            print_header_line('*', "Chosen Device");            
            printPhysicalDeviceProperties(physicalDevice_);

            // Get queue family indices
            // NOTE: Try to get SAME queue for graphics and present, but fall back to separate if necessary
            
            int graphicsIndex = findQueueIndex(physicalDevice_, surface_, {vk::QueueFlagBits::eGraphics}, {}, true);
            int presentIndex = graphicsIndex;
            
            if(graphicsIndex == INVALID_QUEUE) {
                graphicsIndex = findQueueIndex(physicalDevice_, surface_, {vk::QueueFlagBits::eGraphics}, {});
                presentIndex = findQueueIndex(physicalDevice_, surface_, {}, {}, true);           
            }

            int computeIndex = findQueueIndex(physicalDevice_, surface_, {vk::QueueFlagBits::eCompute}, {vk::QueueFlagBits::eGraphics});
            int transferIndex = findQueueIndex(physicalDevice_, surface_, {vk::QueueFlagBits::eTransfer}, {vk::QueueFlagBits::eCompute, vk::QueueFlagBits::eGraphics});
                        
            // Set up queue creation info
            vector<vk::DeviceQueueCreateInfo> queueCreateInfos {};
            float queuePriority = 1.0f;

            queueCreateInfos.push_back(vk::DeviceQueueCreateInfo()
                                        .setQueueFamilyIndex(graphicsIndex)
                                        .setQueueCount(1)
                                        .setPQueuePriorities(&queuePriority));     

            if(presentIndex != graphicsIndex) {
                queueCreateInfos.push_back(vk::DeviceQueueCreateInfo()
                                        .setQueueFamilyIndex(presentIndex)
                                        .setQueueCount(1)
                                        .setPQueuePriorities(&queuePriority)); 
            }

            if(computeIndex != INVALID_QUEUE) {
                queueCreateInfos.push_back(vk::DeviceQueueCreateInfo()
                                        .setQueueFamilyIndex(computeIndex)
                                        .setQueueCount(1)
                                        .setPQueuePriorities(&queuePriority));     
            }

            if(transferIndex != INVALID_QUEUE) {
                queueCreateInfos.push_back(vk::DeviceQueueCreateInfo()
                                        .setQueueFamilyIndex(transferIndex)
                                        .setQueueCount(1)
                                        .setPQueuePriorities(&queuePriority));     
            }
            
            // Create logical device
            auto featureChain = createInfo.deviceRequirements.getFeatureChain();
            auto deviceExtensions = createInfo.deviceRequirements.getExtensions();

            auto deviceCreateInfo = vk::DeviceCreateInfo()
                                        .setQueueCreateInfos(queueCreateInfos)
                                        .setEnabledExtensionCount(deviceExtensions.size())
                                        .setPpEnabledExtensionNames(deviceExtensions.data())
                                        .setPNext(&featureChain.get<vk::PhysicalDeviceFeatures2>());
            
            device_ = vk::raii::Device(physicalDevice_, deviceCreateInfo);
            
            // Get the queues
            graphicsQueue_ = VulkanQueue(device_, graphicsIndex);
            if(presentIndex != graphicsIndex) {
                presentQueue_ = VulkanQueue(device_, presentIndex);
            }
            else {
                presentQueue_ = graphicsQueue_;
            }

            if(computeIndex != INVALID_QUEUE) {
                computeQueue_ = VulkanQueue(device_, computeIndex);
            }

            if(transferIndex != INVALID_QUEUE) {
                transferQueue_ = VulkanQueue(device_, transferIndex);
            }

            printQueues();

            // Check if desired swapchain format is available
            if(!checkSwapSurfaceFormat(physicalDevice_, surface_, createInfo.desiredSwapchainFormat)) {
                print_and_throw_error("VulkanCore", "Desired swapchain format not avialable!"); 
            }

            // Set desired format
            swapchain_create_format_ = createInfo.desiredSwapchainFormat;

            // Create swapchain            
            if(!createSwapchain()) {                
                print_and_throw_error("VulkanCore", "Unable to create swapchain!");     
            }   
            
            // VMA-Hpp     
            vma::AllocatorCreateInfo allocCreateInfo = { {}, physicalDevice_ };

            if(createInfo.deviceRequirements.features12.bufferDeviceAddress) {
                // VMA must know that buffers may expose GPU addresses. Without
                // this flag, allocations used by acceleration structures and
                // shader binding tables are not guaranteed to be addressable.
                allocCreateInfo.flags |= vma::AllocatorCreateFlagBits::eBufferDeviceAddress;
            }

            allocator_ = vma::raii::Allocator(instance_, device_, allocCreateInfo);

            // Create depth images
            recreateDepthImages();
            
            // Success!
            isValid = true;
        };
        
        ~VulkanCore() noexcept {
            cleanup();         
        };

        // Reset: actual destructor
        void cleanup() noexcept {
            if(isValid) {                
                device_.waitIdle();      
                cleanupSwapchain();
                isValid = false;
            }
        };

        // Copy: forbidden (unique ownership)
        VulkanCore(const VulkanCore&)            = delete;  // Copy constructor
        VulkanCore& operator=(const VulkanCore&) = delete;  // Copy assignment

        // Moving: forbidden (really only one of these kicking around)
        VulkanCore(VulkanCore&&) noexcept            = delete;  // Move
        VulkanCore& operator=(VulkanCore&&) noexcept = delete;  // Move-assignment

        // Getters        
        const vk::raii::Instance& instance() const noexcept { return instance_; };
        const vk::raii::PhysicalDevice& physicalDevice() const noexcept { return physicalDevice_; };
        const vk::raii::Device& device() const noexcept { return device_; };
        const VulkanQueue& graphicsQueue() const noexcept { return graphicsQueue_; };
        const VulkanQueue& presentQueue() const noexcept {  return presentQueue_; };
        const VulkanQueue& computeQueue() const noexcept {  return computeQueue_; };
        const VulkanQueue& transferQueue() const noexcept {  return transferQueue_; };
        const VulkanSwapChain& swapchain() const noexcept { return swapchain_; };
        const vma::raii::Allocator& allocator() const noexcept { return allocator_; };
        const vector<VulkanImage>& depthImages() const noexcept { return depthImages_; };

        const bool isComputeQueueValid() const noexcept { return computeQueue_.is_valid; }
        const bool isTransferQueueValid() const noexcept { return transferQueue_.is_valid; }

        // Setter
        void setOnResizeFunc(OnResizeFunc onResizeFunc) {
            this->onResizeFunc = onResizeFunc;
        };

        // Other member functions 
        void doWindowResize() {
            // Make sure window isn't minimized...
            int width = 0;
            int height = 0;

            do {
                getCurrentWindowSizeFunc(width, height);
                // If minimized, this will be 0,0. Block until restored.
                waitUntilWindowEventFunc(); // Actually waits/sleeps/blocks until an event happens
            } while (width == 0 || height == 0);
        
            // Recreate swapchain
            recreateSwapchain();

            // Recreate depth images
            recreateDepthImages();

            // Perform other requested resize actions
            if(onResizeFunc) {
                onResizeFunc();
            }
        };

        void printQueues(std::ostream& os = std::cout) {

            print_header_line('*', "QUEUES:", DEFAULT_MAX_LINE_WIDTH, os);         

            os << "Graphics: " << graphicsQueue_.index << endl;
            os << "Present: " << presentQueue_.index << endl;
            if(computeQueue_.is_valid) os << "Compute: " << computeQueue_.index << endl;
            if(transferQueue_.is_valid) os << "Transfer: " << transferQueue_.index << endl;
            print_header_line('-', "", DEFAULT_MAX_LINE_WIDTH, os);                     
            if(computeQueue_.is_valid) {                
                if(isComputeDedicated()) {
                    os << "Using dedicated compute queue." << endl;
                }
                else {
                    os << "Compute queue same as graphics queue." << endl;
                }
            }
            else {
                os << "Compute queue NOT valid." << endl;
            }

            if(transferQueue_.is_valid) {
                if(isTransferDedicated()) {
                    os << "Using dedicated transfer queue." << endl;
                }
                else {
                    os << "Transfer queue same as graphics queue." << endl;
                }
            }
            else {
                os << "Transfer queue NOT valid." << endl;
            }
            
            print_header_line('*', "", DEFAULT_MAX_LINE_WIDTH, os);              
        };

        bool isComputeDedicated() {
            return (graphicsQueue_.queue != computeQueue_.queue);
        };

        bool isTransferDedicated() {
            return (graphicsQueue_.queue != transferQueue_.queue);
        };

    private:   
        vk::raii::Context   context_;
        vk::raii::Instance  instance_ = nullptr;
        vk::raii::DebugUtilsMessengerEXT debugMessenger_ = nullptr;
        vk::raii::SurfaceKHR surface_ = nullptr;
        vk::raii::PhysicalDevice physicalDevice_ = nullptr;
        vk::raii::Device device_ = nullptr;

        VulkanQueue graphicsQueue_ {};           
        VulkanQueue presentQueue_ {};            
        VulkanQueue computeQueue_ {};            
        VulkanQueue transferQueue_ {};      
        
        VulkanSwapChain swapchain_ {};                    
        vk::SurfaceFormatKHR swapchain_create_format_ {};   

        vma::raii::Allocator allocator_ = nullptr;   
        
        vector<VulkanImage> depthImages_ {};

        bool isValid = false;                    // Is the whole thing valid?

        GetCurrentWindowSizeFunc getCurrentWindowSizeFunc = nullptr;    
        WaitUntilWindowEventFunc waitUntilWindowEventFunc = nullptr;        
        OnResizeFunc onResizeFunc = nullptr;

        bool createSwapchain() {
            // Check what we need in terms of queue sharing
            vk::SharingMode swapSharingMode = vk::SharingMode::eExclusive;
            if(presentQueue_.index != graphicsQueue_.index) {
                swapSharingMode = vk::SharingMode::eConcurrent;
            }

            // Get extent
            auto extent = chooseSwapExtent(physicalDevice_, surface_, getCurrentWindowSizeFunc);

            // Get surface capabilities
            vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice_.getSurfaceCapabilitiesKHR(*surface_);

            // Set up creation info
            auto swapChainCreateInfo = vk::SwapchainCreateInfoKHR()
                                        .setSurface(*surface_)
                                        .setMinImageCount(chooseSwapMinImageCount(physicalDevice_, surface_))
                                        .setImageFormat(swapchain_create_format_.format)
                                        .setImageColorSpace(swapchain_create_format_.colorSpace)
                                        .setImageExtent(extent)
                                        .setImageArrayLayers(1)
                                        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                                        .setImageSharingMode(swapSharingMode)
                                        .setPreTransform(surfaceCapabilities.currentTransform)
                                        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                                        .setPresentMode(chooseSwapPresentMode(physicalDevice_, surface_))
                                        .setClipped(true)
                                        .setOldSwapchain(nullptr);
             
            // Create actual swapchain
            swapchain_.chain = vk::raii::SwapchainKHR(device_, swapChainCreateInfo);

            // Store the rest of the items in our data structure            
            swapchain_.format = swapchain_create_format_.format;
            swapchain_.extent = extent;

            // Get images and views
            auto images = swapchain_.chain.getImages();
                        
            for(int i = 0; i < images.size(); i++) {
                // Create struct
                VulkanSwapImage vimage {};

                // Store image
                vimage.image = images[i];

                // Create image view
                auto imageViewCreateInfo = vk::ImageViewCreateInfo()
                                            .setImage(images[i])                                            
                                            .setViewType(vk::ImageViewType::e2D)
                                            .setFormat(swapchain_.format)
                                            .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 })
                                            .setComponents({
                                                vk::ComponentSwizzle::eIdentity, 
                                                vk::ComponentSwizzle::eIdentity, 
                                                vk::ComponentSwizzle::eIdentity, 
                                                vk::ComponentSwizzle::eIdentity
                                            });
                
                vimage.view = vk::raii::ImageView(device_, imageViewCreateInfo);

                // Create semaphore for rendering
                vimage.renderDone = vk::raii::Semaphore(device_, vk::SemaphoreCreateInfo());
                
                // Add to list of swaps (but move because of RAII view)
                swapchain_.swaps.push_back(move(vimage));
            }
           
            return true;
        };

        void cleanupSwapchain() {                
            swapchain_.swaps.clear();             
            swapchain_.chain = nullptr;
            swapchain_ = {};            
        };   
        
        void recreateSwapchain() {    
            // Wait until device is idle
            device_.waitIdle();

            // Cleanup swapchain data
            cleanupSwapchain();
            
            // (Re)create swap chain and image views
            createSwapchain();
        };

        void recreateDepthImages() {

            // Create a temporary command pool and buffer for image transitions
            CommandData depthCommand = { device_, graphicsQueue_ };
            
            // Start recording...   
            depthCommand.resetFence();
            depthCommand.beginRecording();    
            
            // Cleanup previous images if there are any...
            depthImages_.clear();

            // For each swap image...
            for(unsigned int i = 0; i < swapchain_.swaps.size(); i++) {
                VulkanImage depthImage =  createVulkanImage(
                                            device_, 
                                            allocator_,
                                            getTypicalDepthImageCreateInfo(
                                                swapchain_.extent.width,
                                                swapchain_.extent.height),
                                            vk::ImageAspectFlagBits::eDepth);
            
                performImageTransition( depthCommand.buffer(), 
                                        depthImage.image(),
                                        IMAGE_STATE_TYPE::UNDEF,
                                        IMAGE_STATE_TYPE::DEPTH,
                                        vk::ImageAspectFlagBits::eDepth);
            
                depthImages_.push_back(move(depthImage));           
            }

            // End recording
            depthCommand.endRecording();

            // Submit to queue
            depthCommand.submit();    
            depthCommand.waitForFence();
        };
    }; 
}
