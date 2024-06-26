#pragma once
#ifdef WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#endif
#include "vulkan_utility.hpp"
#include <vulkan_helper.hpp>

class simple_draw_command {
public:
    simple_draw_command(
        vk::CommandBuffer cmd,
        vk::RenderPass render_pass,
        vk::PipelineLayout pipeline_layout,
        vk::Pipeline pipeline,
        vk::DescriptorSet descriptor_set,
        vk::Framebuffer framebuffer,
        vk::Extent2D swapchain_extent,
        vk::DispatchLoaderDynamic dldid)
        : m_cmd{ cmd } {
        vk::CommandBufferBeginInfo begin_info{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };
        cmd.begin(begin_info);
        std::array<vk::ClearValue, 2> clear_values;
        clear_values[0].color = vk::ClearColorValue{ 1.0f, 1.0f,1.0f,1.0f };
        clear_values[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };
        vk::RenderPassBeginInfo render_pass_begin_info{
            render_pass, framebuffer,
            vk::Rect2D{vk::Offset2D{0,0},
            swapchain_extent}, clear_values };
        cmd.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
            pipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            pipeline_layout, 0, descriptor_set, nullptr);
        //cmd.bindVertexBuffers(0, *vertex_buffer, { 0 });
        cmd.setViewport(0, vk::Viewport(0, 0, swapchain_extent.width, swapchain_extent.height, 0, 1));
        cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchain_extent));
        //cmd.draw(3, 1, 0, 0);
        cmd.drawMeshTasksEXT(1, 1, 1, dldid);
        cmd.endRenderPass();
        cmd.end();
    }
    auto get_command_buffer() {
        return m_cmd;
    }
private:
    vk::CommandBuffer m_cmd;
};

namespace concept_helper {
    namespace shared {
        template<typename Instance>
        concept instance = vulkan_helper::concept_helper::instance<Instance> && requires (Instance instance) {
            instance.get_vulkan_shared_instance();
        };
        template<typename PhysicalDevice>
        concept physical_device = vulkan_helper::concept_helper::physical_device<PhysicalDevice> &&
            requires (PhysicalDevice physical_device) {
                { physical_device.get_vulkan_shared_physical_device() } -> std::convertible_to<vk::SharedPhysicalDevice>;
        };
        template<typename Device>
        concept device = vulkan_helper::concept_helper::physical_device<Device> &&
            requires (Device device) {
            device.get_vulkan_shared_device();
        };
    }
}

template<concept_helper::shared::instance Instance>
class add_shared_physical_device : public Instance {
public:
    add_shared_physical_device() {
        auto shared_instance = Instance::get_vulkan_shared_instance();
        auto physical_devices = shared_instance->enumeratePhysicalDevices();
        m_physical_device = vk::SharedPhysicalDevice{ physical_devices[0], shared_instance};
    }
    auto get_vulkan_physical_device() {
        return *m_physical_device;
    }
    auto get_vulkan_shared_physical_device() {
        return m_physical_device;
    }
private:
    vk::SharedPhysicalDevice m_physical_device;
};

    template<class T>
    class add_mesh_extension : public T {
    public:
        auto get_extensions() {
            auto ext = T::get_extensions();
            ext.push_back(vk::EXTMeshShaderExtensionName);
            return ext;
        }
    };
template<class T>
class add_device_create_info_structure_chain : public T {
public:
    auto get_device_create_info_structure_chain() {
        return vk::StructureChain{ vk::DeviceCreateInfo{} };
    }
};

class mesh_device_create_info {
public:
    mesh_device_create_info(uint32_t queue_family_index, std::vector<std::string> device_extensions)
    : m_queue_priority{}, m_queue_create_info{}, m_structure_chain{}, m_device_extensions{device_extensions} {
        m_queue_priority = 0.0f;
        m_queue_create_info.setQueuePriorities(m_queue_priority).setQueueFamilyIndex(queue_family_index);

        m_raw_ptr_device_extensions.resize(m_device_extensions.size());
        std::ranges::transform(
            m_device_extensions,
            m_raw_ptr_device_extensions.begin(),
            [](auto& ext) { return ext.c_str();  });
        m_structure_chain.get<vk::DeviceCreateInfo>().setQueueCreateInfos(m_queue_create_info).setPEnabledExtensionNames(m_raw_ptr_device_extensions);
        m_structure_chain.get<vk::PhysicalDeviceMeshShaderFeaturesEXT>().setMeshShader(true).setTaskShader(true);
        m_structure_chain.get<vk::PhysicalDeviceSynchronization2Features>().setSynchronization2(true);
        m_structure_chain.get<vk::PhysicalDeviceMaintenance4Features>().setMaintenance4(true);
    }
    vk::DeviceCreateInfo& get_device_create_info() {
        return m_structure_chain.get<vk::DeviceCreateInfo>();
    }
private:
    float m_queue_priority;
    vk::DeviceQueueCreateInfo m_queue_create_info;
    vk::StructureChain<
        vk::DeviceCreateInfo,
        vk::PhysicalDeviceMeshShaderFeaturesEXT,
        vk::PhysicalDeviceMaintenance4Features,
        vk::PhysicalDeviceSynchronization2Features> m_structure_chain;
    std::vector<std::string> m_device_extensions;
    std::vector<const char*> m_raw_ptr_device_extensions;
};
class vertex_device_create_info {
public:
    vertex_device_create_info(uint32_t queue_family_index, std::vector<std::string> device_extensions)
        : m_queue_priority{}, m_queue_create_info{}, m_structure_chain{}, m_device_extensions{ device_extensions } {
        m_queue_priority = 0.0f;
        m_queue_create_info.setQueuePriorities(m_queue_priority).setQueueFamilyIndex(queue_family_index);

        m_raw_ptr_device_extensions.resize(m_device_extensions.size());
        std::ranges::transform(
            m_device_extensions,
            m_raw_ptr_device_extensions.begin(),
            [](auto& ext) { return ext.c_str();  });
        m_structure_chain.get<vk::DeviceCreateInfo>().setQueueCreateInfos(m_queue_create_info).setPEnabledExtensionNames(m_raw_ptr_device_extensions);
        m_structure_chain.get<vk::PhysicalDeviceSynchronization2Features>().setSynchronization2(true);
        m_structure_chain.get<vk::PhysicalDeviceMaintenance4Features>().setMaintenance4(true);
    }
    vk::DeviceCreateInfo& get_device_create_info() {
        return m_structure_chain.get<vk::DeviceCreateInfo>();
    }
private:
    float m_queue_priority;
    vk::DeviceQueueCreateInfo m_queue_create_info;
    vk::StructureChain<
        vk::DeviceCreateInfo,
        vk::PhysicalDeviceMaintenance4Features,
        vk::PhysicalDeviceSynchronization2Features> m_structure_chain;
    std::vector<std::string> m_device_extensions;
    std::vector<const char*> m_raw_ptr_device_extensions;
};

template<class T>
class add_mesh_device_create_info_aggregate : public T{
public:
    using parent = T;
    auto get_device_create_info_aggregate() {
        uint32_t queue_family_index = parent::get_queue_family_index();
        std::vector<std::string> deviceExtensions = parent::get_extensions();
        return mesh_device_create_info{queue_family_index, deviceExtensions};
    }
};
template<class T>
class add_vertex_device_create_info_aggregate : public T {
public:
    using parent = T;
    auto get_device_create_info_aggregate() {
        uint32_t queue_family_index = parent::get_queue_family_index();
        std::vector<std::string> deviceExtensions = parent::get_extensions();
        return vertex_device_create_info{ queue_family_index, deviceExtensions };
    }
};
template<class T>
class set_queue_family_index : public T{
public:
    auto get_queue_family_index() {
        return 0;
    }
};

template<concept_helper::shared::physical_device PhysicalDevice>
class add_shared_device : public PhysicalDevice {
public:
    using parent = PhysicalDevice;
    add_shared_device() {
        auto physical_device = PhysicalDevice::get_vulkan_physical_device();

        auto device_create_info_aggregate = parent::get_device_create_info_aggregate();

        m_device = vk::SharedDevice{ physical_device.createDevice(device_create_info_aggregate.get_device_create_info()) };
    }
    auto get_vulkan_device() {
        return *m_device;
    }
    auto get_vulkan_shared_device() {
        return m_device;
    }
private:
    vk::SharedDevice m_device;
};

template<concept_helper::shared::device Device>
class vulkan_render_prepare : public Device {
public:
    using parent = Device;

    auto get_queue(auto device, auto queue_family_index) {
        return vulkan::shared::get_queue(device, queue_family_index, 0);
    }
    auto create_command_pool(auto device, auto queue_family_index) {
        return vulkan::shared::create_command_pool(device, queue_family_index);
    }
    auto get_surface(auto instance, auto&& get_surface_from_extern) {
        return vulkan::shared::get_surface(instance, get_surface_from_extern);
    }
    auto select_color_format(auto physical_device, auto surface) {
        std::vector<vk::SurfaceFormatKHR> formats = physical_device.getSurfaceFormatsKHR(*surface);
        return (formats[0].format == vk::Format::eUndefined) ? vk::Format::eR8G8B8A8Unorm : formats[0].format;
    }
    auto select_depth_format() {
        return vk::Format::eD16Unorm;
    }
    auto get_surface_capabilities(auto physical_device, auto surface) {
        return physical_device.getSurfaceCapabilitiesKHR(*surface);
    }
    auto get_surface_extent(auto surface_capabilities) {
        return surface_capabilities.currentExtent;
    }
    auto create_swapchain(
        auto physical_device,
        auto device,
        auto surface,
        auto surface_capabilities,
        auto color_format) {
        return vk::SharedSwapchainKHR(
            vulkan::create_swapchain(
                physical_device,
                *device,
                *surface,
                surface_capabilities,
                color_format),
            device,
            surface);
    }
    auto create_render_pass(auto device, auto color_format, auto depth_format) {
        return vk::SharedRenderPass{ vulkan::create_render_pass(*device, color_format, depth_format), device };
    }
    auto create_descriptor_set_bindings() {
        return std::array{
            vk::DescriptorSetLayoutBinding{}
            .setBinding(0)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            .setDescriptorCount(1),
            vk::DescriptorSetLayoutBinding{}
            .setBinding(1)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setStageFlags(vk::ShaderStageFlagBits::eMeshEXT)
            .setDescriptorCount(1),
        };
    }
    auto create_descriptor_set_layout(auto device, auto descriptor_set_bindings) {
        return device->createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo{}
            .setBindings(descriptor_set_bindings));
    }
    auto create_pipeline_layout(auto device, auto& descriptor_set_layout) {
        return vk::SharedPipelineLayout{
            vulkan::create_pipeline_layout(*device, *descriptor_set_layout),
            device };
    }
    auto create_descriptor_pool(auto device, auto descriptor_pool_size) {
        return device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{}.setPoolSizes(descriptor_pool_size).setMaxSets(1));
    }
    auto get_descriptor_pool_size() {
        return std::array{
            vk::DescriptorPoolSize{}.setType(vk::DescriptorType::eCombinedImageSampler).setDescriptorCount(1),
            vk::DescriptorPoolSize{}.setType(vk::DescriptorType::eUniformBuffer).setDescriptorCount(1),
        };
    }
    auto allocate_descriptor_set(auto device, auto& descriptor_set_layout) {
        return std::move(
            device->allocateDescriptorSets(
                vk::DescriptorSetAllocateInfo{}
                .setDescriptorPool(*descriptor_pool)
                .setSetLayouts(*descriptor_set_layout))
            .front()
            );
    }

    auto create_font_texture(auto characters) {
        auto physical_device = parent::get_vulkan_physical_device();
        auto device = parent::get_vulkan_device();
        auto shared_device = parent::get_vulkan_shared_device();
        font_loader font_loader{};
        uint32_t font_width = 32, font_height = 32;
        uint32_t line_height = font_height * 2;
        font_loader.set_char_size(font_width, font_height);
        auto [vk_texture, vk_texture_memory, vk_texture_view] =
            vulkan::create_texture(physical_device, device,
                vk::Format::eR8Unorm,
                font_width * std::size(characters), line_height,
                [&font_loader, font_width, font_height, line_height, &characters](char* ptr, int pitch) {
                    std::ranges::for_each(from_0_count_n(characters.size()),
                    [&font_loader, font_width, font_height, line_height, &characters, ptr, pitch](auto i) {
                            font_loader.render_char(characters[i]);
                            auto glyph = font_loader.get_glyph();
                            uint32_t start_row = font_height - glyph->bitmap_top - 1;
                            assert(start_row + glyph->bitmap.rows < line_height);
                            for (int row = 0; row < glyph->bitmap.rows; row++) {
                                for (int x = 0; x < glyph->bitmap.width; x++) {
                                    ptr[(start_row + row) * pitch + glyph->bitmap_left + i * font_width + x] =
                                        glyph->bitmap.buffer[row * glyph->bitmap.pitch + x];
                                }
                            }
                        });
                });
        auto texture = vk::SharedImage{
            vk_texture, shared_device };
        auto texture_memory = vk::SharedDeviceMemory{ vk_texture_memory, shared_device };
        auto texture_view = vk::SharedImageView{ vk_texture_view, shared_device };
        return std::tuple{ texture, texture_memory, texture_view };
    }
    void update_descriptor_set(auto descriptor_set, auto texture_view, auto& sampler, auto& char_indices_buffer) {
        auto texture_image_info =
            vk::DescriptorImageInfo{}
            .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImageView(*texture_view)
            .setSampler(*sampler);
        auto char_texture_indices_info =
            vk::DescriptorBufferInfo{}
            .setBuffer(*char_indices_buffer)
            .setOffset(0)
            .setRange(vk::WholeSize);
        auto descriptor_set_write = std::array{
            vk::WriteDescriptorSet{}
            .setDstBinding(0)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setImageInfo(texture_image_info)
            .setDstSet(descriptor_set),
            vk::WriteDescriptorSet{}
            .setDstBinding(1)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setBufferInfo(char_texture_indices_info)
            .setDstSet(descriptor_set),
        };
        parent::get_vulkan_device().updateDescriptorSets(descriptor_set_write, nullptr);
    }
    auto generate_char_set(auto& terminal_buffer) {
        std::set<char> char_set{};
        std::for_each(
            terminal_buffer.begin(),
            terminal_buffer.end(),
            [&char_set](auto c) {
                char_set.emplace(c);
            });
        return char_set;
    }
    auto generate_characters(auto& char_set) {
        std::vector<char> characters{};
        std::ranges::copy(char_set, std::back_inserter(characters));
        return characters;
    }
    auto generate_char_texture_indices(auto& characters) {
        std::map<char, int> char_texture_indices;
        std::ranges::for_each(from_0_count_n(characters.size()), [&characters, &char_texture_indices](auto i) {
            char_texture_indices.emplace(characters[i], static_cast<int>(i));
            });
        return char_texture_indices;
    }
    auto generate_char_indices_buf(auto& terminal_buffer, auto& char_texture_indices) {
        multidimention_vector<uint32_t> char_indices_buf{terminal_buffer.get_width(), terminal_buffer.get_height()};
        std::transform(
            terminal_buffer.begin(),
            terminal_buffer.end(),
            char_indices_buf.begin(),
            [&char_texture_indices](auto c) {
                return char_texture_indices[c];
            });
        return char_indices_buf;
    }
    void create_and_update_terminal_buffer_relate_data(
        auto descriptor_set, auto& sampler, auto& terminal_buffer,
        auto& imageViews) {
        auto physical_device = parent::get_vulkan_physical_device();
        auto device = parent::get_vulkan_device();
        auto shared_device = parent::get_vulkan_shared_device();
        //generated by attribute_dependence_parser from vulkan_render_prepare_create_and_update_terminal_buffer_relate_data.depend
        auto char_set = generate_char_set(terminal_buffer);


        character_count = char_set.size();


        auto characters = generate_characters(char_set);


        std::tie(texture, texture_memory, texture_view) = create_font_texture(characters);


        auto char_texture_indices = generate_char_texture_indices(characters);


        char_indices = generate_char_indices_buf(terminal_buffer, char_texture_indices);


        char_indices_buffer = vk::SharedBuffer(vulkan::create_buffer(device, char_indices.size() * sizeof(*std::begin(char_indices)),
            vk::BufferUsageFlagBits::eUniformBuffer), shared_device);


        char_indices_buffer_memory = vk::SharedDeviceMemory(
            std::get<0>(
                vulkan::allocate_device_memory(physical_device, device, *char_indices_buffer,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)),
            shared_device);


        vulkan::copy_to_buffer(device, *char_indices_buffer, *char_indices_buffer_memory, char_indices);


        device.bindBufferMemory(*char_indices_buffer, *char_indices_buffer_memory, 0);


        update_descriptor_set(descriptor_set, texture_view, sampler, char_indices_buffer);
    }
    void create_per_swapchain_image_resources(auto& swapchainImages, auto color_format, auto depth_format) {
        auto device = parent::get_vulkan_device();
        auto shared_device = parent::get_vulkan_shared_device();
        imageViews.reserve(swapchainImages.size());
        vk::ImageViewCreateInfo imageViewCreateInfo({}, {},
            vk::ImageViewType::e2D, color_format, {}, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
        for (auto image : swapchainImages) {
            imageViewCreateInfo.image = image;
            auto image_view = device.createImageView(imageViewCreateInfo);
            imageViews.emplace_back(vk::SharedImageView{ image_view, shared_device});
            render_complete_semaphores.push_back(device.createSemaphoreUnique(vk::SemaphoreCreateInfo{}));

            auto [depth_buffer, depth_buffer_memory, depth_buffer_view] =
                vulkan::create_depth_buffer(parent::get_vulkan_physical_device(), device, depth_format, swapchain_extent);
            depth_buffers.emplace_back(vk::SharedImage{ depth_buffer, shared_device });
            depth_buffer_memories.emplace_back(vk::SharedDeviceMemory{ depth_buffer_memory, shared_device });
            depth_buffer_views.emplace_back(vk::SharedImageView{ depth_buffer_view, shared_device });
            framebuffers.emplace_back(
                vk::SharedFramebuffer{
                vulkan::create_framebuffer(device, *render_pass,
                        std::vector{image_view, depth_buffer_view}, swapchain_extent), shared_device
                });
        }
    }
    void init(auto& terminal_buffer) {
        auto physical_device = parent::get_vulkan_physical_device();
        auto device = parent::get_vulkan_device();
        auto shared_device = parent::get_vulkan_shared_device();
        auto queue_family_index = parent::get_queue_family_index();
        vk::Format depth_format = select_depth_format();


        auto descriptor_pool_size = get_descriptor_pool_size();


        auto descriptor_set_bindings = create_descriptor_set_bindings();


        vk::SharedSurfaceKHR surface = parent::get_vulkan_shared_surface();


        auto surface_capabilities = get_surface_capabilities(physical_device, surface);


        swapchain_extent = get_surface_extent(surface_capabilities);


        vk::Format color_format = select_color_format(parent::get_vulkan_physical_device(), surface);

        p_terminal_buffer = &terminal_buffer;

        queue = get_queue(shared_device, queue_family_index);


        render_pass = create_render_pass(shared_device, color_format, depth_format);


        sampler = device.createSamplerUnique(vk::SamplerCreateInfo());


        swapchain = create_swapchain(physical_device, shared_device, surface, surface_capabilities, color_format);


        auto swapchainImages = device.getSwapchainImagesKHR(*swapchain);


        command_pool = create_command_pool(shared_device, queue_family_index);


        descriptor_pool = create_descriptor_pool(shared_device, descriptor_pool_size);


        descriptor_set_layout = create_descriptor_set_layout(shared_device, descriptor_set_bindings);


        create_per_swapchain_image_resources(swapchainImages, color_format, depth_format);


        pipeline_layout = create_pipeline_layout(shared_device, descriptor_set_layout);


        descriptor_set = allocate_descriptor_set(shared_device, descriptor_set_layout);


        create_and_update_terminal_buffer_relate_data(
            descriptor_set, sampler, terminal_buffer, imageViews);
    }
    void notify_update() {
        create_and_update_terminal_buffer_relate_data(descriptor_set, sampler, *p_terminal_buffer,
            imageViews);
    }

protected:
    multidimention_vector<char>* p_terminal_buffer;
    vk::SharedCommandPool command_pool;
    vk::SharedSwapchainKHR swapchain;
    vk::UniqueDescriptorPool descriptor_pool;
    vk::UniqueDescriptorSetLayout descriptor_set_layout;
    vk::DescriptorSet descriptor_set;
    vk::SharedRenderPass render_pass;
    vk::Extent2D swapchain_extent;

    vk::SharedImage texture;
    vk::SharedImageView texture_view;
    vk::SharedDeviceMemory texture_memory;
    multidimention_vector<uint32_t> char_indices;
    vk::SharedBuffer char_indices_buffer;
    vk::SharedDeviceMemory char_indices_buffer_memory;
    vk::UniqueSampler sampler;
    std::vector<vk::SharedImageView> imageViews;
    std::vector<vk::UniqueSemaphore> render_complete_semaphores;
    std::vector<vk::SharedImage> depth_buffers;
    std::vector<vk::SharedImageView> depth_buffer_views;
    std::vector<vk::SharedDeviceMemory> depth_buffer_memories;
    std::vector<vk::SharedFramebuffer> framebuffers;
    vk::SharedQueue queue;

    vk::SharedPipelineLayout pipeline_layout;
    uint32_t character_count;
};

template<concept_helper::shared::device Device>
class mesh_renderer : public vulkan_render_prepare<Device> {
public:
    using parent = vulkan_render_prepare<Device>;
    auto create_pipeline(auto render_pass, auto pipeline_layout, uint32_t character_count) {
        auto device = parent::get_vulkan_device();
        auto shared_device = parent::get_vulkan_shared_device();
        vulkan::task_stage_info task_stage_info{
            task_shader_path, "main",
        };
        class char_count_specialization {
        public:
            char_count_specialization(uint32_t character_count)
                :
                m_count{ character_count },
                map_entry{ vk::SpecializationMapEntry{}.setConstantID(555).setOffset(0).setSize(sizeof(m_count)) },
                specialization_info{ vk::SpecializationInfo{}
                .setMapEntries(map_entry).setDataSize(sizeof(m_count)).setPData(&this->m_count) }
            {
            }
        public:
            uint32_t m_count;
            vk::SpecializationMapEntry map_entry;
            vk::SpecializationInfo specialization_info;
        };
        char_count_specialization specialization{
            character_count
        };

        vulkan::mesh_stage_info mesh_stage_info{
            mesh_shader_path, "main", specialization.specialization_info
        };
        vulkan::geometry_stage_info geometry_stage_info{
            geometry_shader_path, "main",
        };
        return vk::SharedPipeline{
            vulkan::create_pipeline(device,
                    task_stage_info,
                    mesh_stage_info,
                    fragment_shader_path, *render_pass, *pipeline_layout).value, shared_device };
    }
    void create_and_update_terminal_buffer_relate_data() {
        parent::create_and_update_terminal_buffer_relate_data(
            parent::descriptor_set, parent::sampler, *parent::p_terminal_buffer, parent::imageViews
        );
        pipeline = create_pipeline(parent::render_pass, parent::pipeline_layout, parent::character_count);
        vk::DispatchLoaderDynamic dldid(parent::get_vulkan_instance(), vkGetInstanceProcAddr, parent::get_vulkan_device());
        for (integer_less_equal<decltype(parent::imageViews.size())> i{ 0, parent::imageViews.size() }; i < parent::imageViews.size(); i++) {
            simple_draw_command draw_command{
                command_buffers[i],
                *parent::render_pass,
                *parent::pipeline_layout,
                *pipeline,
                parent::descriptor_set,
                *parent::framebuffers[i],
                parent::swapchain_extent, dldid };
        }
    }
    void init(auto& terminal_buffer) {
        parent::init(terminal_buffer);
        vk::CommandBufferAllocateInfo commandBufferAllocateInfo(
            *parent::command_pool, vk::CommandBufferLevel::ePrimary, parent::imageViews.size());
        command_buffers = parent::get_vulkan_device().allocateCommandBuffers(commandBufferAllocateInfo);
        create_and_update_terminal_buffer_relate_data();
    }
    void notify_update() {
        create_and_update_terminal_buffer_relate_data();
    }
protected:
    vk::SharedPipeline pipeline;
    std::vector<vk::CommandBuffer> command_buffers;
};

template<vulkan_helper::concept_helper::instance Instance>
class vertex_renderer : public vulkan_render_prepare<Instance> {
public:
    using parent = vulkan_render_prepare<Instance>;
    struct vertex {
        float x, y, z, w;
    };
    auto create_pipeline(auto device, auto render_pass, auto pipeline_layout, uint32_t character_count) {
        class char_count_specialization {
        public:
            char_count_specialization(uint32_t character_count)
                :
                m_count{ character_count },
                map_entry{ vk::SpecializationMapEntry{}.setConstantID(555).setOffset(0).setSize(sizeof(m_count)) },
                specialization_info{ vk::SpecializationInfo{}
                .setMapEntries(map_entry).setDataSize(sizeof(m_count)).setPData(&this->m_count) }
            {
            }
        public:
            uint32_t m_count;
            vk::SpecializationMapEntry map_entry;
            vk::SpecializationInfo specialization_info;
        };
        char_count_specialization specialization{
            character_count
        };
        vulkan::vertex_stage_info vertex_stage_info{
            vertex_shader_path, "main",
            vk::VertexInputBindingDescription{}.setBinding(0).setInputRate(vk::VertexInputRate::eVertex).setStride(sizeof(vertex)),
            std::vector{
                vk::VertexInputAttributeDescription{}.setBinding(0).setLocation(0).setOffset(0).setFormat(vk::Format::eR32G32B32A32Sfloat)
            },
            specialization.specialization_info
        };
        return vk::SharedPipeline{
            vulkan::create_pipeline(*device,
                    vertex_stage_info,
                    fragment_shader_path, *render_pass, *pipeline_layout).value, device };
    }
    void create_vertex_buffer(auto&& vertices) {
        auto physical_device = parent::get_vulkan_physical_device();
        auto device = parent::get_vulkan_device();
        auto shared_device = parent::get_vulkan_shared_device();
        auto [buffer, memory, memory_size] = vulkan::create_vertex_buffer(physical_device, device, vertices);
        vertex_buffer = vk::SharedBuffer{ buffer, shared_device};
        vertex_buffer_memory = vk::SharedDeviceMemory{ memory, shared_device};
    }
    void create_and_update_terminal_buffer_relate_data() {
        auto device = parent::get_vulkan_shared_device();
        parent::create_and_update_terminal_buffer_relate_data(
            parent::descriptor_set, parent::sampler, *parent::p_terminal_buffer, parent::imageViews
        );
        std::vector<vertex> vertices{};
        auto& terminal_buffer = *parent::p_terminal_buffer;
        for (int y = 0; y < terminal_buffer.get_dim1_size(); y++) {
            for (int x = 0; x < terminal_buffer.get_dim0_size(); x++) {
                float width = 2.0f / terminal_buffer.get_dim0_size();
                float height = 2.0f / terminal_buffer.get_dim1_size();
                float s_x = -1 + x * width;
                float s_y = -1 + y * height;
                auto index = parent::char_indices[std::pair{ x, y }];
                const float tex_width = 1.0 / parent::character_count;
                const float tex_advance = tex_width;
                const float tex_offset = tex_width * index;
                vertices.push_back(vertex{ s_x, s_y, tex_offset, 0 });
                vertices.push_back(vertex{ s_x + width, s_y, tex_offset + tex_advance, 0 });
                vertices.push_back(vertex{ s_x, s_y + height, tex_offset, 1 });
                vertices.push_back(vertex{ s_x, s_y + height, tex_offset, 1 });
                vertices.push_back(vertex{ s_x + width, s_y, tex_offset + tex_advance, 0 });
                vertices.push_back(vertex{ s_x + width,s_y + height, tex_offset + tex_advance, 1 });
            }
        }
        create_vertex_buffer(vertices);
        pipeline = create_pipeline(device, parent::render_pass, parent::pipeline_layout, parent::character_count);
        vk::DispatchLoaderDynamic dldid(parent::get_vulkan_instance(), vkGetInstanceProcAddr, *device);
        for (integer_less_equal<decltype(parent::imageViews.size())> i{ 0, parent::imageViews.size() }; i < parent::imageViews.size(); i++) {
            record_draw_command(
                command_buffers[i],
                *parent::render_pass,
                *parent::pipeline_layout,
                *pipeline,
                parent::descriptor_set,
                *parent::framebuffers[i],
                parent::swapchain_extent, dldid);
        }
    }
    void init(auto& terminal_buffer) {
        auto device = parent::get_vulkan_device();
        parent::init(terminal_buffer);
        vk::CommandBufferAllocateInfo commandBufferAllocateInfo(
            *parent::command_pool, vk::CommandBufferLevel::ePrimary, parent::imageViews.size());
        command_buffers = device.allocateCommandBuffers(commandBufferAllocateInfo);
        create_and_update_terminal_buffer_relate_data();
    }
    void notify_update() {
        create_and_update_terminal_buffer_relate_data();
    }

    void record_draw_command(
            vk::CommandBuffer cmd,
            vk::RenderPass render_pass,
            vk::PipelineLayout pipeline_layout,
            vk::Pipeline pipeline,
            vk::DescriptorSet descriptor_set,
            vk::Framebuffer framebuffer,
            vk::Extent2D swapchain_extent,
            vk::DispatchLoaderDynamic dldid)
    {
            vk::CommandBufferBeginInfo begin_info{ vk::CommandBufferUsageFlagBits::eSimultaneousUse };
            cmd.begin(begin_info);
            std::array<vk::ClearValue, 2> clear_values;
            clear_values[0].color = vk::ClearColorValue{ 1.0f, 1.0f,1.0f,1.0f };
            clear_values[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };
            vk::RenderPassBeginInfo render_pass_begin_info{
                render_pass, framebuffer,
                vk::Rect2D{vk::Offset2D{0,0},
                swapchain_extent}, clear_values };
            cmd.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                pipeline);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                pipeline_layout, 0, descriptor_set, nullptr);
            cmd.bindVertexBuffers(0, *vertex_buffer, { 0 });
            cmd.bindVertexBuffers(1, *parent::char_indices_buffer, { 0 });
            cmd.setViewport(0, vk::Viewport(0, 0, swapchain_extent.width, swapchain_extent.height, 0, 1));
            cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchain_extent));
            cmd.draw(parent::p_terminal_buffer->size()*6, 1, 0, 0);
            cmd.endRenderPass();
            cmd.end();
    }
protected:
    vk::SharedPipeline pipeline;
    std::vector<vk::CommandBuffer> command_buffers;

    vk::SharedBuffer vertex_buffer;
    vk::SharedDeviceMemory vertex_buffer_memory;
    vk::SharedBufferView vertex_buffer_view;
};

template<class Renderer>
class renderer_presenter : public Renderer {
public:
    using parent = Renderer;
    void set_texture_image_layout() {
        auto device = parent::get_vulkan_device();
        auto texture_prepare_semaphore = present_manager->get_next();
        {
            vk::UniqueCommandBuffer init_command_buffer{
                std::move(device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{}.setCommandBufferCount(1).setCommandPool(*Renderer::command_pool)).front()) };
            init_command_buffer->begin(vk::CommandBufferBeginInfo{});
            vulkan::set_image_layout(*init_command_buffer, *Renderer::texture, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::ePreinitialized,
                vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits(), vk::PipelineStageFlagBits::eTopOfPipe,
                vk::PipelineStageFlagBits::eFragmentShader);
            auto& cmd = init_command_buffer;
            cmd->end();
            auto signal_semaphore = texture_prepare_semaphore.semaphore;
            auto command_submit_info = vk::CommandBufferSubmitInfo{}.setCommandBuffer(*cmd);
            auto signal_semaphore_info = vk::SemaphoreSubmitInfo{}.setSemaphore(signal_semaphore).setStageMask(vk::PipelineStageFlagBits2::eTransfer);
            Renderer::queue->submit2(vk::SubmitInfo2{}.setCommandBufferInfos(command_submit_info).setSignalSemaphoreInfos(signal_semaphore_info));
            Renderer::queue->submit2(vk::SubmitInfo2{}.setWaitSemaphoreInfos(signal_semaphore_info), texture_prepare_semaphore.fence);
            auto res = device.waitForFences(std::array<vk::Fence, 1>{texture_prepare_semaphore.fence}, true, UINT64_MAX);
            assert(res == vk::Result::eSuccess);
        }
    }
    void init(auto& terminal_buffer) {
        Renderer::init(terminal_buffer);
        present_manager = std::make_shared<vulkan::present_manager>(parent::get_vulkan_shared_device(), 10);
        set_texture_image_layout();
    }
    run_result run()
    {
        auto reused_acquire_image_semaphore = present_manager->get_next();
        auto image_index = parent::get_vulkan_device().acquireNextImageKHR(
            *Renderer::swapchain, UINT64_MAX,
            reused_acquire_image_semaphore.semaphore)
            .value;

        auto& render_complete_semaphore = Renderer::render_complete_semaphores[image_index];
        auto& command_buffer = Renderer::command_buffers[image_index];

        {
            auto wait_semaphore_infos = std::array{
                vk::SemaphoreSubmitInfo{}
                    .setSemaphore(reused_acquire_image_semaphore.semaphore)
                    .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput),
            };
            auto submit_cmd_info = vk::CommandBufferSubmitInfo{}.setCommandBuffer(command_buffer);
            auto signal_semaphore_info = vk::SemaphoreSubmitInfo{}.setSemaphore(*render_complete_semaphore).setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
            Renderer::queue->submit2(
                vk::SubmitInfo2{}
                .setWaitSemaphoreInfos(wait_semaphore_infos)
                .setCommandBufferInfos(submit_cmd_info)
                .setSignalSemaphoreInfos(signal_semaphore_info),
                reused_acquire_image_semaphore.fence);
        }

        {
            std::array<vk::Semaphore, 1> wait_semaphores{ *render_complete_semaphore };
            std::array<vk::SwapchainKHR, 1> swapchains{ *Renderer::swapchain };
            std::array<uint32_t, 1> indices{ image_index };
            vk::PresentInfoKHR present_info{ wait_semaphores, swapchains, indices };
            auto res = Renderer::queue->presentKHR(present_info);
            assert(res == vk::Result::eSuccess || res == vk::Result::eSuboptimalKHR);
        }
        return run_result::eContinue;
    }
    void notify_update() {
        present_manager->wait_all();
        Renderer::notify_update();
        set_texture_image_layout();
    }
private:
    std::shared_ptr<vulkan::present_manager> present_manager;
};
