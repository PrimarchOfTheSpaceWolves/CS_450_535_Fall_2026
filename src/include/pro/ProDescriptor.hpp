#pragma once
#include "ProBuffer.hpp"

namespace pro {
    ///////////////////////////////////////////////////////////////////////////
    // STRUCTS
    ///////////////////////////////////////////////////////////////////////////

    struct DescriptorSetLayoutData {
        uint32_t setIndex = 0;
        vector<vk::DescriptorSetLayoutBinding> bindings {};
        vk::raii::DescriptorSetLayout layout = nullptr;

        DescriptorSetLayoutData() = default;
        ~DescriptorSetLayoutData() = default;

        // Copy: forbidden (unique ownership)
        DescriptorSetLayoutData(const DescriptorSetLayoutData&)            = delete;  // Copy constructor
        DescriptorSetLayoutData& operator=(const DescriptorSetLayoutData&) = delete;  // Copy assignment

        // Moving: OK
        DescriptorSetLayoutData(DescriptorSetLayoutData&&) noexcept            = default;  // Move
        DescriptorSetLayoutData& operator=(DescriptorSetLayoutData&&) noexcept = default;  // Move-assignment
    };

    ///////////////////////////////////////////////////////////////////////////
    // HELPER FUNCTIONS
    ///////////////////////////////////////////////////////////////////////////

    inline vector<vk::DescriptorSetLayout> extractLayoutsOnly(const vector<DescriptorSetLayoutData> &data) {
        vector<vk::DescriptorSetLayout> layouts {};
        layouts.resize(data.size());
        for(int i = 0; i < data.size(); i++) {
            layouts[i] = data[i].layout;
        }
        return layouts;
    };

    ///////////////////////////////////////////////////////////////////////////
    // CLASSES
    ///////////////////////////////////////////////////////////////////////////

    class DescriptorSetLayoutPack {
    private:
        vector<DescriptorSetLayoutData> allData {};
        
    public:
        DescriptorSetLayoutPack(const vk::raii::Device &device,
                                const vector<vector<vk::DescriptorSetLayoutBinding>> &allBindings,
                                const vector<vk::DescriptorSetLayoutBindingFlagsCreateInfo> &allBindingFlags = {}) {
                   
            // Create the actual descriptor set layouts
            allData.resize(allBindings.size());
            for(int i = 0; i < allBindings.size(); i++) {
                // Set index
                allData[i].setIndex = i;

                // Bindings
                allData[i].bindings.insert(allData[i].bindings.end(), allBindings[i].begin(), allBindings[i].end());
                
                // Creation info
                auto setCreationInfo = vk::DescriptorSetLayoutCreateInfo({}, allBindings[i]);

                // Set up extra flags if necessary
                if(allBindingFlags.size() > i && allBindingFlags[i].bindingCount > 0) {
                    setCreationInfo.pNext = &(allBindingFlags[i]);
                }

                // Actual layout
                allData[i].layout = vk::raii::DescriptorSetLayout(device, setCreationInfo);                 
            }
        };

        ~DescriptorSetLayoutPack() = default;

        // Copy: forbidden (unique ownership)
        DescriptorSetLayoutPack(const DescriptorSetLayoutPack&)            = delete;  // Copy constructor
        DescriptorSetLayoutPack& operator=(const DescriptorSetLayoutPack&) = delete;  // Copy assignment

        // Moving: OK
        DescriptorSetLayoutPack(DescriptorSetLayoutPack&&) noexcept            = default;  // Move
        DescriptorSetLayoutPack& operator=(DescriptorSetLayoutPack&&) noexcept = default;  // Move-assignment
        
        const vector<DescriptorSetLayoutData>& data() const noexcept { return allData; };        
    };
    

    class DescriptorSetPack {
    private:
        vk::raii::DescriptorPool allPool = nullptr;
        vector<vk::raii::DescriptorSet> allSets {};
        vector<vk::DescriptorSet> allSetHandles {};

    public:
        DescriptorSetPack(const vk::raii::Device &device,
                          const vector<DescriptorSetLayoutData> &layoutData) {

            // Compute necessary pool sizes
            unordered_map<vk::DescriptorType, uint32_t> typeCounts {};
            uint32_t totalSetCnt = layoutData.size();
            
            for(int i = 0; i < layoutData.size(); i++) {
                auto setBindings = layoutData[i].bindings;          
                
                for(auto oneSet : setBindings) {
                    uint32_t cnt = oneSet.descriptorCount;
                    if(typeCounts.contains(oneSet.descriptorType)) {
                        typeCounts.at(oneSet.descriptorType) += cnt;
                    }
                    else {
                        typeCounts.insert({oneSet.descriptorType, cnt});                        
                    }
                }
            }

            vector<vk::DescriptorPoolSize> poolSizes {};
            for(auto oneCount : typeCounts) {
                poolSizes.push_back(vk::DescriptorPoolSize(oneCount.first, oneCount.second));
            }

            // Create pool
            this->allPool = vk::raii::DescriptorPool(
                                device, 
                                vk::DescriptorPoolCreateInfo()
                                    .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
                                    .setPoolSizes(poolSizes)                    
                                    .setMaxSets(totalSetCnt));
            
            // Create actual sets  
            auto allLayouts = extractLayoutsOnly(layoutData);          
            allSets = device.allocateDescriptorSets(
                                    vk::DescriptorSetAllocateInfo()
                                        .setDescriptorPool(allPool)
                                        .setDescriptorSetCount(allLayouts.size())
                                        .setSetLayouts(allLayouts));  
                                        
            // Keep a list of the raw handles for later
            for(const auto& set : allSets) {
                allSetHandles.push_back(*set);
            }
        };

        ~DescriptorSetPack() = default;
        
        DescriptorSetPack(const DescriptorSetPack&) = delete;
        DescriptorSetPack& operator=(const DescriptorSetPack&) = delete;

        DescriptorSetPack(DescriptorSetPack&&) noexcept            = default;  // Move
        DescriptorSetPack& operator=(DescriptorSetPack&&) noexcept = default;  // Move-assignment

        void updateBuffer(  const vk::raii::Device &device,
                            int setIndex, int bindingIndex,
                            const VulkanBuffer &vbuffer,
                            vk::DescriptorType descType,
                            vk::DeviceSize offset = 0,
                            int descCnt = 1) {

            vector<vk::WriteDescriptorSet> writes;            
            vk::DescriptorBufferInfo bufferInfo = vk::DescriptorBufferInfo()
                                                            .setBuffer(vbuffer.buffer())
                                                            .setOffset(offset)
                                                            .setRange(vbuffer.size());
            vk::WriteDescriptorSet descWrite = vk::WriteDescriptorSet()
                .setDstSet(allSets[setIndex])
                .setDstBinding(bindingIndex)
                .setDstArrayElement(0)
                .setDescriptorType(descType)
                .setDescriptorCount(descCnt)
                .setBufferInfo(bufferInfo);
                
            device.updateDescriptorSets(descWrite, {}); 
        };

        void updateTexture( const vk::raii::Device &device,
                            int setIndex, int bindingIndex,
                            VulkanTexture &vtexture,
                            vk::DescriptorType descType = vk::DescriptorType::eCombinedImageSampler,
                            vk::DeviceSize offset = 0,
                            int descCnt = 1,
                            int arraySlot = 0) {

            vk::DescriptorImageInfo imageInfo(  vtexture.sampler, 
                                                vtexture.data.view,
                                                vk::ImageLayout::eShaderReadOnlyOptimal);

            vk::WriteDescriptorSet imageWrite = vk::WriteDescriptorSet()            
                .setDstSet(allSets[setIndex])
                .setDstBinding(bindingIndex)
                .setDescriptorCount(descCnt)
                .setDescriptorType(descType)
                .setImageInfo(imageInfo)
                .setDstArrayElement(arraySlot);

            device.updateDescriptorSets(imageWrite, {});             
        };

        const vector<vk::DescriptorSet>& sets() const noexcept { 
            return allSetHandles;
        };        
    };
}
