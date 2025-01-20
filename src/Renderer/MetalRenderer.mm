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

id<MTLDevice> device = nil;
id<MTLCommandQueue> commandQueue = nil;
MTLRenderPassDescriptor* renderPassDescriptor = nil;
CAMetalLayer* layer = nil;
NSWindow* nswin = nil; 
float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};

void MetalRenderer::init(GLFWwindow* window)
{
    this->window = window;
    device = MTLCreateSystemDefaultDevice();
    commandQueue = [device newCommandQueue];

    ImGui_ImplGlfw_InitForOther(window, true);
    ImGui_ImplMetal_Init(device);

    nswin = glfwGetCocoaWindow(window);
    layer = [CAMetalLayer layer];
    layer.device = device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.contentsScale = nswin.backingScaleFactor;
    nswin.contentView.layer = layer;
    nswin.contentView.wantsLayer = YES;

    renderPassDescriptor = [MTLRenderPassDescriptor new];
}

void MetalRenderer::step(std::function<void()> guiFunction, bool shouldIncreaseFramerate)
{
    @autoreleasepool
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        if (width == 0 || height == 0) 
        {
            NSLog(@"Window width or height set to zero!");
            return; 
        }

        layer.drawableSize = CGSizeMake(width, height);
        id<CAMetalDrawable> drawable = [layer nextDrawable];

        if (!drawable) 
        {
            NSLog(@"Failed to get a valid drawable.");
            return;
        }

        id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

        if (!commandBuffer) {
            NSLog(@"Failed to create command buffer.");
            return;
        }

        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clear_color[0] * clear_color[3], clear_color[1] * clear_color[3], clear_color[2] * clear_color[3], clear_color[3]);
        renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
        renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        [renderEncoder pushDebugGroup:@"ImGui demo"];
        // Start the Dear ImGui frame
        ImGui_ImplMetal_NewFrame(renderPassDescriptor);

        ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);

        guiFunction();

        ImGui::Render();
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
