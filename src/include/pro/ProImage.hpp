#pragma once
#include "ProBuffer.hpp"

namespace pro {

    ///////////////////////////////////////////////////////////////////////////
    // ENUMS
    ///////////////////////////////////////////////////////////////////////////
    
    enum IMAGE_STATE_TYPE {
        UNDEF,
        COLOR,
        PRESENT,
        DEPTH,
        SRC_TRANSFER,
        DST_TRANSFER,
        TEXTURE        
    };

    ///////////////////////////////////////////////////////////////////////////
    // STRUCTS
    ///////////////////////////////////////////////////////////////////////////

    struct ImageState {
        vk::ImageLayout layout {};        
        vk::AccessFlags2 mask {};
        vk::PipelineStageFlags2 stage {};
    };
    
    struct PixelRGBA {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        uint8_t a = 1;

        PixelRGBA() = default;
        PixelRGBA(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) :
            r(red), g(green), b(blue), a(alpha) {};

        PixelRGBA(float red, float green, float blue, float alpha) {
            glm::vec4 color = glm::vec4(red, green, blue, alpha);
            color = glm::clamp(color, 0.0f, 1.0f);
            glm::u8vec4 ucolor = glm::u8vec4(glm::round(color * 255.0f));
            r = ucolor.r;
            g = ucolor.g;
            b = ucolor.b;
            a = ucolor.a;
        };            
    };

    struct StbImageDeleter {
        void operator()(PixelRGBA* data) const noexcept {
            stbi_image_free(data);
        }
    };

    using UniqueStbImageData = unique_ptr<PixelRGBA, StbImageDeleter>;
        
    struct HostImage {
        int imageWidth = 0;
        int imageHeight = 0;
        static constexpr int imageChannels = 4;          // Always 4
        UniqueStbImageData imageData; 
        
        HostImage() = default;
        
        HostImage(  int imageWidth, int imageHeight, 
                    PixelRGBA initColor = PixelRGBA(), 
                    bool debugPrint = true) {

            this->imageWidth = imageWidth;
            this->imageHeight = imageHeight;            
            PixelRGBA* data = reinterpret_cast<PixelRGBA*>(malloc(sizeof(PixelRGBA)*imageWidth*imageHeight));            
            imageData.reset(data);
            clear(initColor);
            
            if(debugPrint) {
                print_header_line();
                print_header_line('*', "IMAGE CREATED");                
                cout << "Dimensions: " << width() << " x " << height() << " x " << channels() << endl;
                cout << "Size: " << size() << endl;
                print_header_line();
            }
        };

        HostImage(string filepath, bool debugPrint = true) {
            int forcedChannels = 4;
            int origImageChannels;
            auto data = stbi_load(  filepath.c_str(), 
                                    &imageWidth, &imageHeight, 
                                    &origImageChannels, forcedChannels);
            
            imageData.reset(reinterpret_cast<PixelRGBA*>(data));

            if(!imageData) {
                print_and_throw_error("HostImage", "Image failed to load: " + filepath);                
            }

            if(debugPrint) {
                print_header_line();
                print_header_line('*', "IMAGE LOADED: " + filepath);                
                cout << "Dimensions: " << width() << " x " << height() << " x " << channels() << endl;
                cout << "Size: " << size() << endl;
                print_header_line();
            }
        };

        // Copy: forbidden (unique ownership)
        HostImage(const HostImage&)            = delete;  // Copy constructor
        HostImage& operator=(const HostImage&) = delete;  // Copy assignment

        // Moving: OK
        HostImage(HostImage&&) noexcept            = default;  // Move
        HostImage& operator=(HostImage&&) noexcept = default;  // Move-assignment

        // Destructor
        ~HostImage() = default;
    
        // Getters
        int width() const noexcept {  return imageWidth; };
        int height() const noexcept {  return imageHeight; };
        int channels() const noexcept {  return imageChannels; };
        uint8_t* data() const noexcept {  return reinterpret_cast<uint8_t*>(imageData.get()); };
        int count() const noexcept { return imageWidth*imageHeight; };
        size_t size() const noexcept {  return sizeof(PixelRGBA)*imageWidth*imageHeight; };
                
        bool in_bounds(int x, int y) const noexcept {
            return (x >= 0 && y >= 0 && x < imageWidth && y < imageHeight);
        };

        int get_pixel_index(int x, int y) const noexcept {
            return (y*imageWidth + x);
        };
        
        bool get(int x, int y, PixelRGBA &pixel) const noexcept {
            bool safe = in_bounds(x,y);
            if(safe) {
                auto *data = imageData.get();
                int index = get_pixel_index(x,y);
                pixel = data[index];
            }
            return safe;
        };

        // Setters        
        void clear(PixelRGBA clearColor) const noexcept {            
            fill_n(imageData.get(), count(), clearColor);            
        };

        bool set(int x, int y, PixelRGBA pixel) const noexcept {
            bool safe = in_bounds(x,y);
            if(safe) {
                auto *data = imageData.get();
                int index = get_pixel_index(x,y);
                data[index] = pixel;
            }
            return safe;
        };
    };

    struct VulkanImage {
        vma::raii::Image vmaImage = nullptr;
        vk::raii::ImageView view = nullptr;        
        vk::Format imageFormat {};
        vk::Extent3D imageExtent {};
        uint32_t imageMipLevels {1};
        vk::ImageAspectFlags imageAspectFlags {};

        VulkanImage() = default;
        ~VulkanImage() = default;

        // Copy: forbidden (unique ownership)
        VulkanImage(const VulkanImage&)            = delete;  // Copy constructor
        VulkanImage& operator=(const VulkanImage&) = delete;  // Copy assignment

        // Moving: OK
        VulkanImage(VulkanImage&&) noexcept            = default;  // Move
        VulkanImage& operator=(VulkanImage&&) noexcept = default;  // Move-assignment
                
        const vk::raii::Image& image() const noexcept { return vmaImage; };
        const vma::raii::Allocation& allocation() const noexcept { return vmaImage.getAllocation(); };  
        
        vk::Format format() const noexcept {  return imageFormat; };
        vk::Extent3D extent() const noexcept {  return imageExtent; };
        uint32_t mipLevels() const noexcept { return imageMipLevels; };
        vk::ImageAspectFlags aspectFlags() const noexcept { return imageAspectFlags; };
    };

    using VulkanImagePtr = std::shared_ptr<VulkanImage>;

    struct VulkanTexture {
        VulkanImage data {};
        vk::raii::Sampler sampler = nullptr;

        VulkanTexture() = default;
        ~VulkanTexture() = default;

        // Copy: forbidden (unique ownership)
        VulkanTexture(const VulkanTexture&)            = delete;  // Copy constructor
        VulkanTexture& operator=(const VulkanTexture&) = delete;  // Copy assignment

        // Moving: OK
        VulkanTexture(VulkanTexture&&) noexcept            = default;  // Move
        VulkanTexture& operator=(VulkanTexture&&) noexcept = default;  // Move-assignment
    };

    ///////////////////////////////////////////////////////////////////////////
    // FUNCTIONS 
    ///////////////////////////////////////////////////////////////////////////

    inline ImageState createImageState(IMAGE_STATE_TYPE state_type) {
        ImageState state {};

        switch(state_type) {
            case UNDEF: 
            {
                state.layout = vk::ImageLayout::eUndefined;
                state.mask = vk::AccessFlagBits2::eNone;
                state.stage = vk::PipelineStageFlagBits2::eTopOfPipe;
                break;
            }
            case COLOR: 
            {
                state.layout = vk::ImageLayout::eColorAttachmentOptimal;
                state.mask = vk::AccessFlagBits2::eColorAttachmentWrite;
                state.stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                break;
            }
            case PRESENT: 
            {
                state.layout = vk::ImageLayout::ePresentSrcKHR;
                state.mask = vk::AccessFlagBits2::eNone;
                state.stage = vk::PipelineStageFlagBits2::eBottomOfPipe;
                break;
            }
            case DEPTH: 
            {
                state.layout = vk::ImageLayout::eDepthAttachmentOptimal;
                state.mask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite
                             | vk::AccessFlagBits2::eDepthStencilAttachmentRead;
                state.stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests
                             | vk::PipelineStageFlagBits2::eLateFragmentTests;
                break;
            }
            case SRC_TRANSFER: 
            {
                state.layout = vk::ImageLayout::eTransferSrcOptimal;
                state.mask = vk::AccessFlagBits2::eTransferRead;
                state.stage = vk::PipelineStageFlagBits2::eTransfer;
                break;
            }
            case DST_TRANSFER: 
            {
                state.layout = vk::ImageLayout::eTransferDstOptimal;
                state.mask = vk::AccessFlagBits2::eTransferWrite;
                state.stage = vk::PipelineStageFlagBits2::eTransfer;
                break;
            }
            case TEXTURE:
            {
                state.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
                state.mask = vk::AccessFlagBits2::eShaderRead;
                state.stage = vk::PipelineStageFlagBits2::eFragmentShader;
                break;
            }
            default: {
                print_and_throw_error("createImageState", "Unknown image state type!");
            }        
        };

        return state;
    };

    inline vk::ImageMemoryBarrier2 createImageTransition(
        const vk::Image &image, 
        IMAGE_STATE_TYPE srcType, IMAGE_STATE_TYPE dstType,
        vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor,
        uint32_t startMipLevel = 0,
        uint32_t mipLevelCount = vk::RemainingMipLevels) {

        // Create image states
        auto src = createImageState(srcType);
        auto dst = createImageState(dstType);

        // Create the actual barrier
        vk::ImageMemoryBarrier2 barrier {};
        barrier.setOldLayout(src.layout);
        barrier.setSrcAccessMask(src.mask);
        barrier.setSrcStageMask(src.stage);

        barrier.setNewLayout(dst.layout);        
        barrier.setDstAccessMask(dst.mask);        
        barrier.setDstStageMask(dst.stage);

        barrier.setImage(image);

        barrier.setSubresourceRange(
            vk::ImageSubresourceRange(aspectFlags, startMipLevel, mipLevelCount, 0, 1));

        return barrier;
    }

    inline void performImageTransition(
        const vk::raii::CommandBuffer &commandBuffer, 
        vk::ImageMemoryBarrier2 &transition) {

        commandBuffer.pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(transition));
    };

    inline void performImageTransition(
        const vk::raii::CommandBuffer &commandBuffer, 
        const vk::Image &image, 
        IMAGE_STATE_TYPE srcType, IMAGE_STATE_TYPE dstType,
        vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor,
        uint32_t startMipLevel = 0,
        uint32_t mipLevelCount = vk::RemainingMipLevels) {

        vk::ImageMemoryBarrier2 transition = createImageTransition(image, srcType, dstType, aspectFlags, startMipLevel, mipLevelCount);
        performImageTransition(commandBuffer, transition);
    };
    
    inline vk::RenderingAttachmentInfoKHR createColorAttachment(
        const vk::ImageView &swapImageView,
        vk::ClearColorValue clearColor) {

        vk::RenderingAttachmentInfoKHR colorAtt {};
        colorAtt.setImageView(swapImageView)
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(clearColor);
        return colorAtt;
    };

    inline vk::RenderingAttachmentInfoKHR createDepthAttachment(
        const vk::ImageView &depthImageView,
        vk::ClearDepthStencilValue clearDepthStencil = vk::ClearDepthStencilValue {1.0f, 0}) {

        vk::RenderingAttachmentInfoKHR depthAtt {};
        depthAtt.setImageView(depthImageView)
                .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setClearValue(clearDepthStencil);
        return depthAtt;
    };

    inline VulkanImage createVulkanImage(   
        const vk::raii::Device &device,
        const vma::raii::Allocator &allocator,
        vk::ImageCreateInfo imageCreateInfo,
        vk::ImageAspectFlags aspectFlags) {                                    

        // Create struct
        VulkanImage imageData {};
        imageData.imageExtent = imageCreateInfo.extent;
        imageData.imageFormat = imageCreateInfo.format;
        imageData.imageMipLevels = imageCreateInfo.mipLevels;
                        
        // Want data on device
        vma::AllocationCreateInfo allocInfo {};
        allocInfo.usage = vma::MemoryUsage::eAutoPreferDevice;

        // Actually allocate data
        vma::raii::Image image = allocator.createImage(imageCreateInfo, allocInfo);       

        // Save VMA image (which includes allocation)
        imageData.vmaImage = move(image);
        
        // Also create image view while we're here
        auto viewInfo = vk::ImageViewCreateInfo()
                            .setImage(*(imageData.image()))                                            
                            .setViewType(vk::ImageViewType::e2D)
                            .setFormat(imageCreateInfo.format)
                            .setSubresourceRange({ aspectFlags, 0, imageCreateInfo.mipLevels, 0, 1 });
                                     
        imageData.view = vk::raii::ImageView(device, viewInfo);
        imageData.imageAspectFlags = aspectFlags;

        // Return data
        return imageData;
    };

    inline VulkanTexture createVulkanTexture(   
        const vk::raii::Device &device,
        const vma::raii::Allocator &allocator,
        vk::ImageCreateInfo imageCreateInfo,
        vk::ImageAspectFlags aspectFlags,
        vk::SamplerCreateInfo samplerInfo) {

        VulkanTexture texture {};
        texture.data = createVulkanImage(device, allocator, imageCreateInfo, aspectFlags);
        texture.sampler = vk::raii::Sampler(device, samplerInfo);        
        return texture;
    };

    inline vk::ImageCreateInfo getTypicalTextureImageCreateInfo(
        uint32_t width, uint32_t height, 
        vk::Format format = vk::Format::eR8G8B8A8Srgb, //vk::Format::eR8G8B8A8Unorm,
        int mipLevels = 1) {

        return vk::ImageCreateInfo()
                .setImageType(vk::ImageType::e2D)
                .setExtent({width, height, 1})
                .setFormat(format)
                .setUsage(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
                .setArrayLayers(1)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setMipLevels(mipLevels)
                .setTiling(vk::ImageTiling::eOptimal)
                .setSharingMode(vk::SharingMode::eExclusive);
    };

    inline vk::SamplerCreateInfo getTypicalTextureSamplerCreateInfo(
        vk::Filter filter,
        vk::SamplerAddressMode addressMode,
        vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear,
        float minLod = 0.0f,
        float maxLod = 0.0f,
        float lodBias = 0.0f,
        bool enableAnisotropy = false,
        float maxAnisotrophy = 0.0f,
        bool enableCompare = false,
        vk::CompareOp compareOp = vk::CompareOp::eNever
    ) {
        return vk::SamplerCreateInfo()
                .setMagFilter(filter)
                .setMinFilter(filter)
                .setMipmapMode(mipmapMode)
                .setAddressModeU(addressMode)
                .setAddressModeV(addressMode)
                .setAddressModeW(addressMode)
                .setMinLod(minLod)
                .setMaxLod(maxLod)          // vk::LodClampNone for unrestricted
                .setMipLodBias(lodBias)
                .setAnisotropyEnable(enableAnisotropy)
                .setMaxAnisotropy(maxAnisotrophy)       // properties.limits.maxSamplerAnisotropy
                .setCompareEnable(enableCompare)
                .setCompareOp(compareOp);
    };  
    
    inline vk::ImageCreateInfo getTypicalDepthImageCreateInfo(
        uint32_t width, uint32_t height, 
        vk::Format format = vk::Format::eD32Sfloat) {

        return vk::ImageCreateInfo()
                .setImageType(vk::ImageType::e2D)
                .setExtent({width, height, 1})
                .setFormat(format)
                .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
                .setArrayLayers(1)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setMipLevels(1)
                .setTiling(vk::ImageTiling::eOptimal)
                .setSharingMode(vk::SharingMode::eExclusive);
    };
    
    inline void recordCopyBufferToTexture(
        const vk::raii::CommandBuffer &commandBuffer,
        const pro::VulkanBuffer &srcBuffer,
        const pro::VulkanTexture &dstTexture,
        bool firstTime = true) {

        // Set up the transition
        IMAGE_STATE_TYPE srcType = UNDEF;
        if(!firstTime) {
            srcType = TEXTURE;
        }
        IMAGE_STATE_TYPE dstType = DST_TRANSFER;
        
        // Transition image for target copy
        performImageTransition(commandBuffer, dstTexture.data.image(), srcType, dstType);

        // Perform copy
        commandBuffer.copyBufferToImage(
            srcBuffer.buffer(),
            dstTexture.data.image(),
            vk::ImageLayout::eTransferDstOptimal,
            vk::BufferImageCopy()
                .setImageExtent(dstTexture.data.extent())
                .setImageSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1}));

        // Transition back to texture
        performImageTransition(commandBuffer, dstTexture.data.image(), 
                                IMAGE_STATE_TYPE::DST_TRANSFER, IMAGE_STATE_TYPE::TEXTURE);
    };    

    inline uint32_t computeMaxMipLevels(int width, int height) {
        return static_cast<uint32_t>(floor(log2(max(width, height)))) + 1;
    };

    inline float getMaxAnisotrophy(const vk::raii::PhysicalDevice &physicalDevice) {
        vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
        return properties.limits.maxSamplerAnisotropy;
    };

    // Adapted from: https://docs.vulkan.org/tutorial/latest/09_Generating_Mipmaps.html
    inline void generateMipmaps(
        const vk::raii::PhysicalDevice &physicalDevice,
        const vk::raii::CommandBuffer &commandBuffer, 
        const VulkanImage &vimage) {
	
		// Check if image format supports linear blitting
		vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(vimage.format());
		if (!(formatProperties.optimalTilingFeatures 
                & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
                    print_and_throw_error("generateMipmaps", "Texture image format does not support linear blitting!");
        }
		
        // Create the baseline barrier to prepare image to be READ from
        vk::ImageMemoryBarrier2 readBarrier = createImageTransition(
                                                vimage.image(), 
                                                DST_TRANSFER, SRC_TRANSFER,
                                                vk::ImageAspectFlagBits::eColor,
                                                0, 1);

        // Create the baseline barrier to prepare the level to be a TEXTURE
        vk::ImageMemoryBarrier2 textureBarrier = createImageTransition(
                                                    vimage.image(), 
                                                    SRC_TRANSFER, TEXTURE,
                                                    vk::ImageAspectFlagBits::eColor,
                                                    0, 1);

        // Set starting width and height
		int32_t mipWidth  = vimage.extent().width;
		int32_t mipHeight = vimage.extent().height;

        // For each mip level...
		for (uint32_t i = 1; i < vimage.mipLevels(); i++) {
            // Point to correct level
            readBarrier.subresourceRange.baseMipLevel = i - 1;
            textureBarrier.subresourceRange.baseMipLevel = i - 1;
            
            // Do image transition for READING
            performImageTransition(commandBuffer, readBarrier);

            // Get destination size
            int32_t dstMipWidth = 1 < mipWidth ? mipWidth / 2 : 1;
            int32_t dstMipHeight = 1 < mipHeight ? mipHeight / 2 : 1;
            
            // Set up blit information
            // (from (i-1) to (i))
			vk::ImageBlit2 blitRegions = vk::ImageBlit2()
                                    .setSrcSubresource({vk::ImageAspectFlagBits::eColor, i - 1, 0, 1})
                                    .setSrcOffsets(array<vk::Offset3D, 2>({{}, {mipWidth, mipHeight, 1}}))
                                    .setDstSubresource({vk::ImageAspectFlagBits::eColor, i, 0, 1})
                                    .setDstOffsets(array<vk::Offset3D, 2>({{}, {dstMipWidth, dstMipHeight, 1}}));

            vk::BlitImageInfo2 blitInfo = vk::BlitImageInfo2()
                                            .setSrcImage(vimage.image())
                                            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
                                            .setFilter(vk::Filter::eLinear)
                                            .setRegions(blitRegions)
                                            .setDstImage(vimage.image())
                                            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal);

            // Actually perform the blit
			commandBuffer.blitImage2(blitInfo);

            // Do image transition for TEXTURE
            performImageTransition(commandBuffer, textureBarrier);
            
            // Update current mip level width and height
			if (1 < mipWidth) {
				mipWidth /= 2;
			}

			if (1 < mipHeight) {
				mipHeight /= 2;
			}
		}

        // Do the last texture transition
        vk::ImageMemoryBarrier2 finaltextureBarrier = createImageTransition(
                                                        vimage.image(), 
                                                        DST_TRANSFER, TEXTURE,
                                                        vk::ImageAspectFlagBits::eColor,
                                                        vimage.mipLevels() - 1, 
                                                        1);        
        performImageTransition(commandBuffer, finaltextureBarrier);
	};
}

