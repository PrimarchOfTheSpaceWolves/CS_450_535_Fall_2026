#include <iostream>
#include <string>
#define PRO_USE_GLFW
#include "pro/Prometheus.hpp"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// STRUCTS
///////////////////////////////////////////////////////////////////////////////

struct ProVertex {
    glm::vec3 pos;    
    glm::vec2 texcoord;
};

glm::vec2 lastMousePos {};

///////////////////////////////////////////////////////////////////////////////
// GLOBALS
///////////////////////////////////////////////////////////////////////////////

bool didWindowResize = false;

///////////////////////////////////////////////////////////////////////////////
// GLFW CALLBACKS
///////////////////////////////////////////////////////////////////////////////

// Note: static prevents name conflicts in other cpp files

// When the window resizes/minimizes...
static void window_resize_callback(GLFWwindow* window, int width, int height) {
    didWindowResize = true;    
}

// When key events occur...
static void key_callback(   GLFWwindow *window,
                            int key,
                            int scancode,
                            int action,
                            int mods) {

    if(action == GLFW_PRESS || action == GLFW_REPEAT) {
        if(key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        }        
    }
}

// When the mouse moves...
static void mouse_position_callback(GLFWwindow* window, double xpos, double ypos) {
    glm::vec2 mousePos = glm::vec2(xpos, ypos);
    lastMousePos = mousePos;
}

// When a mouse button is pressed/released/clicked...
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        cout << "Left mouse press." << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////
// HOST-SIDE "DRAWING" FUNCTION
///////////////////////////////////////////////////////////////////////////////

void renderToFramebuffer(const pro::HostImage &hostFramebuffer) {
    // Use last mouse X position to shift sine wave
    const float periodPixelWidth = 200.0f;    
    float mouseX = lastMousePos.x;
    
    for(int x = 0; x < hostFramebuffer.width(); x++) {
        // Get current "angle"
        float position = (x + mouseX)/periodPixelWidth;
        float angle = 2.0f*glm::pi<float>()*position;
        // Calculate color for column
        float red = (1.0f + glm::sin(angle))/2.0f;
        float green = 0.0f;
        float blue = 0.0f;
        pro::PixelRGBA pixel = pro::PixelRGBA(red, green, blue, 1.0f);

        for(int y = 0; y < hostFramebuffer.height(); y++) {        
            hostFramebuffer.set(x, y, pixel);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// PER-FRAME "DRAWING" FUNCTION
///////////////////////////////////////////////////////////////////////////////

void recordFrame(   const pro::VulkanCore &vkCore, 
                    const pro::CommandData &cd,
                    const pro::VulkanSwapImage &swapImage,
                    const pro::VulkanImage &depthImage,
                    pro::VulkanPipelineData &pipelineData,
                    bool updateFramebuffer,
                    pro::HostImage &hostFramebuffer,
                    pro::VulkanBuffer &screenStagingBuffer,
                    pro::VulkanTexture &screenTexture,
                    bool firstScreenTransfer,
                    pro::VulkanMesh &screenQuadMesh,                    
                    pro::DescriptorSetPack &descSetPack) {

    // Begin recording
    cd.beginRecording();

    // Did the host image change?
    if(updateFramebuffer) {
        // Copy to staging buffer
        pro::copyToHostVisibleVulkanBuffer( vkCore.allocator(), 
                                            screenStagingBuffer, 
                                            hostFramebuffer.data());
            
        // Copy to texture
        pro::recordCopyBufferToTexture( cd.buffer(), 
                                        screenStagingBuffer, 
                                        screenTexture, 
                                        firstScreenTransfer);
    }

    // Transition swap image from undefined to color buffer
    pro::performImageTransition(cd.buffer(), swapImage.image, 
                                pro::IMAGE_STATE_TYPE::UNDEF,
                                pro::IMAGE_STATE_TYPE::COLOR);

    // Define behavior for the color attachment (including clear color)
    vk::RenderingAttachmentInfoKHR colorAtt = pro::createColorAttachment(
        swapImage.view, 
        vk::ClearColorValue {0.0f, 1.0f, 1.0f, 1.0f});

    // Define behavior for the depth attachment
    vk::RenderingAttachmentInfoKHR depthAtt = pro::createDepthAttachment(depthImage.view);
        
    // Set rendering info and begin (dynamic) rendering
    vk::RenderingInfoKHR ri{};
    ri.setRenderArea(vk::Rect2D{ {0,0}, vkCore.swapchain().extent })
        .setLayerCount(1)
        .setColorAttachments(colorAtt)
        .setPDepthAttachment(&depthAtt);

    cd.buffer().beginRendering(ri);
    
    // Bind pipeline
    cd.buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, pipelineData.pipeline);
    
    // Set up viewport and scissors
    vk::Viewport viewports[] = { pro::makeDefaultViewport(vkCore) };    
    cd.buffer().setViewport(0, viewports);
    
    vk::Rect2D scissors[] = { pro::makeDefaultScissors(vkCore) };
    cd.buffer().setScissor(0, scissors);

    // Bind the descriptor sets
    cd.buffer().bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, 
        *pipelineData.layout,
        0, descSetPack.sets(), nullptr);
    
    // Render fullscreen quad
    pro::recordDrawVulkanMesh(cd.buffer(), screenQuadMesh);
        
    // End rendering
    cd.buffer().endRendering();
   
    // Transition swap image from color buffer to presentation
    pro::performImageTransition(cd.buffer(), swapImage.image, 
                                pro::IMAGE_STATE_TYPE::COLOR,
                                pro::IMAGE_STATE_TYPE::PRESENT);
                                
    // End recording
    cd.endRecording();
}

///////////////////////////////////////////////////////////////////////////////
// MAIN FUNCTION
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
    cout << "BEGIN PROGRAM..." << endl;

    // Set app name
    string appName = "SoftwareRenderer";
    string windowName = appName + ": realemj";
        
    ///////////////////////////////////////////////////////////////////////
    // GLFW
    ///////////////////////////////////////////////////////////////////////

    // Initialize GLFW
    if(!glfwInit()) {
        cerr << "ERROR: Cannot start GLFW!" << endl;
        exit(1);
    }
    
    // Create GLFW window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, true);
    GLFWwindow *window = glfwCreateWindow(800, 600, windowName.c_str(), nullptr, nullptr);

    // Was window successfully created?
    if(!window) {
        cerr << "ERROR: Cannot create GLFW window!" << endl;
        glfwTerminate();
        exit(1);
    }

    // Define GLFW callback functions
    glfwSetFramebufferSizeCallback(window, window_resize_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    lastMousePos = glm::vec2(xpos, ypos);    

    // Create scope for Vulkan Init Data (to ensure proper cleanup)
    {
        ///////////////////////////////////////////////////////////////////////
        // VULKAN INIT DATA
        ///////////////////////////////////////////////////////////////////////

        // Creation information for basic Vulkan components
        pro::VulkanCoreCreateInfo createInfo {};
        createInfo.appName = appName;
        // If you encounter errors with instance creation, try requesting Vulkan 1.3:
        //createInfo.deviceRequirements.minMinorVersion = 3;
        
        // If you encounter errors with compute and/or transfer queue creation, try these:
        //createInfo.deviceRequirements.requireComputeQueue = false;
        //createInfo.deviceRequirements.requireTransferQueue = false;
        
        // Set GLFW functions
        prepareVulkanInitGLFWFunctions(createInfo, window);
    
        // Create the basic Vulkan components
        pro::VulkanCore vkCore(createInfo);
                
        ///////////////////////////////////////////////////////////////////////
        // VULKAN COMMAND DATA
        ///////////////////////////////////////////////////////////////////////
        
        // Create command data
        pro::CommandData frameCommandData = pro::createFrameCommandData(vkCore);

        ///////////////////////////////////////////////////////////////////////
        // VULKAN GRAPHICS PIPELINE
        ///////////////////////////////////////////////////////////////////////

        // Set up creation info for pipeline
        pro::VulkanPipelineCreateInfo pipelineCreateInfo(vkCore);

        // Create shader info
        pipelineCreateInfo.shaderInfo = {
            pro::VulkanShaderCreateInfo(
                "build/compiledshaders/" + appName + "/shader.vert.spv",
                vk::ShaderStageFlagBits::eVertex
            ),

            pro::VulkanShaderCreateInfo(
                "build/compiledshaders/" + appName + "/shader.frag.spv",
                vk::ShaderStageFlagBits::eFragment
            )
        };

        // Set up vertex information
        pipelineCreateInfo.bindDesc = vk::VertexInputBindingDescription(
            0, sizeof(ProVertex), vk::VertexInputRate::eVertex);

        // POSITION
        pipelineCreateInfo.attribDesc.push_back(vk::VertexInputAttributeDescription(
            0, // location
            0, // binding
            vk::Format::eR32G32B32Sfloat,  // format
            offsetof(ProVertex, pos) // offset
        ));
        
        // TEXTURE COORDINATES
        pipelineCreateInfo.attribDesc.push_back(vk::VertexInputAttributeDescription(
            1, // location
            0, // binding
            vk::Format::eR32G32Sfloat,  // format
            offsetof(ProVertex, texcoord) // offset
        ));
        
        // Layout bindings
        pro::DescriptorSetLayoutPack layoutPack = pro::DescriptorSetLayoutPack(
            vkCore.device(),
            {                
                {
                    vk::DescriptorSetLayoutBinding(
                        0, vk::DescriptorType::eCombinedImageSampler,
                        1, vk::ShaderStageFlagBits::eFragment)
                }
            });            
 
        pipelineCreateInfo.allDescSetLayouts = pro::extractLayoutsOnly(layoutPack.data());

        // Actually create the pipeline data
        pro::VulkanPipelineData pipelineData = createVulkanPipeline(vkCore.device(), pipelineCreateInfo);
        
        ///////////////////////////////////////////////////////////////////////
        // MESH CREATION
        ///////////////////////////////////////////////////////////////////////

        // Create host data  
    
        // Just need a fullscreen quad
        pro::HostMesh<ProVertex> screenQuad {};
        float slen = 1.0f;
        screenQuad.vertices = {
            {{-slen, -slen, 0.5f},  {0,0}},
            {{slen, -slen, 0.5f},   {1,0}},
            {{slen, slen, 0.5f},    {1,1}},
            {{-slen, slen, 0.5f},   {0,1}}
        };
        screenQuad.indices = { 0, 1, 2, 0, 2, 3 };      
        
        // Create the Vulkan mesh for the quad
        pro::VulkanMesh screenQuadMesh = pro::createVulkanMesh(vkCore.allocator(), screenQuad, false);
        pro::copyToHostVisibleVulkanMesh(vkCore.allocator(), screenQuadMesh, screenQuad);            
                
        ///////////////////////////////////////////////////////////////////////
        // DESCRIPTOR SET CREATION
        ///////////////////////////////////////////////////////////////////////

        // Creating only one group of descriptor set packs
        pro::DescriptorSetPack descSetPack = pro::DescriptorSetPack(
            vkCore.device(), layoutPack.data()
        );

        ///////////////////////////////////////////////////////////////////////
        // TEXTURE CREATION
        ///////////////////////////////////////////////////////////////////////

        // Initially need to update everything
        bool updateFramebuffer = true;

        // First time to transfer
        bool firstTimeFramebuffer = true;

        // Necessary image, buffer, and texture
        pro::HostImage hostFramebuffer;
        pro::VulkanBuffer screenStagingBuffer;
        pro::VulkanTexture screenTexture;

        // Since we should resize the image and texture on a window resize,
        // we wrap this into a resize function
        pro::OnResizeFunc resizeFullscreenImages = [
            &vkCore,
            &updateFramebuffer,
            &firstTimeFramebuffer,
            window,
            &hostFramebuffer,
            &screenStagingBuffer,
            &screenTexture,
            &descSetPack
        ]() {

            // Reset the booleans
            updateFramebuffer = true;            
            firstTimeFramebuffer = true;

            // Recreate blank host image
            int screenWidth, screenHeight;
            glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
            hostFramebuffer = pro::HostImage(screenWidth, screenHeight);

            // Fill it with a sold color to start
            hostFramebuffer.clear({1.0f, 1.0f, 0.0f, 1.0f});

            // Create staging buffer        
            screenStagingBuffer = pro::createStagingBuffer(vkCore.allocator(), 
                                                                hostFramebuffer.size(), 
                                                                hostFramebuffer.data());
            
            // Create the texture
            screenTexture = pro::createVulkanTexture(
                vkCore.device(), vkCore.allocator(), 
                pro::getTypicalTextureImageCreateInfo(hostFramebuffer.width(), hostFramebuffer.height()),
                vk::ImageAspectFlagBits::eColor,
                pro::getTypicalTextureSamplerCreateInfo(
                    vk::Filter::eNearest, 
                    vk::SamplerAddressMode::eClampToEdge, 
                    vk::SamplerMipmapMode::eNearest));

            // Update descriptor set
            descSetPack.updateTexture(vkCore.device(), 0, 0, screenTexture);
        };

        // Set resize function for later
        vkCore.setOnResizeFunc(resizeFullscreenImages);

        // Run it at least once to set things up
        resizeFullscreenImages();
        
        ///////////////////////////////////////////////////////////////////////
        // MAIN RENDER LOOP
        ///////////////////////////////////////////////////////////////////////
        
        // While the window is still open...
        while (!glfwWindowShouldClose(window)) { 
            // Check for window/keyboard/mouse events...	
            glfwPollEvents();	

            // Blindly redraw every frame
            renderToFramebuffer(hostFramebuffer);

            // Did the window resize?
            if(didWindowResize) {
                didWindowResize = false;
                vkCore.doWindowResize();
            }

            // Acquire swap image
            unsigned int indexSwap = pro::acquireNextSwapImage(vkCore, frameCommandData);

            // Record a frame
            recordFrame(
                vkCore, 
                frameCommandData,
                vkCore.swapchain().swaps[indexSwap], 
                vkCore.depthImages()[indexSwap],                
                pipelineData,
                updateFramebuffer,
                hostFramebuffer,
                screenStagingBuffer,
                screenTexture,
                firstTimeFramebuffer,
                screenQuadMesh,                           
                descSetPack
            );      
            
            // Not the first time anymore
            if(firstTimeFramebuffer) {
                firstTimeFramebuffer = false;
            }
                           
            // Submit to queue
            pro::submitForFrame(vkCore, frameCommandData, indexSwap);

            // Present
            if(!pro::presentSwapImage(vkCore, indexSwap)) {
                cout << "Warning: Presentation was not successful." << endl;
            }            
        }
        
        ///////////////////////////////////////////////////////////////////////
        // CLEANUP
        ///////////////////////////////////////////////////////////////////////

        // Wait until device is completely idle
        vkCore.device().waitIdle();             
    }
    
    ///////////////////////////////////////////////////////////////////////
    // GLFW CLEANUP
    ///////////////////////////////////////////////////////////////////////

    glfwDestroyWindow(window);
    glfwTerminate();   

    // End program successfully
    return 0;
}

