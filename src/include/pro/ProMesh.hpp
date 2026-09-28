#pragma once
#include "ProBuffer.hpp"
#include "ProTransfer.hpp"

namespace pro {
    ///////////////////////////////////////////////////////////////////////////
    // STRUCTS 
    ///////////////////////////////////////////////////////////////////////////
        
    template<typename T>
    struct HostMesh {
        vector<T> vertices {};
        vector<unsigned int> indices {};
    };
    
    struct VulkanMesh {
        VulkanBufferPtr vertices_;
        VulkanBufferPtr indices_;
        unsigned int indexCnt = 0;
        unsigned int vertexCnt = 0;

        const VulkanBuffer& vertices() const noexcept { return *vertices_; };
        const VulkanBuffer& indices() const noexcept { return *indices_; };

        VulkanBuffer& vertices() noexcept { return *vertices_; };
        VulkanBuffer& indices() noexcept { return *indices_; };

        const VulkanBufferPtr& verticesPtr() const noexcept { return vertices_; }
        const VulkanBufferPtr& indicesPtr()  const noexcept { return indices_; }

        VulkanBufferPtr& verticesPtr() noexcept { return vertices_; }
        VulkanBufferPtr& indicesPtr()  noexcept { return indices_; }
    };
        
    ///////////////////////////////////////////////////////////////////////////
    // FUNCTIONS 
    ///////////////////////////////////////////////////////////////////////////  

    template<typename T>
    VulkanMesh createVulkanMesh(    const vma::raii::Allocator &allocator,                           
                                    HostMesh<T> &hostMesh,
                                    bool isDeviceLocal,
                                    // Applications can request additional uses,
                                    // such as acceleration-structure input and
                                    // shader device-address access.
                                    vk::BufferUsageFlags extraFlags = {}) {
        // Set up Vulkan mesh                            
        VulkanMesh mesh;

        // Create appropriate VMA info and usage flags
        vma::AllocationCreateInfo vmaInfo {};
        vk::BufferUsageFlags vertUsageFlags = vk::BufferUsageFlagBits::eVertexBuffer;
        vk::BufferUsageFlags indexUsageFlags = vk::BufferUsageFlagBits::eIndexBuffer;

        if(isDeviceLocal) {
            vmaInfo = createVMADeviceLocalInfo();
            vertUsageFlags |= vk::BufferUsageFlagBits::eTransferDst;
            indexUsageFlags |= vk::BufferUsageFlagBits::eTransferDst;
        }
        else {
            vmaInfo = createVMAHostVisibleInfo();
        }

        // Add on extra flags
        vertUsageFlags |= extraFlags;
        indexUsageFlags |= extraFlags;

        // Create vertex buffer and index buffer
        vk::DeviceSize vertBufferSize = sizeof(hostMesh.vertices[0]) * hostMesh.vertices.size();    
        mesh.vertices_ = make_shared<VulkanBuffer>(createVulkanBuffer(allocator, vertBufferSize, vertUsageFlags, vmaInfo));

        vk::DeviceSize indexBufferSize = sizeof(hostMesh.indices[0]) * hostMesh.indices.size();
        mesh.indices_ = make_shared<VulkanBuffer>(createVulkanBuffer(allocator, indexBufferSize, indexUsageFlags, vmaInfo));

        // Return mesh
        return mesh;
    };

    template<typename T>
    void copyToHostVisibleVulkanMesh(   const vma::raii::Allocator &allocator,
                                        VulkanMesh &mesh,                                   
                                        HostMesh<T> &hostMesh) {
        
        // Copy to buffers
        copyToHostVisibleVulkanBuffer(allocator, mesh.vertices(), hostMesh.vertices.data());
        copyToHostVisibleVulkanBuffer(allocator, mesh.indices(), hostMesh.indices.data());
        
        // Set index and vertex count
        mesh.indexCnt = hostMesh.indices.size();
        mesh.vertexCnt = hostMesh.vertices.size();
    };

    template<typename T>
    void addPendingBufferCopies (   VulkanMesh &mesh,                                   
                                    HostMesh<T> &hostMesh,
                                    vector<PendingBufferCopy> &pendingCopies) {

        pendingCopies.push_back(PendingBufferCopy(  mesh.verticesPtr(), 
                                                    hostMesh.vertices.data(), 
                                                    vk::PipelineStageFlagBits2::eVertexInput, 
                                                    vk::AccessFlagBits2::eVertexAttributeRead));

        pendingCopies.push_back(PendingBufferCopy(  mesh.indicesPtr(), 
                                                    hostMesh.indices.data(), 
                                                    vk::PipelineStageFlagBits2::eVertexInput, 
                                                    vk::AccessFlagBits2::eIndexRead));
        
        // Still set index and vertex count
        mesh.indexCnt = hostMesh.indices.size();
        mesh.vertexCnt = hostMesh.vertices.size();
    };

    void recordDrawVulkanMesh(const vk::raii::CommandBuffer &commandBuffer, VulkanMesh &mesh) {
        
        vk::Buffer vertexBuffers[] = {mesh.vertices().buffer()};
        vk::DeviceSize offsets[] = {0};
        commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
        commandBuffer.bindIndexBuffer(mesh.indices().buffer(), 0, vk::IndexType::eUint32);
        
        commandBuffer.drawIndexed(mesh.indexCnt, 1, 0, 0, 0);
    };   
}
