#pragma once
#include "ProCommand.hpp"
#include "ProBuffer.hpp"
#include "ProImage.hpp"

namespace pro {

    ///////////////////////////////////////////////////////////////////////////
    // FUNCTIONS 
    ///////////////////////////////////////////////////////////////////////////  
        
    inline VulkanBuffer createStagingBuffer(    const vma::raii::Allocator &allocator,
                                                vk::DeviceSize bufferSize, 
                                                void *hostData = nullptr) {

        // Create host-visible staging buffer (TRANSFER_SRC)
        VulkanBuffer stageBuffer = createVulkanBuffer(  allocator, 
                                                        bufferSize,
                                                        vk::BufferUsageFlagBits::eTransferSrc,                                                        
                                                        createVMAHostVisibleInfo());

        // Copy host data into staging buffer if available
        if(hostData) {
            copyToHostVisibleVulkanBuffer(allocator, stageBuffer, hostData);
        }

        // Return staging buffer
        return stageBuffer;
    }; 
    
    inline void copyToDeviceLocalBuffer(  const vk::raii::CommandBuffer &commandBuffer,
                                          const VulkanBuffer &stagingBuffer,
                                          const VulkanBuffer &dstBuffer) {



        vk::BufferCopy copyRegion{};
        copyRegion.size = dstBuffer.size();
        commandBuffer.copyBuffer(stagingBuffer.buffer(), dstBuffer.buffer(), {copyRegion});
    };

    inline void copyToDeviceLocalImage( const vk::raii::CommandBuffer &commandBuffer,
                                        const VulkanBuffer &stagingBuffer,
                                        const VulkanImage &dstImage) {

        vk::BufferImageCopy copyRegion {};
        copyRegion.imageSubresource = vk::ImageSubresourceLayers(
                                        dstImage.aspectFlags(), 0, 0, 1);
        copyRegion.imageExtent = dstImage.extent();
        
        commandBuffer.copyBufferToImage(
            stagingBuffer.buffer(),
            dstImage.image(),
            vk::ImageLayout::eTransferDstOptimal,
            copyRegion);
    };
}
