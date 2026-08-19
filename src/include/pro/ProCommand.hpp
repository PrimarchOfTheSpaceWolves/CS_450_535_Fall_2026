#pragma once
#include "ProBase.hpp"

namespace pro {
    
    ///////////////////////////////////////////////////////////////////////////
    // HELPER FUNCTIONS 
    ///////////////////////////////////////////////////////////////////////////

    inline vk::raii::CommandPool createCommandPool(
        const vk::raii::Device &device,
        unsigned int queueIndex,
        vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlagBits::eTransient) {

        return move(vk::raii::CommandPool(
            device, vk::CommandPoolCreateInfo(flags, queueIndex)));        
    };

    inline vk::raii::CommandBuffer createCommandBuffer(
        const vk::raii::Device &device,
        vk::raii::CommandPool &commandPool,
        vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) {

        auto buffer = move(vk::raii::CommandBuffers(
            device, vk::CommandBufferAllocateInfo(commandPool, level, 1)).front());

        return move(buffer);               
    };

    inline vk::raii::Fence createFence(
        const vk::raii::Device &device,
        vk::FenceCreateInfo createInfo = vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)) {

        return move(vk::raii::Fence(device, createInfo));
    };

    inline vk::raii::Semaphore createSemaphore(
        const vk::raii::Device &device,
        vk::SemaphoreCreateInfo createInfo = vk::SemaphoreCreateInfo()) {

        return move(vk::raii::Semaphore(device, createInfo));        
    };
        
    ///////////////////////////////////////////////////////////////////////////
    // CLASSES 
    ///////////////////////////////////////////////////////////////////////////

    class CommandData {
    private:
        vk::raii::CommandPool commandPool = nullptr;
        vk::raii::CommandBuffer commandBuffer = nullptr;
        vk::raii::Fence finishedFence = nullptr;
        unordered_map<std::string, vk::raii::Semaphore> alwaysWaitSemaphores {};
        unordered_map<std::string, vk::raii::Semaphore> alwaysSignalSemaphores {};
        vector<vk::PipelineStageFlags> alwaysWaitStages {};

        bool isValid = false;                       // Is the whole thing valid?
        
        const vk::raii::Device *refDevice = nullptr;     
        VulkanQueue targetQueue;                        

    public:
        CommandData(const vk::raii::Device &device,
                    const VulkanQueue &queue,
                    const vector<string> waitSemaphoresToCreate = {},
                    const vector<string> signalSemaphoresToCreate = {},
                    const vector<vk::PipelineStageFlags> waitStages = {},                
                    vk::CommandPoolCreateFlags poolFlags = vk::CommandPoolCreateFlagBits::eTransient) {

            // Save for future reference
            this->refDevice = &device;
            this->targetQueue = queue;

            // Create necessary pieces
            this->commandPool = createCommandPool(*refDevice, queue.index, poolFlags);
            this->commandBuffer = createCommandBuffer(*refDevice, commandPool);
            this->finishedFence = createFence(*refDevice);

            // Do the number of wait stages match the wait semaphores?
            if(waitStages.size() != waitSemaphoresToCreate.size()) {
                print_and_throw_error("CommandData", "Mismatch between wait stages and semaphores!"); 
            }

            // Copy in wait stages
            alwaysWaitStages.insert(alwaysWaitStages.end(), waitStages.begin(), waitStages.end());

            // Create semaphores for wait and signal
            for(auto key : waitSemaphoresToCreate) {
                this->alwaysWaitSemaphores.insert_or_assign(key, move(createSemaphore(*refDevice)));
            }

            for(auto key : signalSemaphoresToCreate) {
                this->alwaysSignalSemaphores.insert_or_assign(key, move(createSemaphore(*refDevice)));
            }
            
            // Ready to go!
            isValid = true;
        };

        // No moving or copying
        CommandData(const CommandData&)                 = delete;
        CommandData& operator=(const CommandData&)      = delete;
        CommandData(CommandData&&) noexcept             = delete;  
        CommandData& operator=(CommandData&&) noexcept  = delete;  

        ~CommandData() {
            cleanup();
        };

        void cleanup() {
            if(isValid) {

                if(!refDevice) {
                    return;
                }

                alwaysWaitSemaphores.clear();
                alwaysSignalSemaphores.clear();

                isValid = false;
            }
        };

        const vk::raii::CommandPool& pool() const noexcept { return commandPool; };
        const vk::raii::CommandBuffer& buffer() const noexcept { return commandBuffer; };
        const vk::raii::Fence& fence() const noexcept { return finishedFence; };
        const VulkanQueue& queue() const noexcept { return targetQueue; };

        const vk::Semaphore& waitSemaphore(string key) const noexcept { return *(alwaysWaitSemaphores.at(key)); }; 
        const vk::Semaphore& signalSemaphore(string key) const noexcept { return *(alwaysSignalSemaphores.at(key)); }; 

        void waitForFence() const {     
            refDevice->waitForFences(*finishedFence, true, UINT64_MAX);
        };

        void resetFence() const {
            // Reset the fence since we're about to submit work
            refDevice->resetFences(*finishedFence);            
        };

        void beginRecording(vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit) const {
            // Reset our command pool so it's cleared and ready to go
            commandPool.reset();
            
            // Begin recording
            commandBuffer.begin(vk::CommandBufferBeginInfo());
        };

        void endRecording() const {
            // End recording
            commandBuffer.end();
        };

        void submit(const vector<vk::Semaphore> additionalWaitSemaphores = {},
                    const vector<vk::Semaphore> additionalSignalSemaphores = {},
                    const vector<vk::PipelineStageFlags> additionalWaitStages = {}) const {
            
            // Do the number of wait stages match the wait semaphores?
            if(additionalWaitStages.size() != additionalWaitSemaphores.size()) {
                print_and_throw_error("CommandData::submit", "Mismatch between wait stages and semaphores!"); 
            }
                    
            // Set up wait semaphores
            vector<vk::Semaphore> waitSemaphores {};
            for(auto key : alwaysWaitSemaphores | views::keys) {
                waitSemaphores.push_back(*(alwaysWaitSemaphores.at(key)));
            }
            for(auto s : additionalWaitSemaphores) {
                waitSemaphores.push_back(s);
            }

            // Set up wait stages
            vector<vk::PipelineStageFlags> waitStages;
            waitStages.insert(waitStages.end(), alwaysWaitStages.begin(), alwaysWaitStages.end());
            waitStages.insert(waitStages.end(), additionalWaitStages.begin(), additionalWaitStages.end());

            // Set up signal semaphores
            vector<vk::Semaphore> signalSemaphores {};
            for(auto key : alwaysSignalSemaphores | views::keys) {
                signalSemaphores.push_back(*(alwaysSignalSemaphores.at(key)));
            }
            for(auto s : additionalSignalSemaphores) {
                signalSemaphores.push_back(s);
            }

            // Prepare submission and submit
            // (Noting that we will also signal the fence as well)
            vk::SubmitInfo submitInfo(
                waitSemaphores,
                waitStages,
                *commandBuffer,
                signalSemaphores);
                        
            targetQueue.queue.submit(submitInfo, *finishedFence);  
        };
    };
}
