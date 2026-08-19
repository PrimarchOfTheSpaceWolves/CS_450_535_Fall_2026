#pragma once
#include "ProBase.hpp"

namespace pro {
    
    ///////////////////////////////////////////////////////////////////////////
    // STRUCTS 
    ///////////////////////////////////////////////////////////////////////////

    struct VulkanBuffer {
        vma::raii::Buffer vmaBuffer = nullptr;    
        vk::DeviceSize bufferSize {};
        vk::BufferUsageFlags bufferUsage {};       
        void* bufferMapped = nullptr;     

        VulkanBuffer() = default;
        ~VulkanBuffer() = default;

        // Copy: forbidden (unique ownership)
        VulkanBuffer(const VulkanBuffer&)            = delete;  // Copy constructor
        VulkanBuffer& operator=(const VulkanBuffer&) = delete;  // Copy assignment

        // Moving: OK
        VulkanBuffer(VulkanBuffer&&) noexcept            = default;  // Move
        VulkanBuffer& operator=(VulkanBuffer&&) noexcept = default;  // Move-assignment
                
        const vk::raii::Buffer& buffer() const noexcept { return vmaBuffer; };
        const vma::raii::Allocation& allocation() const noexcept { return vmaBuffer.getAllocation(); };  
        
        vk::DeviceSize size() const noexcept {  return bufferSize; };
        vk::BufferUsageFlags usage() const noexcept {  return bufferUsage; };
        void* mapped() const noexcept { return bufferMapped; };
    };

    using VulkanBufferPtr = std::shared_ptr<VulkanBuffer>;

    ///////////////////////////////////////////////////////////////////////////
    // COMMON DEFAULTS (HELPER FUNCTIONS) 
    ///////////////////////////////////////////////////////////////////////////

    inline vma::AllocationCreateInfo createVMAHostVisibleInfo() {
        vma::AllocationCreateInfo vci{};
        vci.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | // Written sequentially (like with memcopy)
                    vma::AllocationCreateFlagBits::eMapped;                     // Persistently mapped        
        vci.usage = vma::MemoryUsage::eAuto;
        return vci;
    };

    inline vma::AllocationCreateInfo createVMADeviceLocalInfo() {
        vma::AllocationCreateInfo vci{};
        vci.usage = vma::MemoryUsage::eAutoPreferDevice;
        return vci;
    };

    ///////////////////////////////////////////////////////////////////////////
    // FUNCTIONS 
    ///////////////////////////////////////////////////////////////////////////  

    inline VulkanBuffer createVulkanBuffer( const vma::raii::Allocator &allocator,
                                            vk::DeviceSize bufferSize,
                                            vk::BufferUsageFlags bufferUsage,                                                                               
                                            vma::AllocationCreateInfo vmaAllocCreateInfo,
                                            vk::SharingMode bufferSharingMode = vk::SharingMode::eExclusive) {
        
        // Set buffer creation info
        vk::BufferCreateInfo bufferCreateInfo {};
        bufferCreateInfo.setSize(bufferSize);
        bufferCreateInfo.setUsage(bufferUsage);
        bufferCreateInfo.setSharingMode(bufferSharingMode);

        // Create the (VMA) buffer
        vma::AllocationInfo allocInfo {};
        vma::raii::Buffer buffer = allocator.createBuffer( bufferCreateInfo, 
                                                           vmaAllocCreateInfo, 
                                                           allocInfo);
        
        // Create output struct
        VulkanBuffer out {};
        out.vmaBuffer = move(buffer);
        out.bufferSize = bufferCreateInfo.size;
        out.bufferUsage = bufferCreateInfo.usage;        
        out.bufferMapped = allocInfo.pMappedData; 
        
        return out;
    };

    inline void copyToHostVisibleVulkanBuffer(  const vma::raii::Allocator &allocator,
                                                VulkanBuffer &bufferData,
                                                void *hostData) {

        memcpy(bufferData.mapped(), hostData, bufferData.size());
        allocator.flushAllocations({bufferData.allocation()}, {0}, {vk::WholeSize});
    };
}
