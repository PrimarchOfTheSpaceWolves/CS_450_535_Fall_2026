#pragma once
#include "ProCore.hpp"

namespace pro {

    ///////////////////////////////////////////////////////////////////////////
    // CONSTANTS AND DEFINES 
    ///////////////////////////////////////////////////////////////////////////
    
    #define IMAGE_AVAILABLE_SEM     "imageAvailable"
        
    ///////////////////////////////////////////////////////////////////////////
    // FUNCTIONS 
    ///////////////////////////////////////////////////////////////////////////

    inline CommandData createFrameCommandData(VulkanCore &vkCore) {
        return CommandData( vkCore.device(), 
                            vkCore.graphicsQueue(), 
                            { IMAGE_AVAILABLE_SEM },
                            {},
                            { vk::PipelineStageFlagBits::eAllCommands });        
    };

    inline unsigned int acquireNextSwapImage(   VulkanCore &vkCore, 
                                                CommandData &commandData) {

        // Before a frame-in-flight can start rendering, it needs:
        // - CPU must wait until command buffer has finished all rendering commands (GPU)
        // - Index of swap image to use
        // - Signal (from imageAvail semaphore) that this particular swap image IS actually done presenting

        // CPU waiting until frame-in-flight has completed any rendering commands.
        commandData.waitForFence();

        // Get next swap image.
        // This MAY return before swap image is actually done presenting,
        // hence why we will have our rendering commands wait until imageAvail signals
        unsigned int indexSwap = -1;
        bool successfulAcquire = true;
        do {
            successfulAcquire = true;
            auto frameResult = vkCore.swapchain().chain.acquireNextImage(   UINT64_MAX, 
                                                                            commandData.waitSemaphore(IMAGE_AVAILABLE_SEM),
                                                                            nullptr);   
            
            // Is this out of date?
            if (frameResult.result == vk::Result::eErrorOutOfDateKHR) {
                vkCore.doWindowResize();
                successfulAcquire = false;
            }

            // Get swap image index
            indexSwap = frameResult.value;
        } while(!successfulAcquire);

        // Reset the fence since we're about to submit work
        commandData.resetFence();

        // Return SWAP image index
        return indexSwap;      
    };

    inline void submitForFrame( VulkanCore &vkCore, 
                                CommandData &commandData,
                                unsigned int indexSwap) {

        // With our submission of render commands, we want:
        // - To WAIT until the swap image is actually available
        // - When we're done, to SIGNAL the (PER SWAP IMAGE) render finished semaphore
        commandData.submit(
            {}, 
            {vkCore.swapchain().swaps[indexSwap].renderDone}, 
            {});
    };

    inline bool presentSwapImage(   VulkanCore &vkCore,                                   
                                    unsigned int indexSwap) {

        vk::PresentInfoKHR presentInfo {};
        presentInfo.setWaitSemaphores(*(vkCore.swapchain().swaps[indexSwap].renderDone));
        presentInfo.setSwapchains(*(vkCore.swapchain().chain));
        presentInfo.setImageIndices(indexSwap);
               
        bool successPresent = true;
        try {
            vk::Result presentRes = (*(vkCore.presentQueue().queue)).presentKHR(presentInfo);
        }
        catch (const vk::SystemError& e) {
            if (e.code() == vk::Result::eErrorOutOfDateKHR ||
                e.code() == vk::Result::eSuboptimalKHR) {
                vkCore.doWindowResize();
                successPresent = false;
            } 
            else {
                throw;  // Throws original exception (not copy)
            }
        }

        return successPresent;
    };    
}