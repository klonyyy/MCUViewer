#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_metal.h"
#include <stdio.h>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

// Global Metal objects
static id<MTLDevice> device = nil;
static id<MTLCommandQueue> commandQueue = nil;
static id<MTLRenderPassDescriptor> renderPassDescriptor = nil;
static CAMetalLayer* layer = nil;
static NSWindow* nswin = nil; // Window from GLFW
static float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};

void init(GLFWwindow* window)
{
    @autoreleasepool
    {
        // Create Metal device and command queue
        device = MTLCreateSystemDefaultDevice();
        if (!device) {
            printf("Metal is not supported on this device.\n");
            return;
        }
        commandQueue = [device newCommandQueue];
        if (!commandQueue) {
            printf("Failed to create command queue.\n");
            return;
        }

        // Initialize ImGui Metal binding
        ImGui_ImplMetal_Init(device);

        // Get GLFW window and set up Metal layer
        nswin = glfwGetCocoaWindow(window);
        layer = [CAMetalLayer layer];
        layer.device = device;
        layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        nswin.contentView.layer = layer;
        nswin.contentView.wantsLayer = YES;

        // Create a render pass descriptor
        renderPassDescriptor = [MTLRenderPassDescriptor new];
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clear_color[0] * clear_color[3], clear_color[1] * clear_color[3], clear_color[2] * clear_color[3], clear_color[3]);
        renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    }
}

void stepEntry(GLFWwindow* window)
{
    @autoreleasepool
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        layer.drawableSize = CGSizeMake(width, height);
        id<CAMetalDrawable> drawable = [layer nextDrawable];

        id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

        renderPassDescriptor.colorAttachments[0].texture = drawable.texture;

        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        [renderEncoder pushDebugGroup:@"ImGui demo"];

        // Start the ImGui frame
        ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    }
}

void stepExit()
{
    @autoreleasepool
    {
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandQueue, renderPassDescriptor);
        [renderPassDescriptor.colorAttachments[0].texture endEncoding];
    }
}

void deinit()
{
    @autoreleasepool
    {
        ImGui_ImplMetal_Shutdown();
        device = nil;
        commandQueue = nil;
        renderPassDescriptor = nil;
        layer = nil;
        nswin = nil;
    }
}
