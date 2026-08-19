#pragma once
#include "ProCommand.hpp"
#include "ProBuffer.hpp"
#include "ProImage.hpp"
#include "ProStaging.hpp"

namespace pro {

    ///////////////////////////////////////////////////////////////////////////
    // STRUCTS 
    ///////////////////////////////////////////////////////////////////////////

    struct PendingBufferCopy {        
        void *hostData = nullptr;        
        VulkanBufferPtr dstBuffer = nullptr;

        vk::PipelineStageFlags2 dstStageMask {};
        vk::AccessFlags2 dstAccessMask {};

        PendingBufferCopy(  VulkanBufferPtr dstBuffer, 
                            void *hostData, 
                            vk::PipelineStageFlags2 dstStageMask, 
                            vk::AccessFlags2 dstAccessMask) {     

            this->dstBuffer = dstBuffer;
            this->hostData = hostData;
            this->dstStageMask = dstStageMask;
            this->dstAccessMask = dstAccessMask;
        };
    };
    
    ///////////////////////////////////////////////////////////////////////////
    // CLASSES 
    ///////////////////////////////////////////////////////////////////////////  
    
    class BufferCopyReceipt {
    private:
        string copyID = "";
        VulkanCore *refCore;        // Do NOT clean up!!!
                     
        vk::raii::CommandPool transferCommandPool = nullptr;
        vk::raii::CommandBuffer transferCommandBuffer = nullptr;
        vk::raii::Fence copyFinished = nullptr; 
        
        bool useDedicatedTransferQueue = false;

        vector<vk::BufferMemoryBarrier2> allReceiveBarriers {};        
        vector<VulkanBuffer> allStageBuffers {};  
        vector<VulkanBufferPtr> allDstBuffers {};   // Here just to ensure they survive until copy complete.
                        
    public:        
        BufferCopyReceipt(  VulkanCore &vkCore, 
                            string copyID,
                            vector<PendingBufferCopy> &allPendingCopies) {   

            // Store init data
            refCore = &vkCore;

            // Set copyID
            this->copyID = copyID;

            // Create pool from transfer queue if possible;
            // otherwise, just use graphics queue
            useDedicatedTransferQueue = refCore->transferQueue().is_valid;
            uint32_t chosenQueueIndex = 0;
            if(useDedicatedTransferQueue) {                
                chosenQueueIndex = refCore->transferQueue().index;            
            }
            else {
                chosenQueueIndex = refCore->graphicsQueue().index;            
            }
            this->transferCommandPool = createCommandPool(refCore->device(), chosenQueueIndex);
            
            // Create the fence (but start as UNsignaled)
            this->copyFinished = createFence(refCore->device(), vk::FenceCreateInfo());

            // Create the command buffer
            this->transferCommandBuffer = createCommandBuffer(refCore->device(), transferCommandPool);
            
            // Start recording            
            this->transferCommandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

            // For each copy...
            vector<vk::BufferMemoryBarrier2> srcOwnershipBarriers {};
            for(auto &pendingCopy : allPendingCopies) {
                // Make the staging buffer
                VulkanBuffer stageBuffer = createStagingBuffer(
                    refCore->allocator(),
                    pendingCopy.dstBuffer->size(),
                    pendingCopy.hostData);
                
                // Record the copy
                vk::BufferCopy copyRegion{};
                copyRegion.size = pendingCopy.dstBuffer->size();
                this->transferCommandBuffer.copyBuffer(stageBuffer.buffer(), pendingCopy.dstBuffer->buffer(), {copyRegion});
                
                // Are we using the transfer queue?
                if(useDedicatedTransferQueue) {

                    // Create the source ownership transfer barrier
                    vk::BufferMemoryBarrier2 tbarrier{};

                    tbarrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;                   
                    tbarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;

                    tbarrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;                 
                    tbarrier.dstAccessMask = vk::AccessFlagBits2::eNone;
                                           
                    tbarrier.srcQueueFamilyIndex = refCore->transferQueue().index;
                    tbarrier.dstQueueFamilyIndex = refCore->graphicsQueue().index; 
                    tbarrier.buffer = pendingCopy.dstBuffer->buffer();
                    tbarrier.size = vk::WholeSize;
                    srcOwnershipBarriers.push_back(tbarrier);

                    // Create the destination ownership transfer barrier
                    vk::BufferMemoryBarrier2 gbarrier{};

                    gbarrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
                    gbarrier.srcAccessMask = vk::AccessFlagBits2::eNone; 

                    gbarrier.dstStageMask = pendingCopy.dstStageMask;    
                    gbarrier.dstAccessMask = pendingCopy.dstAccessMask;                    
                    
                    gbarrier.srcQueueFamilyIndex = refCore->transferQueue().index;
                    gbarrier.dstQueueFamilyIndex = refCore->graphicsQueue().index; 
                    gbarrier.buffer = pendingCopy.dstBuffer->buffer();
                    gbarrier.size = vk::WholeSize;                               
                    this->allReceiveBarriers.push_back(gbarrier);                    
                }

                // Move to list of staging buffers
                this->allStageBuffers.push_back(move(stageBuffer));

                // Add to list of destination buffers
                this->allDstBuffers.push_back(pendingCopy.dstBuffer);
            }

            // If we have source ownership barriers...
            if(srcOwnershipBarriers.size() > 0) {
                // Do the source ownership barriers at the bottom of the pipeline
                this->transferCommandBuffer.pipelineBarrier2(        
                    vk::DependencyInfo().setBufferMemoryBarriers(srcOwnershipBarriers));                
            }

            // End recording
            this->transferCommandBuffer.end();
        };

        // Copy: forbidden (unique ownership)
        BufferCopyReceipt(const BufferCopyReceipt&)            = delete;  // Copy constructor
        BufferCopyReceipt& operator=(const BufferCopyReceipt&) = delete;  // Copy assignment

        // Moving: OK
        BufferCopyReceipt(BufferCopyReceipt&&) noexcept            = default;  // Move
        BufferCopyReceipt& operator=(BufferCopyReceipt&&) noexcept = default;  // Move-assignment
        
        ~BufferCopyReceipt() = default;

        void submit() {
            vk::SubmitInfo submitInfo{};
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &(*transferCommandBuffer);
            
            vk::Queue chosenQueue;
            if(useDedicatedTransferQueue) {
                chosenQueue = refCore->transferQueue().queue;
            }
            else {
                chosenQueue = refCore->graphicsQueue().queue;
            }

            chosenQueue.submit(1, &submitInfo, this->copyFinished);
        };

        bool isCopyFinished() {
            vk::Result status = this->copyFinished.getStatus();
            return (status == vk::Result::eSuccess);
        };

        bool waitForCopyFinished() {
            vk::Result status = refCore->device().waitForFences(*(this->copyFinished), true, UINT64_MAX);
            return (status == vk::Result::eSuccess);
        };

        void queueReceiveBarriers(vk::raii::CommandBuffer &graphicsCommandBuffer) {
            // Did we have a transfer queue?
            if(useDedicatedTransferQueue) {

                // Queue up barriers
                graphicsCommandBuffer.pipelineBarrier2(     
                    vk::DependencyInfo().setBufferMemoryBarriers(this->allReceiveBarriers));  
            }
        };

        string getCopyID() { return copyID; };            
    };


    class TransferManager {
    private:       
        unordered_map<string, BufferCopyReceipt> copiesInProgress;
        VulkanCore *refCore;         // Do NOT clean up!!!

    public:
        TransferManager(VulkanCore &vkCore) {
            // Store init data
            refCore = &vkCore;
        };

        ~TransferManager() {
            // Cleanup any residual copies,
            // but wait for completion first...
            for (auto& [id, receipt] : copiesInProgress) {
                receipt.waitForCopyFinished();
            }

            copiesInProgress.clear();     
        };

        bool submitCopies(string copyID, vector<PendingBufferCopy> &allPendingCopies) {
            // Does this ID exist already?
            auto receiptIt = copiesInProgress.find(copyID);

            if(receiptIt != copiesInProgress.end()) {
                print_error("TransferManager::submitCopies", "Copy ID already exists: " + copyID);
                return false;
            }
           
            // Create the buffer receipt
            BufferCopyReceipt receipt = BufferCopyReceipt(*refCore, copyID, allPendingCopies);
            
            // Add to copies in progress            
            copiesInProgress.insert_or_assign(copyID, move(receipt));   

            // Submit copies to (transfer?) queue
            copiesInProgress.at(copyID).submit();
            
            // Success
            return true;
        };

        bool checkCompleted(string copyID,
                            vk::raii::CommandBuffer &graphicsCommandBuffer) {

            auto receiptIt = copiesInProgress.find(copyID);

            // Does this ID exist? 
            if(receiptIt == copiesInProgress.end()) {
                print_and_throw_error("TransferManager::checkCompleted", "Invalid copy ID: " + copyID);                
            }

            BufferCopyReceipt &receipt = receiptIt->second;

            // Did this copy finish?
            if (!receipt.isCopyFinished()) {
                return false;
            }
                
            // Queue up receive barriers (for ending ownership handshake)
            receipt.queueReceiveBarriers(graphicsCommandBuffer);
            
            // Remove from list of pending copies                
            copiesInProgress.erase(receiptIt);

            // Good to go!
            return true;
        };

        bool checkAnyCompleted( vector<string> &allCopyIDs, 
                                vk::raii::CommandBuffer &graphicsCommandBuffer,
                                vector<string> &receiptsComplete) {
                                
            bool anyComplete = false;


            for(auto it = allCopyIDs.begin(); it != allCopyIDs.end(); ) {                
                string copyID = *it;
                if(checkCompleted(copyID, graphicsCommandBuffer)) {                    
                    it = allCopyIDs.erase(it);   
                    receiptsComplete.push_back(copyID);
                    anyComplete = true;             
                }
                else {
                    it++;
                }
            }

            return anyComplete;
        };

        void waitUntilCompleted(vector<string> &allCopyIDs) {    

            // Make graphics queue pool, buffer, and fence
            vk::raii::CommandPool blockGraphicsPool = createCommandPool(refCore->device(), refCore->graphicsQueue().index);
            vk::raii::CommandBuffer blockGraphicsBuffer = createCommandBuffer(refCore->device(), blockGraphicsPool);
            vk::raii::Fence blockFence = createFence(refCore->device(), vk::FenceCreateInfo());

            // Start recording            
            blockGraphicsBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

            // For each receipt...
            for(int i = 0; i < allCopyIDs.size(); i++) {
                auto copyID = allCopyIDs[i];                
                auto receiptIt = copiesInProgress.find(copyID);

                // Does this ID exist? 
                if(receiptIt == copiesInProgress.end()) {
                    print_and_throw_error("TransferManager::waitUntilCompleted", "Invalid copy ID: " + copyID);                
                }

                // Wait on fence...
                if(!receiptIt->second.waitForCopyFinished()) {
                    print_and_throw_error("TransferManager::waitUntilCompleted", "Failed to wait on " + copyID);                
                }
                
                // Queue up receive barriers (for ending ownership handshake)
                receiptIt->second.queueReceiveBarriers(blockGraphicsBuffer);                
            }

            // End recording
            blockGraphicsBuffer.end();

            // Submit...
            vk::SubmitInfo submitInfo{};
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &(*blockGraphicsBuffer);             
            refCore->graphicsQueue().queue.submit({submitInfo}, *blockFence);

            // Block until fence complete...
            auto result = refCore->device().waitForFences(*blockFence, true, UINT64_MAX);

            // Did wait fail?
            if (result != vk::Result::eSuccess) {
                print_and_throw_error(
                    "TransferManager::waitUntilCompleted", 
                    "Failed to wait for copy completion.");
            }

            // Safely release all these receipts
            for (string copyID : allCopyIDs) {
                copiesInProgress.erase(copyID);
            }

            // Clear out list of IDs
            allCopyIDs.clear();
        };
    };
}
