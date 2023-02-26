#pragma once
#include "Memory/Base.h"
#include "VulkanCommon.h"
#include "Window.h"

struct Engine {
    Window*     window;
    bool        validation_layers = false;
    IAllocator& allocator;

    bool       is_initialized = false;
    VkExtent2D extent         = {0, 0};

    // Rendering Objects

    VkInstance            instance;
    VkSurfaceKHR          surface;
    VkPhysicalDevice      physical_device;
    VkDevice              device;
    VkSwapchainKHR        swap_chain;
    VkFormat              swap_chain_image_format;
    TArray<VkImage>       swap_chain_images;
    TArray<VkImageView>   swap_chain_image_views;
    VkCommandPool         cmd_pool;
    VkCommandBuffer       main_cmd_buffer;
    VkRenderPass          render_pass;
    TArray<VkFramebuffer> framebuffers;

    // Sync Objects

    VkSemaphore sem_render, sem_present;
    VkFence     fnc_render;

    // Statistics
    u32 frame_num = 0;

    struct {
        u32     family;
        VkQueue queue;
    } graphics;

    struct {
        u32     family;
        VkQueue queue;
    } presentation;

    void init();
    void deinit();
    void draw();

private:
    void init_default_renderpass();
    void init_framebuffers();
    void init_sync_objects();
};