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
#include "MetalRenderer.hpp"

static id<MTLDevice> device = nil;
static id<MTLCommandQueue> commandQueue = nil;
static id<MTLCommandBuffer> commandBuffer = nil;
static MTLRenderPassDescriptor* renderPassDescriptor = nil;
static id <MTLRenderCommandEncoder> renderEncoder = nil;
static id<CAMetalDrawable> drawable = nil;
static CAMetalLayer* layer = nil;
static NSWindow* nswin = nil; // Window from GLFW
static float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};

void MetalRenderer::init(GLFWwindow* window)
{
    device = MTLCreateSystemDefaultDevice();
    commandQueue = [device newCommandQueue];

    ImGui_ImplMetal_Init(device);

    nswin = glfwGetCocoaWindow(window);
    CAMetalLayer *layer = [CAMetalLayer layer];
    layer.device = device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    nswin.contentView.layer = layer;
    nswin.contentView.wantsLayer = YES;

    renderPassDescriptor = [MTLRenderPassDescriptor new];
    float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};
}

void MetalRenderer::stepEnter()
{
    @autoreleasepool
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        layer.drawableSize = CGSizeMake(width, height);
        drawable = [layer nextDrawable];

        commandBuffer = [commandQueue commandBuffer];
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clear_color[0] * clear_color[3], clear_color[1] * clear_color[3], clear_color[2] * clear_color[3], clear_color[3]);
        renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
        renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        [renderEncoder pushDebugGroup:@"ImGui demo"];

        // Start the Dear ImGui frame
        ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    }
}

void MetalRenderer::stepExit()
{
    @autoreleasepool
    {
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderEncoder);

        [renderEncoder popDebugGroup];
        [renderEncoder endEncoding];

        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
    }
}

void MetalRenderer::deinit()
{
    ImGui_ImplMetal_Shutdown();
}
