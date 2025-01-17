#pragma once
#include "AssetLibrary/AssetLibrary.h"
#include "Containers/Map.h"
#include "Core/DeletionQueue.h"
#include "Core/MathTypes.h"
#include "DescriptorBuilder.h"
#include "ImmediateDrawQueue.h"
#include "MaterialSystem.h"
#include "Memory/Base.h"
#include "Mesh.h"
#include "MulticastDelegate.h"
#include "RenderObject.h"
#include "RendererTypes.h"
#include "Shader.h"
#include "TextureSystem.h"
#include "VulkanCommon/VulkanCommon.h"
#include "Window/Window.h"
#include "vk_mem_alloc.h"
using namespace win;

struct Renderer {
    Renderer() = default;

    Window*    window;
    Input*     input;
    bool       validation_layers = false;
    Allocator& allocator         = System_Allocator;

    static constexpr int      num_overlap_frames    = 2;
    static constexpr int      num_indirect_commands = 100;
    bool                      is_initialized        = false;
    VkExtent2D                extent                = {0, 0};
    bool                      do_blit_pass          = true;
    Arena<ArenaMode::Dynamic> frame_arena;
    VMA                       vma;

    // Rendering Objects
    VkInstance                 instance;
    VkSurfaceKHR               surface;
    VkPhysicalDevice           physical_device;
    VkPhysicalDeviceProperties physical_device_properties;
    VkDevice                   device;
    VkSwapchainKHR             swap_chain;
    FrameData                  frames[num_overlap_frames];
    VkDescriptorSetLayout      global_set_layout;
    VkDescriptorSetLayout      object_set_layout;

    MaterialSystem material_system;
    TextureSystem  texture_system;
    ShaderCache    shader_cache;

    using Hook = MulticastDelegate<Renderer*>;
    struct {
        Hook                                          post_init;
        /** Called right before the presentation render pass is ended */
        MulticastDelegate<Renderer*, VkCommandBuffer> post_present_pass;
    } hooks;

    /**
     * @brief structs that relate to rendering to texture.
     * This texture will then be used as a shader resource for
     * the presentation pass
     */
    struct {
        AllocatedImage color_image;
        VkImageView    color_image_view;
        VkFormat       color_image_format = VK_FORMAT_R8G8B8A8_UNORM;

        AllocatedImage depth_image;
        VkImageView    depth_image_view;
        VkFormat       depth_image_format = VK_FORMAT_D32_SFLOAT;

        VkFramebuffer framebuffer;
        VkRenderPass  render_pass;

        DeletionQueue deletion{System_Allocator};
        VkExtent2D    extent;
    } color_pass;

    struct {
        TArray<VkImage>       images;
        TArray<VkImageView>   image_views;
        VkFormat              image_format;
        TArray<VkFramebuffer> framebuffers;
        VkRenderPass          render_pass;
        VkPipelineLayout      pipeline_layout;
        VkPipeline            pipeline;
        VkDescriptorSetLayout texture_set_layout;
        VkDescriptorSet       texture_set = VK_NULL_HANDLE;
        VkSampler             texture_sampler;
    } present_pass;

    // Statistics
    u32 frame_num = 0;

    DeletionQueue main_deletion_queue;
    DeletionQueue swap_chain_deletion_queue;

    // Scene Management
    Slice<RenderObject> render_objects;

    // Immediate
    ImmediateDrawQueue imm;

    /** GPU Global Instance Data */
    struct {
        AllocatedBuffer<> buffer;
        u32               total_buffer_size;
    } global;

    UploadContext upload;

    struct {
        DescriptorAllocator   allocator;
        DescriptorLayoutCache cache;
    } desc;

    struct {
        u32     family;
        VkQueue queue;
    } graphics;

    struct {
        u32     family;
        VkQueue queue;
    } presentation;

    struct {
        glm::vec3 position        = {0, 0, -5};
        glm::quat rotation        = {1, 0, 0, 0};
        glm::vec3 velocity        = {0, 0, 0};
        glm::vec3 input_direction = {0, 0, 0};
        glm::vec3 move_velocity   = {0, 0, 0};
        float     acceleration    = 0.05f;
        float     yaw = 0.f, pitch = 0.f;
        bool      has_focus = false;
        glm::mat4 view;
        glm::mat4 proj;
    } debug_camera;

    void init();
    void deinit();
    void draw();
    void draw_color_pass(VkCommandBuffer cmd, FrameData& frame, u32 frame_idx);
    void draw_present_pass(
        VkCommandBuffer cmd, FrameData& frame, u32 frame_idx);

    void resize_offscreen_buffer(u32 width, u32 height);

    /**
     * Used to update debug camera
     */
    void                             update();
    VkShaderModule                   load_shader(Str path);
    void                             upload_mesh(Mesh& mesh);
    Result<AllocatedImage, VkResult> upload_image_from_file(Str path);
    Result<AllocatedImage, VkResult> upload_image(const Asset& asset);

    using ImmediateSubmitDelegate = Delegate<void, VkCommandBuffer>;

    FORWARD_DELEGATE_LAMBDA_TEMPLATE()
    void immediate_submit_lambda(
        FORWARD_DELEGATE_LAMBDA_SIG(ImmediateSubmitDelegate))
    {
        ImmediateSubmitDelegate delegate =
            FORWARD_DELEGATE_LAMBDA_CREATE(ImmediateSubmitDelegate);
        immediate_submit(std::move(delegate));
    }

    void immediate_submit(ImmediateSubmitDelegate&& submit_delegate);

private:
    void init_present_render_pass();
    void init_color_render_pass();
    void init_pipelines();
    void init_framebuffers();
    void init_descriptors();
    void init_sync_objects();
    void init_default_images();
    void init_commands();
    void init_input();
    void recreate_swapchain();

    void on_resize_presentation();

    FrameData& get_current_frame();

    size_t pad_uniform_buffer_size(size_t original_size);

    // Debug Camera
    void on_debug_camera_forward();
    void on_debug_camera_back();
    void on_debug_camera_right();
    void on_debug_camera_left();
    void on_debug_camera_up();
    void on_debug_camera_down();
    void on_debug_camera_toggle_control();
    void on_debug_camera_mousex(float value);
    void on_debug_camera_mousey(float value);
    void debug_camera_update_rotation();

    bool compact_render_objects(
        const Slice<RenderObject>& objects, TArray<IndirectBatch>& result);
};