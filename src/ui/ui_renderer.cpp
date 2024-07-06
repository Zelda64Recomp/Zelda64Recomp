#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <fstream>
#include <filesystem>
#ifdef _WIN32
#include <SDL_video.h>
#else
#include <SDL2/SDL_video.h>
#endif


#include "recomp_ui.h"
#include "recomp_input.h"
#include "librecomp/game.hpp"
#include "zelda_config.h"
#include "ui_rml_hacks.hpp"

#include "concurrentqueue.h"

#include "rt64_render_hooks.h"
#include "rt64_render_interface_builders.h"

#include "RmlUi/Core.h"
#include "RmlUi/Debugger.h"
#include "RmlUi/Core/RenderInterfaceCompatibility.h"
#include "RmlUi/../../Source/Core/Elements/ElementLabel.h"
#include "RmlUi_Platform_SDL.h"

#include "InterfaceVS.hlsl.spirv.h"
#include "InterfacePS.hlsl.spirv.h"

#ifdef _WIN32
#   include "InterfaceVS.hlsl.dxil.h"
#   include "InterfacePS.hlsl.dxil.h"
#endif

#ifdef _WIN32
#    define GET_SHADER_BLOB(name, format) \
        ((format) == RT64::RenderShaderFormat::SPIRV ? name##BlobSPIRV : \
        (format) == RT64::RenderShaderFormat::DXIL ? name##BlobDXIL : nullptr)
#    define GET_SHADER_SIZE(name, format) \
        ((format) == RT64::RenderShaderFormat::SPIRV ? std::size(name##BlobSPIRV) : \
        (format) == RT64::RenderShaderFormat::DXIL ? std::size(name##BlobDXIL) : 0)
#else
#    define GET_SHADER_BLOB(name, format) \
        ((format) == RT64::RenderShaderFormat::SPIRV ? name##BlobSPIRV : nullptr)
#    define GET_SHADER_SIZE(name, format) \
        ((format) == RT64::RenderShaderFormat::SPIRV ? std::size(name##BlobSPIRV) : 0)
#endif

struct UIRenderContext {
    RT64::RenderInterface* interface;
    RT64::RenderDevice* device;
    Rml::ElementDocument* document;
};

// TODO deduplicate from rt64_common.h
void CalculateTextureRowWidthPadding(uint32_t rowPitch, uint32_t &rowWidth, uint32_t &rowPadding) {
    const int RowMultiple = 256;
    rowWidth = rowPitch;
    rowPadding = (rowWidth % RowMultiple) ? RowMultiple - (rowWidth % RowMultiple) : 0;
    rowWidth += rowPadding;
}

struct RmlPushConstants {
    Rml::Matrix4f transform;
    Rml::Vector2f translation;
};

struct TextureHandle {
    std::unique_ptr<RT64::RenderTexture> texture;
    std::unique_ptr<RT64::RenderDescriptorSet> set;
};

static std::vector<char> read_file(const std::filesystem::path& filepath) {
    std::vector<char> ret{};
    std::ifstream input_file{ filepath, std::ios::binary };

    if (!input_file) {
        return ret;
    }

    input_file.seekg(0, std::ios::end);
    std::streampos filesize = input_file.tellg();
    input_file.seekg(0, std::ios::beg);

    ret.resize(filesize);

    input_file.read(ret.data(), filesize);

    return ret;
}


template <typename T>
T from_bytes_le(const char* input) {
    return *reinterpret_cast<const T*>(input);
}

void load_document();

class RmlRenderInterface_RT64 : public Rml::RenderInterfaceCompatibility {
    static constexpr uint32_t per_frame_descriptor_set = 0;
    static constexpr uint32_t per_draw_descriptor_set = 1;

    static constexpr uint32_t initial_upload_buffer_size = 1024 * 1024;
    static constexpr uint32_t initial_vertex_buffer_size = 512 * sizeof(Rml::Vertex);
    static constexpr uint32_t initial_index_buffer_size = 1024 * sizeof(int);
    static constexpr RT64::RenderFormat RmlTextureFormat = RT64::RenderFormat::R8G8B8A8_UNORM;
    static constexpr RT64::RenderFormat RmlTextureFormatBgra = RT64::RenderFormat::B8G8R8A8_UNORM;
    static constexpr RT64::RenderFormat SwapChainFormat = RT64::RenderFormat::B8G8R8A8_UNORM;
    static constexpr uint32_t RmlTextureFormatBytesPerPixel = RenderFormatSize(RmlTextureFormat);
    static_assert(RenderFormatSize(RmlTextureFormatBgra) == RmlTextureFormatBytesPerPixel);
    struct UIRenderContext* render_context_;
    int scissor_x_ = 0;
    int scissor_y_ = 0;
    int scissor_width_ = 0;
    int scissor_height_ = 0;
    int window_width_ = 0;
    int window_height_ = 0;
    RT64::RenderMultisampling multisampling_ = RT64::RenderMultisampling();
    Rml::Matrix4f projection_mtx_ = Rml::Matrix4f::Identity();
    Rml::Matrix4f transform_ = Rml::Matrix4f::Identity();
    Rml::Matrix4f mvp_ = Rml::Matrix4f::Identity();
    std::unordered_map<Rml::TextureHandle, TextureHandle> textures_{};
    Rml::TextureHandle texture_count_ = 1; // Start at 1 to reserve texture 0 as the 1x1 pixel white texture
    std::unique_ptr<RT64::RenderBuffer> upload_buffer_{};
    std::unique_ptr<RT64::RenderBuffer> vertex_buffer_{};
    std::unique_ptr<RT64::RenderBuffer> index_buffer_{};
    std::unique_ptr<RT64::RenderSampler> nearestSampler_{};
    std::unique_ptr<RT64::RenderSampler> linearSampler_{};
    std::unique_ptr<RT64::RenderShader> vertex_shader_{};
    std::unique_ptr<RT64::RenderShader> pixel_shader_{};
    std::unique_ptr<RT64::RenderDescriptorSet> sampler_set_{};
    std::unique_ptr<RT64::RenderDescriptorSetBuilder> texture_set_builder_{};
    std::unique_ptr<RT64::RenderPipelineLayout> layout_{};
    std::unique_ptr<RT64::RenderPipeline> pipeline_{};
    std::unique_ptr<RT64::RenderPipeline> pipeline_ms_{};
    std::unique_ptr<RT64::RenderTexture> screen_texture_ms_{};
    std::unique_ptr<RT64::RenderTexture> screen_texture_{};
    std::unique_ptr<RT64::RenderFramebuffer> screen_framebuffer_{};
    std::unique_ptr<RT64::RenderDescriptorSet> screen_descriptor_set_{};
    std::unique_ptr<RT64::RenderBuffer> screen_vertex_buffer_{};
    uint64_t screen_vertex_buffer_size_ = 0;
    uint32_t upload_buffer_size_ = 0;
    uint32_t upload_buffer_bytes_used_ = 0;
    uint8_t* upload_buffer_mapped_data_ = nullptr;
    uint32_t vertex_buffer_size_ = 0;
    uint32_t index_buffer_size_ = 0;
    uint32_t gTexture_descriptor_index;
    RT64::RenderInputSlot vertex_slot_{ 0, sizeof(Rml::Vertex) };
    RT64::RenderCommandList* list_ = nullptr;
    bool scissor_enabled_ = false;
    std::vector<std::unique_ptr<RT64::RenderBuffer>> stale_buffers_{};
public:
    RmlRenderInterface_RT64(struct UIRenderContext* render_context) {
        render_context_ = render_context;

        // Enable 4X MSAA if supported by the device.
        const RT64::RenderSampleCounts desired_sample_count = RT64::RenderSampleCount::COUNT_8;
        if (render_context->device->getSampleCountsSupported(SwapChainFormat) & desired_sample_count) {
            multisampling_.sampleCount = desired_sample_count;
        }

        // Create the texture upload buffer, vertex buffer and index buffer
        resize_upload_buffer(initial_upload_buffer_size, false);
        resize_vertex_buffer(initial_vertex_buffer_size);
        resize_index_buffer(initial_index_buffer_size);

        // Describe the vertex format
        std::vector<RT64::RenderInputElement> vertex_elements{};
        vertex_elements.emplace_back(RT64::RenderInputElement{ "POSITION", 0, 0, RT64::RenderFormat::R32G32_FLOAT, 0, offsetof(Rml::Vertex, position) });
        vertex_elements.emplace_back(RT64::RenderInputElement{ "COLOR", 0, 1, RT64::RenderFormat::R8G8B8A8_UNORM, 0, offsetof(Rml::Vertex, colour) });
        vertex_elements.emplace_back(RT64::RenderInputElement{ "TEXCOORD", 0, 2, RT64::RenderFormat::R32G32_FLOAT, 0, offsetof(Rml::Vertex, tex_coord) });

        // Create a nearest sampler and a linear sampler
        RT64::RenderSamplerDesc samplerDesc;
        samplerDesc.minFilter = RT64::RenderFilter::NEAREST;
        samplerDesc.magFilter = RT64::RenderFilter::NEAREST;
        samplerDesc.addressU = RT64::RenderTextureAddressMode::CLAMP;
        samplerDesc.addressV = RT64::RenderTextureAddressMode::CLAMP;
        samplerDesc.addressW = RT64::RenderTextureAddressMode::CLAMP;
        nearestSampler_ = render_context->device->createSampler(samplerDesc);

        samplerDesc.minFilter = RT64::RenderFilter::LINEAR;
        samplerDesc.magFilter = RT64::RenderFilter::LINEAR;
        linearSampler_ = render_context->device->createSampler(samplerDesc);

        // Create the shaders
        RT64::RenderShaderFormat shaderFormat = render_context->interface->getCapabilities().shaderFormat;

        vertex_shader_ = render_context->device->createShader(GET_SHADER_BLOB(InterfaceVS, shaderFormat), GET_SHADER_SIZE(InterfaceVS, shaderFormat), "VSMain", shaderFormat);
        pixel_shader_ = render_context->device->createShader(GET_SHADER_BLOB(InterfacePS, shaderFormat), GET_SHADER_SIZE(InterfacePS, shaderFormat), "PSMain", shaderFormat);


        // Create the descriptor set that contains the sampler
        RT64::RenderDescriptorSetBuilder sampler_set_builder{};
        sampler_set_builder.begin();
        sampler_set_builder.addImmutableSampler(1, linearSampler_.get());
        sampler_set_builder.addConstantBuffer(3, 1); // Workaround D3D12 crash due to an empty RT64 descriptor set
        sampler_set_builder.end();
        sampler_set_ = sampler_set_builder.create(render_context->device);

        // Create a builder for the descriptor sets that will contain textures
        texture_set_builder_ = std::make_unique<RT64::RenderDescriptorSetBuilder>();
        texture_set_builder_->begin();
        gTexture_descriptor_index = texture_set_builder_->addTexture(2);
        texture_set_builder_->end();

        // Create the pipeline layout
        RT64::RenderPipelineLayoutBuilder layout_builder{};
        layout_builder.begin(false, true);
        layout_builder.addPushConstant(0, 0, sizeof(RmlPushConstants), RT64::RenderShaderStageFlag::VERTEX);
        // Add the descriptor set for descriptors changed once per frame.
        layout_builder.addDescriptorSet(sampler_set_builder);
        // Add the descriptor set for descriptors changed once per draw.
        layout_builder.addDescriptorSet(*texture_set_builder_);
        layout_builder.end();
        layout_ = layout_builder.create(render_context->device);

        // Create the pipeline description
        RT64::RenderGraphicsPipelineDesc pipeline_desc{};
        pipeline_desc.renderTargetBlend[0] = RT64::RenderBlendDesc::AlphaBlend();
        pipeline_desc.renderTargetFormat[0] = SwapChainFormat; // TODO: Use whatever format the swap chain was created with.
        pipeline_desc.renderTargetCount = 1;
        pipeline_desc.cullMode = RT64::RenderCullMode::NONE;
        pipeline_desc.inputSlots = &vertex_slot_;
        pipeline_desc.inputSlotsCount = 1;
        pipeline_desc.inputElements = vertex_elements.data();
        pipeline_desc.inputElementsCount = uint32_t(vertex_elements.size());
        pipeline_desc.pipelineLayout = layout_.get();
        pipeline_desc.primitiveTopology = RT64::RenderPrimitiveTopology::TRIANGLE_LIST;
        pipeline_desc.vertexShader = vertex_shader_.get();
        pipeline_desc.pixelShader = pixel_shader_.get();

        pipeline_ = render_context->device->createGraphicsPipeline(pipeline_desc);

        if (multisampling_.sampleCount > 1) {
            pipeline_desc.multisampling = multisampling_;
            pipeline_ms_ = render_context->device->createGraphicsPipeline(pipeline_desc);

            // Create the descriptor set for the screen drawer.
            RT64::RenderDescriptorRange screen_descriptor_range(RT64::RenderDescriptorRangeType::TEXTURE, 2, 1);
            screen_descriptor_set_ = render_context->device->createDescriptorSet(RT64::RenderDescriptorSetDesc(&screen_descriptor_range, 1));

            // Create vertex buffer for the screen drawer (full-screen triangle).
            screen_vertex_buffer_size_ = sizeof(Rml::Vertex) * 3;
            screen_vertex_buffer_ = render_context->device->createBuffer(RT64::RenderBufferDesc::VertexBuffer(screen_vertex_buffer_size_, RT64::RenderHeapType::UPLOAD));
            Rml::Vertex *vertices = (Rml::Vertex *)(screen_vertex_buffer_->map());
            const Rml::ColourbPremultiplied white(255, 255, 255, 255);
            vertices[0] = Rml::Vertex{ Rml::Vector2f(-1.0f, 1.0f), white, Rml::Vector2f(0.0f, 0.0f) };
            vertices[1] = Rml::Vertex{ Rml::Vector2f(-1.0f, -3.0f), white, Rml::Vector2f(0.0f, 2.0f) };
            vertices[2] = Rml::Vertex{ Rml::Vector2f(3.0f, 1.0f), white, Rml::Vector2f(2.0f, 0.0f) };
            screen_vertex_buffer_->unmap();
        }
    }

    void resize_upload_buffer(uint32_t new_size, bool map = true) {
        // Unmap the upload buffer if it's mapped
        if (upload_buffer_mapped_data_ != nullptr) {
            upload_buffer_->unmap();
        }
        
        // If there's already an upload buffer, move it into the stale buffers so it persists until the start of next frame.
        if (upload_buffer_) {
            stale_buffers_.emplace_back(std::move(upload_buffer_));
        }

        // Create the new upload buffer, update the size and map it.
        upload_buffer_ = render_context_->device->createBuffer(RT64::RenderBufferDesc::UploadBuffer(new_size));
        upload_buffer_size_ = new_size;
        upload_buffer_bytes_used_ = 0;
        if (map) {
            upload_buffer_mapped_data_ = reinterpret_cast<uint8_t*>(upload_buffer_->map());
        }
        else {
            upload_buffer_mapped_data_ = nullptr;
        }
    }

    uint32_t allocate_upload_data(uint32_t num_bytes) {
        // Check if there's enough remaining room in the upload buffer to allocate the requested bytes.
        uint32_t total_bytes = num_bytes + upload_buffer_bytes_used_;

        if (total_bytes > upload_buffer_size_) {
            // There isn't, so mark the current upload buffer as stale and allocate a new one with 50% more space than the required amount.
            resize_upload_buffer(total_bytes + total_bytes / 2);
        }

        // Record the current end of the upload buffer to return.
        uint32_t offset = upload_buffer_bytes_used_;

        // Bump the upload buffer's end forward by the number of bytes allocated.
        upload_buffer_bytes_used_ += num_bytes;

        return offset;
    }

    uint32_t allocate_upload_data_aligned(uint32_t num_bytes, uint32_t alignment) {
        // Check if there's enough remaining room in the upload buffer to allocate the requested bytes.
        uint32_t total_bytes = num_bytes + upload_buffer_bytes_used_;

        // Determine the amount of padding needed to meet the target alignment.
        uint32_t padding_bytes = ((upload_buffer_bytes_used_ + alignment - 1) / alignment) * alignment - upload_buffer_bytes_used_;

        // If there isn't enough room to allocate the required bytes plus the padding then resize the upload buffer and allocate from the start of the new one.
        if (total_bytes + padding_bytes > upload_buffer_size_) {
            resize_upload_buffer(total_bytes + total_bytes / 2);

            upload_buffer_bytes_used_ += num_bytes;

            return 0;
        }

        // Otherwise allocate the padding and required bytes and offset the allocated position by the padding size.
        return allocate_upload_data(padding_bytes + num_bytes) + padding_bytes;
    }

    void resize_vertex_buffer(uint32_t new_size) {
        if (vertex_buffer_) {
            stale_buffers_.emplace_back(std::move(vertex_buffer_));
        }
        vertex_buffer_ = render_context_->device->createBuffer(RT64::RenderBufferDesc::VertexBuffer(new_size, RT64::RenderHeapType::DEFAULT));
        vertex_buffer_size_ = new_size;
    }

    void resize_index_buffer(uint32_t new_size) {
        if (index_buffer_) {
            stale_buffers_.emplace_back(std::move(index_buffer_));
        }
        index_buffer_ = render_context_->device->createBuffer(RT64::RenderBufferDesc::IndexBuffer(new_size, RT64::RenderHeapType::DEFAULT));
        index_buffer_size_ = new_size;
    }

    void RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation) override {
        uint32_t vert_size_bytes = num_vertices * sizeof(*vertices);
        uint32_t index_size_bytes = num_indices * sizeof(*indices);
        uint32_t total_bytes = vert_size_bytes + index_size_bytes;
        uint32_t index_bytes_start = vert_size_bytes;


        if (!textures_.contains(texture)) {
            if (texture == 0) {
                // Create a 1x1 pixel white texture as the first handle
                Rml::byte white_pixel[] = { 255, 255, 255, 255 };
                create_texture(0, white_pixel, Rml::Vector2i{ 1,1 });
            }
            else {
                assert(false && "Rendered without texture!");
            }
        }

        uint32_t upload_buffer_offset = allocate_upload_data(total_bytes);

        if (vert_size_bytes > vertex_buffer_size_) {
            resize_vertex_buffer(vert_size_bytes + vert_size_bytes / 2);
        }

        if (index_size_bytes > index_buffer_size_) {
            resize_index_buffer(index_size_bytes + index_size_bytes / 2);
        }

        // Copy the vertex and index data into the mapped upload buffer.
        memcpy(upload_buffer_mapped_data_ + upload_buffer_offset, vertices, vert_size_bytes);
        memcpy(upload_buffer_mapped_data_ + upload_buffer_offset + vert_size_bytes, indices, index_size_bytes);

        // Prepare the vertex and index buffers for being copied to.
        RT64::RenderBufferBarrier copy_barriers[] = {
			RT64::RenderBufferBarrier(vertex_buffer_.get(), RT64::RenderBufferAccess::WRITE),
			RT64::RenderBufferBarrier(index_buffer_.get(), RT64::RenderBufferAccess::WRITE)
		};
        list_->barriers(RT64::RenderBarrierStage::COPY, copy_barriers, uint32_t(std::size(copy_barriers)));

        // Copy from the upload buffer to the vertex and index buffers.
        list_->copyBufferRegion(vertex_buffer_->at(0), upload_buffer_->at(upload_buffer_offset), vert_size_bytes);
        list_->copyBufferRegion(index_buffer_->at(0), upload_buffer_->at(upload_buffer_offset + index_bytes_start), index_size_bytes);

        // Prepare the vertex and index buffers for being used for rendering.
        RT64::RenderBufferBarrier usage_barriers[] = {
			RT64::RenderBufferBarrier(vertex_buffer_.get(), RT64::RenderBufferAccess::READ),
			RT64::RenderBufferBarrier(index_buffer_.get(), RT64::RenderBufferAccess::READ)
		};
        list_->barriers(RT64::RenderBarrierStage::GRAPHICS, usage_barriers, uint32_t(std::size(usage_barriers)));

        list_->setViewports(RT64::RenderViewport{ 0, 0, float(window_width_), float(window_height_) });
        if (scissor_enabled_) {
            list_->setScissors(RT64::RenderRect{
                scissor_x_,
                scissor_y_,
                (scissor_width_ + scissor_x_),
                (scissor_height_ + scissor_y_) });
        }
        else {
            list_->setScissors(RT64::RenderRect{ 0, 0, window_width_, window_height_ });
        }

        RT64::RenderIndexBufferView index_view{index_buffer_->at(0), index_size_bytes, RT64::RenderFormat::R32_UINT};
        list_->setIndexBuffer(&index_view);
        RT64::RenderVertexBufferView vertex_view{vertex_buffer_->at(0), vert_size_bytes};
        list_->setVertexBuffers(0, &vertex_view, 1, &vertex_slot_);
        list_->setGraphicsDescriptorSet(textures_.at(texture).set.get(), 1);

        RmlPushConstants constants{
            .transform = mvp_,
            .translation = translation
        };

        list_->setGraphicsPushConstants(0, &constants);

        list_->drawIndexedInstanced(num_indices, 1, 0, 0, 0);
    }

    void EnableScissorRegion(bool enable) override {
        scissor_enabled_ = enable;
    }

    void SetScissorRegion(int x, int y, int width, int height) override {
        scissor_x_ = x;
        scissor_y_ = y;
        scissor_width_ = width;
        scissor_height_ = height;
    }

    bool LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source) override {
        std::filesystem::path image_path{ source.c_str() };

        if (image_path.extension() == ".tga") {
            std::vector<char> file_data = read_file(image_path);

            if (file_data.empty()) {
                printf("  File not found or empty\n");
                return false;
            }

            // Make sure ID length is zero
            if (file_data[0] != 0) {
                printf("  Nonzero ID length not supported\n");
                return false;
            }

            // Make sure no color map is used
            if (file_data[1] != 0) {
                printf("  Color maps not supported\n");
                return false;
            }

            // Make sure the image is uncompressed
            if (file_data[2] != 2) {
                printf("  Only uncompressed tga files supported\n");
                return false;
            }

            uint16_t origin_x = from_bytes_le<uint16_t>(file_data.data() + 8);
            uint16_t origin_y = from_bytes_le<uint16_t>(file_data.data() + 10);
            uint16_t size_x = from_bytes_le<uint16_t>(file_data.data() + 12);
            uint16_t size_y = from_bytes_le<uint16_t>(file_data.data() + 14);

            // Nonzero origin not supported
            if (origin_x != 0 || origin_y != 0) {
                printf("  Nonzero origin not supported\n");
                return false;
            }

            uint8_t pixel_depth = file_data[16];

            if (pixel_depth != 32) {
                printf("  Only 32bpp images supported\n");
                return false;
            }

            uint8_t image_descriptor = file_data[17];

            if ((image_descriptor & 0b1111) != 8) {
                printf("  Only 8bpp alpha supported\n");
            }

            if (image_descriptor & 0b110000) {
                printf("  Only bottom-to-top, left-to-right pixel order supported\n");
            }

            texture_dimensions.x = size_x;
            texture_dimensions.y = size_y;

            texture_handle = texture_count_++;
            create_texture(texture_handle, reinterpret_cast<const Rml::byte*>(file_data.data() + 18), texture_dimensions, true, true);

            return true;
        }

        return false;
    }

    bool GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions) override {
        if (source_dimensions.x == 0 || source_dimensions.y == 0) {
            texture_handle = 0;
            return true;
        }

        texture_handle = texture_count_++;
        return create_texture(texture_handle, source, source_dimensions);
    }

    bool create_texture(Rml::TextureHandle texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions, bool flip_y = false, bool bgra = false) {
        std::unique_ptr<RT64::RenderTexture> texture =
            render_context_->device->createTexture(RT64::RenderTextureDesc::Texture2D(source_dimensions.x, source_dimensions.y, 1, bgra ? RmlTextureFormatBgra : RmlTextureFormat));

        if (texture != nullptr) {
            uint32_t image_size_bytes = source_dimensions.x * source_dimensions.y * RmlTextureFormatBytesPerPixel;

            // Calculate the texture padding for alignment purposes.
            uint32_t row_pitch = source_dimensions.x * RmlTextureFormatBytesPerPixel;
            uint32_t row_byte_width, row_byte_padding;
            CalculateTextureRowWidthPadding(row_pitch, row_byte_width, row_byte_padding);
            uint32_t row_width = row_byte_width / RmlTextureFormatBytesPerPixel;

            // Calculate the real number of bytes to upload including padding.
            uint32_t uploaded_size_bytes = row_byte_width * source_dimensions.y;

            // Allocate room in the upload buffer for the uploaded data.
            uint32_t upload_buffer_offset = allocate_upload_data_aligned(uploaded_size_bytes, 512);

            // Copy the source data into the upload buffer.
            uint8_t* dst_data = upload_buffer_mapped_data_ + upload_buffer_offset;
                
            if (row_byte_padding == 0) {
                // Copy row-by-row if the image is flipped.
                if (flip_y) {
                    for (int row = 0; row < source_dimensions.y; row++) {
                        memcpy(dst_data + row_byte_width * (source_dimensions.y - row - 1), source + row_byte_width * row, row_byte_width);
                    }
                }
                // Directly copy if no padding is needed and the image isn't flipped.
                else {
                    memcpy(dst_data, source, image_size_bytes);
                }
            }
            // Otherwise pad each row as necessary.
            else {
                const Rml::byte *src_data = flip_y ? source + row_pitch * (source_dimensions.y - 1) : source;
                uint32_t src_stride = flip_y ? -row_pitch : row_pitch;
                size_t offset = 0;

                for (int row = 0; row < source_dimensions.y; row++) { //(offset + increment) <= image_size_bytes) {
                    memcpy(dst_data, src_data, row_pitch);
                    src_data += src_stride;
                    offset += row_pitch;
                    dst_data += row_byte_width;
                }
            }

            // Prepare the texture to be a destination for copying.
            list_->barriers(RT64::RenderBarrierStage::COPY, RT64::RenderTextureBarrier(texture.get(), RT64::RenderTextureLayout::COPY_DEST));
            
            // Copy the upload buffer into the texture.
            list_->copyTextureRegion(
                RT64::RenderTextureCopyLocation::Subresource(texture.get()),
                RT64::RenderTextureCopyLocation::PlacedFootprint(upload_buffer_.get(), RmlTextureFormat, source_dimensions.x, source_dimensions.y, 1, row_width, upload_buffer_offset));
            
            // Prepare the texture for being read from a pixel shader.
            list_->barriers(RT64::RenderBarrierStage::GRAPHICS, RT64::RenderTextureBarrier(texture.get(), RT64::RenderTextureLayout::SHADER_READ));

            // Create a descriptor set with this texture in it.
            std::unique_ptr<RT64::RenderDescriptorSet> set = texture_set_builder_->create(render_context_->device);

            set->setTexture(gTexture_descriptor_index, texture.get(), RT64::RenderTextureLayout::SHADER_READ);

            textures_.emplace(texture_handle, TextureHandle{ std::move(texture), std::move(set) });

            return true;
        }

        return false;
    }

	void ReleaseTexture(Rml::TextureHandle texture) override {
        textures_.erase(texture);
    }

    void SetTransform(const Rml::Matrix4f* transform) override {
        transform_ = transform ? *transform : Rml::Matrix4f::Identity();
        recalculate_mvp();
    }

    void recalculate_mvp() {
        mvp_ = projection_mtx_ * transform_;
    }

    void start(RT64::RenderCommandList* list, int image_width, int image_height) {
        list_ = list;

        if (multisampling_.sampleCount > 1) {
            if (window_width_ != image_width || window_height_ != image_height) {
                screen_framebuffer_.reset();
                screen_texture_ = render_context_->device->createTexture(RT64::RenderTextureDesc::ColorTarget(image_width, image_height, SwapChainFormat));
                screen_texture_ms_ = render_context_->device->createTexture(RT64::RenderTextureDesc::ColorTarget(image_width, image_height, SwapChainFormat, multisampling_));
                const RT64::RenderTexture *color_attachment = screen_texture_ms_.get();
                screen_framebuffer_ = render_context_->device->createFramebuffer(RT64::RenderFramebufferDesc(&color_attachment, 1));
                screen_descriptor_set_->setTexture(0, screen_texture_.get(), RT64::RenderTextureLayout::SHADER_READ);
            }

            list_->setPipeline(pipeline_ms_.get());
        }
        else {
            list_->setPipeline(pipeline_.get());
        }

        list_->setGraphicsPipelineLayout(layout_.get());
        // Bind the set for descriptors that don't change across draws
        list_->setGraphicsDescriptorSet(sampler_set_.get(), 0);

        window_width_ = image_width;
        window_height_ = image_height;

        projection_mtx_ = Rml::Matrix4f::ProjectOrtho(0.0f, float(image_width), float(image_height), 0.0f, -10000, 10000);
        recalculate_mvp();

        // The following code assumes command lists aren't double buffered.
        // Clear out any stale buffers from the last command list.
        stale_buffers_.clear();

        // Reset and map the upload buffer.
        upload_buffer_bytes_used_ = 0;
        upload_buffer_mapped_data_ = reinterpret_cast<uint8_t*>(upload_buffer_->map());

        // Set an internal texture as the render target if MSAA is enabled.
        if (multisampling_.sampleCount > 1) {
            list->barriers(RT64::RenderBarrierStage::GRAPHICS, RT64::RenderTextureBarrier(screen_texture_ms_.get(), RT64::RenderTextureLayout::COLOR_WRITE));
            list->setFramebuffer(screen_framebuffer_.get());
            list->clearColor(0, RT64::RenderColor(0.0f, 0.0f, 0.0f, 0.0f));
        }
    }

    void end(RT64::RenderCommandList* list, RT64::RenderFramebuffer* framebuffer) {
        // Draw the texture were rendered the UI in to the swap chain framebuffer if MSAA is enabled.
        if (multisampling_.sampleCount > 1) {
            RT64::RenderTextureBarrier before_resolve_barriers[] = {
                RT64::RenderTextureBarrier(screen_texture_ms_.get(), RT64::RenderTextureLayout::RESOLVE_SOURCE),
                RT64::RenderTextureBarrier(screen_texture_.get(), RT64::RenderTextureLayout::RESOLVE_DEST)
            };

            list->barriers(RT64::RenderBarrierStage::COPY, before_resolve_barriers, uint32_t(std::size(before_resolve_barriers)));
            list->resolveTexture(screen_texture_.get(), screen_texture_ms_.get());
            list->barriers(RT64::RenderBarrierStage::GRAPHICS, RT64::RenderTextureBarrier(screen_texture_.get(), RT64::RenderTextureLayout::SHADER_READ));
            list->setFramebuffer(framebuffer);
            list->setPipeline(pipeline_.get());
            list->setGraphicsPipelineLayout(layout_.get());
            list->setGraphicsDescriptorSet(sampler_set_.get(), 0);
            list->setGraphicsDescriptorSet(screen_descriptor_set_.get(), 1);
            RT64::RenderVertexBufferView vertex_view(screen_vertex_buffer_.get(), screen_vertex_buffer_size_);
            list->setVertexBuffers(0, &vertex_view, 1, &vertex_slot_);

            RmlPushConstants constants{
                .transform = Rml::Matrix4f::Identity(),
                .translation = Rml::Vector2f(0.0f, 0.0f)
            };

            list_->setGraphicsPushConstants(0, &constants);
            list->drawInstanced(3, 1, 0, 0);
        }

        list_ = nullptr;

        // Unmap the upload buffer if it's mapped.
        if (upload_buffer_mapped_data_) {
            upload_buffer_->unmap();
            upload_buffer_mapped_data_ = nullptr;
        }
    }
};

bool can_focus(Rml::Element* element) {
    return element->GetOwnerDocument() != nullptr && element->GetProperty(Rml::PropertyId::TabIndex)->Get<Rml::Style::TabIndex>() != Rml::Style::TabIndex::None;
}

//! Copied from lib\RmlUi\Source\Core\Elements\ElementLabel.cpp
// Get the first descending element whose tag name matches one of tags.
static Rml::Element* TagMatchRecursive(const Rml::StringList& tags, Rml::Element* element)
{
	const int num_children = element->GetNumChildren();

	for (int i = 0; i < num_children; i++)
	{
		Rml::Element* child = element->GetChild(i);

		for (const Rml::String& tag : tags)
		{
			if (child->GetTagName() == tag)
				return child;
		}

		Rml::Element* matching_element = TagMatchRecursive(tags, child);
		if (matching_element)
			return matching_element;
	}

	return nullptr;
}

Rml::Element* get_target(Rml::ElementDocument* document, Rml::Element* element) {
    // Labels can have targets, so check if this element is a label.
    if (element->GetTagName() == "label") {
        Rml::ElementLabel* labelElement = (Rml::ElementLabel*)element;
        const Rml::String target_id = labelElement->GetAttribute<Rml::String>("for", "");

        if (target_id.empty())
        {
            const Rml::StringList matching_tags = {"button", "input", "textarea", "progress", "progressbar", "select"};

            return TagMatchRecursive(matching_tags, element);
        }
        else
        {
            Rml::Element* target = labelElement->GetElementById(target_id);
            if (target != element)
                return target;
        }

        return nullptr;
    }
    // Return the element directly if no target exists.
    return element;
}

namespace recompui {
    class UiEventListener : public Rml::EventListener {
        event_handler_t* handler_;
        Rml::String param_;
    public:
        UiEventListener(event_handler_t* handler, Rml::String&& param) : handler_(handler), param_(std::move(param)) {}
        void ProcessEvent(Rml::Event& event) override {
            handler_(param_, event);
        }
    };

    class UiEventListenerInstancer : public Rml::EventListenerInstancer {
        std::unordered_map<Rml::String, event_handler_t*> handler_map_;
        std::unordered_map<Rml::String, UiEventListener> listener_map_;
    public:
        Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* element) override {
            // Check if a listener has already been made for the full event string and return it if so.
            auto find_listener_it = listener_map_.find(value);
            if (find_listener_it != listener_map_.end()) {
                return &find_listener_it->second;
            }

            // No existing listener, so check if a handler has been registered for this event type and create a listener for it if so.
            size_t delimiter_pos = value.find(':');
            Rml::String event_type = value.substr(0, delimiter_pos);
            auto find_handler_it = handler_map_.find(event_type);
            if (find_handler_it != handler_map_.end()) {
                // A handler was found, create a listener and return it.
                Rml::String event_param = value.substr(std::min(delimiter_pos, value.size()));
                return &listener_map_.emplace(value, UiEventListener{ find_handler_it->second, std::move(event_param) }).first->second;
            }

            return nullptr;
        }

        void register_event(const Rml::String& value, event_handler_t* handler) {
            handler_map_.emplace(value, handler);
        }
    };
}

void recompui::register_event(UiEventListenerInstancer& listener, const std::string& name, event_handler_t* handler) {
    listener.register_event(name, handler);
}

Rml::Element* find_autofocus_element(Rml::Element* start) {
    Rml::Element* cur_element = start;

    while (cur_element) {
        if (cur_element->HasAttribute("autofocus")) {
            break;
        }
        cur_element = RecompRml::FindNextTabElement(cur_element, true);
    }

    return cur_element;
}

struct UIContext {
    struct UIRenderContext render;
    class {
        std::unordered_map<recompui::Menu, std::unique_ptr<recompui::MenuController>> menus;
        std::unordered_map<recompui::Menu, Rml::ElementDocument*> documents;
        Rml::ElementDocument* current_document;
        Rml::Element* prev_focused;
        bool mouse_is_active_changed = false;
    public:
        bool mouse_is_active_initialized = false;
        bool mouse_is_active = false;
        bool cont_is_active = false;
        bool await_stick_return_x = false;
        bool await_stick_return_y = false;
        int last_active_mouse_position[2] = {0, 0};
        std::unique_ptr<SystemInterface_SDL> system_interface;
        std::unique_ptr<RmlRenderInterface_RT64> render_interface;
        Rml::Context* context;
        recompui::UiEventListenerInstancer event_listener_instancer;

        void unload() {
            render_interface.reset();
        }

        void swap_document(recompui::Menu menu) {
            if (current_document != nullptr) {
                Rml::Element* window_el = current_document->GetElementById("window");
                if (window_el != nullptr) {
                    window_el->SetClassNames("rmlui-window rmlui-window--hidden");
                }
                current_document->Hide();
            }

            auto find_it = documents.find(menu);
            if (find_it != documents.end()) {
                assert(find_it->second && "Document for menu not loaded!");
                current_document = find_it->second;
                Rml::Element* window_el = current_document->GetElementById("window");
                if (window_el != nullptr) {
                    window_el->SetClassNames("rmlui-window rmlui-window--hidden");
                }
                current_document->Show();
                if (window_el != nullptr) {
                    window_el->SetClassNames("rmlui-window");
                }
            }
            else {
                current_document = nullptr;
            }
            prev_focused = nullptr;
            mouse_is_active = false;
            mouse_is_active_changed = false;
            mouse_is_active_initialized = false;
        }

        void swap_config_menu(recompui::ConfigSubmenu submenu) {
            if (current_document != nullptr) {
                Rml::Element* config_tabset_base = current_document->GetElementById("config_tabset");
                if (config_tabset_base != nullptr) {
                    Rml::ElementTabSet* config_tabset = rmlui_dynamic_cast<Rml::ElementTabSet*>(config_tabset_base);
                    if (config_tabset != nullptr) {
                        config_tabset->SetActiveTab(static_cast<int>(submenu));
                        prev_focused = nullptr;
                        mouse_is_active = false;
                        mouse_is_active_changed = false;
                        mouse_is_active_initialized = false;
                    }
                }
            }
        }

        void load_documents() {
            if (!documents.empty()) {
                Rml::Factory::RegisterEventListenerInstancer(nullptr);
                for (auto doc : documents) {
                    doc.second->ReloadStyleSheet();
                }

                Rml::ReleaseTextures();
                Rml::ReleaseMemoryPools();

                if (current_document != nullptr) {
                    current_document->Close();
                }

                current_document = nullptr;

                documents.clear();
                Rml::Factory::RegisterEventListenerInstancer(&event_listener_instancer);
            }

            for (auto& [menu, controller]: menus) {
                documents.emplace(menu, controller->load_document(context));
            }

            prev_focused = nullptr;
            mouse_is_active = false;
            mouse_is_active_changed = false;
            mouse_is_active_initialized = false;
        }

        void make_event_listeners() {
            for (auto& [menu, controller]: menus) {
                controller->register_events(event_listener_instancer);
            }
        }

        void make_bindings() {
            for (auto& [menu, controller]: menus) {
                controller->make_bindings(context);
            }
        }

        void update_primary_input(bool mouse_moved, bool non_mouse_interacted) {
            mouse_is_active_changed = false;
            if (non_mouse_interacted) {
                // controller newly interacted with
                if (mouse_is_active) {
                    mouse_is_active = false;
                    mouse_is_active_changed = true;
                }
            }
            else if (mouse_moved) {
                // mouse newly interacted with
                if (!mouse_is_active) {
                    mouse_is_active = true;
                    mouse_is_active_changed = true;
                }
            }

            if (mouse_moved || non_mouse_interacted) {
                mouse_is_active_initialized = true;
            }

            if (mouse_is_active_initialized) {
                recompui::set_cursor_visible(mouse_is_active);
            }

            if (current_document == nullptr) {
                return;
            }

            Rml::Element* window_el = current_document->GetElementById("window");
            if (window_el != nullptr) {
                if (mouse_is_active) {
                    if (!window_el->HasAttribute("mouse-active")) {
                        window_el->SetAttribute("mouse-active", true);
                    }
                }
                else if (window_el->HasAttribute("mouse-active")) {
                    window_el->RemoveAttribute("mouse-active");
                }
            }
        }

        void update_focus(bool mouse_moved, bool non_mouse_interacted) {
            if (current_document == nullptr) {
                return;
            }

            if (cont_is_active || non_mouse_interacted) {
                if (non_mouse_interacted) {
                    auto focusedEl = current_document->GetFocusLeafNode();
                    if (focusedEl == nullptr || RecompRml::CanFocusElement(focusedEl) != RecompRml::CanFocus::Yes) {
                        Rml::Element* element = find_autofocus_element(current_document);
                        if (element != nullptr) {
                            element->Focus();
                        }
                    }
                }
                return;
            }

            // If there was mouse motion, get the current hovered element (or its target if it points to one) and focus that if applicable.
            if (mouse_is_active) {
                if (mouse_is_active_changed) {
                    Rml::Element* focused = current_document->GetFocusLeafNode();
                    if (focused) focused->Blur();
                } else if (mouse_moved) {
                    Rml::Element* hovered = context->GetHoverElement();
                    if (hovered) {
                        Rml::Element* hover_target = get_target(current_document, hovered);
                        if (hover_target && can_focus(hover_target)) {
                            prev_focused = hover_target;
                        }
                    }
                }
            }

            if (!mouse_is_active) {
                if (!prev_focused || !can_focus(prev_focused)) {
                    // Find the autofocus element in the tab chain
                    Rml::Element* element = find_autofocus_element(current_document);
                    if (element && can_focus(element)) {
                        prev_focused = element;
                    }
                }

                if (mouse_is_active_changed && prev_focused && can_focus(prev_focused)) {
                    prev_focused->Focus();
                }
            }
        }

        void add_menu(recompui::Menu menu, std::unique_ptr<recompui::MenuController>&& controller) {
            menus.emplace(menu, std::move(controller));
        }
        
        void update_config_menu_loop(bool menu_changed) {
            static int prevTab = -1;
            if (menu_changed) prevTab = -1;
            recompui::update_rml_display_refresh_rate();

            Rml::ElementTabSet *tabset = (Rml::ElementTabSet *)current_document->GetElementById("config_tabset");
            if (tabset == nullptr) return;

            int curTab = tabset->GetActiveTab();
            if (curTab == prevTab) return;
            prevTab = curTab;

            Rml::ElementList panels;
            current_document->GetElementsByTagName(panels, "panel");
            
            Rml::Element *firstFocus = nullptr;
            for (const auto& panel : panels) {
                if (panel->IsVisible()) {
                    firstFocus = RecompRml::FindNextTabElement(panel, true);
                    break;
                }
            }

            if (!firstFocus) return;
            Rml::String id = firstFocus->GetId();
            if (id.empty()) return;

            Rml::ElementList tabs;
            current_document->GetElementsByTagName(tabs, "tab");
            for (const auto& tab : tabs) {
                tab->SetProperty("nav-down", "#" + id);
            }
        }
        
        void update_prompt_loop(void) {
            static bool wasShowingPrompt = false;

            recompui::PromptContext *ctx = recompui::get_prompt_context();
            if (!ctx->open && wasShowingPrompt) {
                Rml::Element* focused = current_document->GetFocusLeafNode();
                if (focused) focused->Blur();

                bool didFocus = false;

                if (ctx->returnElementId.size() > 0) {
                    Rml::Element *retEl = current_document->GetElementById(ctx->returnElementId);
                    if (retEl != nullptr && retEl->IsVisible()) {
                        retEl->Focus(true);
                        didFocus = true;
                    }
                }

                if (!didFocus) {
                    Rml::ElementList tabs;
                    current_document->GetElementsByTagName(tabs, "tab");
                    for (const auto& tab : tabs) {
                        if (tab->IsVisible()) {
                            tab->Focus(true);
                            break;
                        }
                    }
                }
            }
            wasShowingPrompt = ctx->open;

            if (!ctx->open) {
                return;
            }

            Rml::Element* focused = current_document->GetFocusLeafNode();
            // Check if unfocused or current focus isn't either prompt button
            if (mouse_is_active == false) {
                if (
                    focused == nullptr || (
                        focused != current_document->GetElementById("prompt__cancel-button") &&
                        focused != current_document->GetElementById("prompt__confirm-button")
                    )
                ) {
                    ctx->shouldFocus = true;
                }
            }

            if (!ctx->shouldFocus) {
                return;
            }

            if (focused != nullptr) {
                focused->Blur();
            }

            Rml::Element *targetButton = current_document->GetElementById(
                ctx->focusOnCancel ? "prompt__cancel-button" :  "prompt__confirm-button");

            if (targetButton == nullptr) {
                return;
            }

            targetButton->Focus(true);

            ctx->shouldFocus = false;

            Rml::Element *confirmButton = current_document->GetElementById("prompt__confirm-button");
            Rml::Element *cancelButton  = current_document->GetElementById("prompt__cancel-button");
            if (confirmButton != nullptr) confirmButton->SetClassNames("button button--" + recompui::button_variants.at(ctx->confirmVariant));
            if (cancelButton  != nullptr) cancelButton->SetClassNames( "button button--" + recompui::button_variants.at(ctx->cancelVariant));
        }
    } rml;
};

std::unique_ptr<UIContext> ui_context;
std::mutex ui_context_mutex{};

// TODO make this not be global
extern SDL_Window* window;

void recompui::get_window_size(int& width, int& height) {
    SDL_GetWindowSizeInPixels(window, &width, &height);
}

void init_hook(RT64::RenderInterface* interface, RT64::RenderDevice* device) {
#if defined(__linux__)
    std::locale::global(std::locale::classic());
#endif
    ui_context = std::make_unique<UIContext>();

    ui_context->rml.add_menu(recompui::Menu::Config, recompui::create_config_menu());
    ui_context->rml.add_menu(recompui::Menu::Launcher, recompui::create_launcher_menu());

    ui_context->render.interface = interface;
    ui_context->render.device = device;

    // Setup RML
    ui_context->rml.system_interface = std::make_unique<SystemInterface_SDL>();
    ui_context->rml.system_interface->SetWindow(window);
    ui_context->rml.render_interface = std::make_unique<RmlRenderInterface_RT64>(&ui_context->render);
    ui_context->rml.make_event_listeners();

    Rml::SetSystemInterface(ui_context->rml.system_interface.get());
    Rml::SetRenderInterface(ui_context->rml.render_interface.get()->GetAdaptedInterface());
    Rml::Factory::RegisterEventListenerInstancer(&ui_context->rml.event_listener_instancer);

    Rml::Initialise();

    // Apply the hack to replace RmlUi's default color parser with one that conforms to HTML5 alpha parsing for SASS compatibility
    recompui::apply_color_hack();

    int width, height;
    SDL_GetWindowSizeInPixels(window, &width, &height);
    
    ui_context->rml.context = Rml::CreateContext("main", Rml::Vector2i(width, height));
    ui_context->rml.make_bindings();

    Rml::Debugger::Initialise(ui_context->rml.context);

    {
        const Rml::String directory = "assets/";

        struct FontFace {
            const char* filename;
            bool fallback_face;
        };
        FontFace font_faces[] = {
            {"LatoLatin-Regular.ttf", false},
            {"ChiaroNormal.otf", false},
            {"ChiaroBold.otf", false},
            {"LatoLatin-Italic.ttf", false},
            {"LatoLatin-Bold.ttf", false},
            {"LatoLatin-BoldItalic.ttf", false},
            {"NotoEmoji-Regular.ttf", true},
            {"promptfont/promptfont.ttf", false},
        };

        for (const FontFace& face : font_faces) {
            Rml::LoadFontFace(directory + face.filename, face.fallback_face);
        }
    }

    ui_context->rml.load_documents();
}

moodycamel::ConcurrentQueue<SDL_Event> ui_event_queue{};

void recompui::queue_event(const SDL_Event& event) {
    ui_event_queue.enqueue(event);
}

bool recompui::try_deque_event(SDL_Event& out) {
    return ui_event_queue.try_dequeue(out);
}

std::atomic<recompui::Menu> open_menu = recompui::Menu::Launcher;
std::atomic<recompui::ConfigSubmenu> open_config_submenu = recompui::ConfigSubmenu::Count;

int cont_button_to_key(SDL_ControllerButtonEvent& button) {
    // Configurable accept button in menu
    auto menuAcceptBinding0 = recomp::get_input_binding(recomp::GameInput::ACCEPT_MENU, 0, recomp::InputDevice::Controller);
    auto menuAcceptBinding1 = recomp::get_input_binding(recomp::GameInput::ACCEPT_MENU, 1, recomp::InputDevice::Controller);
    // note - magic number: 0 is InputType::None
    if ((menuAcceptBinding0.input_type != 0 && button.button == menuAcceptBinding0.input_id) ||
        (menuAcceptBinding1.input_type != 0 && button.button == menuAcceptBinding1.input_id)) {
        return SDLK_RETURN;
    }

    // Configurable apply button in menu
    auto menuApplyBinding0 = recomp::get_input_binding(recomp::GameInput::APPLY_MENU, 0, recomp::InputDevice::Controller);
    auto menuApplyBinding1 = recomp::get_input_binding(recomp::GameInput::APPLY_MENU, 1, recomp::InputDevice::Controller);
    // note - magic number: 0 is InputType::None
    if ((menuApplyBinding0.input_type != 0 && button.button == menuApplyBinding0.input_id) ||
        (menuApplyBinding1.input_type != 0 && button.button == menuApplyBinding1.input_id)) {
        return SDLK_f;
    } 

    // Allows closing the menu
    auto menuToggleBinding0 = recomp::get_input_binding(recomp::GameInput::TOGGLE_MENU, 0, recomp::InputDevice::Controller);
    auto menuToggleBinding1 = recomp::get_input_binding(recomp::GameInput::TOGGLE_MENU, 1, recomp::InputDevice::Controller);
    // note - magic number: 0 is InputType::None
    if ((menuToggleBinding0.input_type != 0 && button.button == menuToggleBinding0.input_id) ||
        (menuToggleBinding1.input_type != 0 && button.button == menuToggleBinding1.input_id)) {
        return SDLK_ESCAPE;
    }

    switch (button.button) {
        case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP:
            return SDLK_UP;
        case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return SDLK_DOWN;
        case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            return SDLK_LEFT;
        case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return SDLK_RIGHT;
    }

    return 0;
}


int cont_axis_to_key(SDL_ControllerAxisEvent& axis, float value) {
    switch (axis.axis) {
    case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY:
        if (value < 0) return SDLK_UP;
        return SDLK_DOWN;
    case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX:
        if (value >= 0) return SDLK_RIGHT;
        return SDLK_LEFT;
    }
    return 0;
}

void apply_background_input_mode() {
    static recomp::BackgroundInputMode last_input_mode = recomp::BackgroundInputMode::OptionCount;

    recomp::BackgroundInputMode cur_input_mode = recomp::get_background_input_mode();

    if (last_input_mode != cur_input_mode) {
        SDL_SetHint(
            SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,
            cur_input_mode == recomp::BackgroundInputMode::On
                ? "1"
                : "0"
        );
    }
    last_input_mode = cur_input_mode;
}

bool recompui::get_cont_active() {
    return ui_context->rml.cont_is_active;
}

void recompui::set_cont_active(bool active) {
    ui_context->rml.cont_is_active = active;
}

void recompui::activate_mouse() {
    ui_context->rml.update_primary_input(true, false);
    ui_context->rml.update_focus(true, false);
}

void draw_hook(RT64::RenderCommandList* command_list, RT64::RenderFramebuffer* swap_chain_framebuffer) {
    std::lock_guard lock {ui_context_mutex};

    apply_background_input_mode();

    // Return early if the ui context has been destroyed already.
    if (!ui_context) {
        return;
    }

    int num_keys;
    const Uint8* key_state = SDL_GetKeyboardState(&num_keys);

    static bool was_reload_held = false;
    bool is_reload_held = key_state[SDL_SCANCODE_F10] != 0;
    bool reload_sheets = is_reload_held && !was_reload_held;
    was_reload_held = is_reload_held;

    static recompui::Menu prev_menu = recompui::Menu::None;
    recompui::Menu cur_menu = open_menu.load();

    if (reload_sheets) {
        ui_context->rml.load_documents();
        prev_menu = recompui::Menu::None;
    }

    bool menu_changed = cur_menu != prev_menu;
    if (menu_changed) {
        ui_context->rml.swap_document(cur_menu);
    }

    recompui::ConfigSubmenu config_submenu = open_config_submenu.load();
    if (config_submenu != recompui::ConfigSubmenu::Count) {
        ui_context->rml.swap_config_menu(config_submenu);
        open_config_submenu.store(recompui::ConfigSubmenu::Count);
    }

    prev_menu = cur_menu;

    SDL_Event cur_event{};

    bool mouse_moved = false;
    bool mouse_clicked = false;
    bool non_mouse_interacted = false;
    bool cont_interacted = false;
    bool kb_interacted = false;

    if (cur_menu == recompui::Menu::Config) {
        ui_context->rml.update_config_menu_loop(menu_changed);
    }
    if (cur_menu != recompui::Menu::None) {
        ui_context->rml.update_prompt_loop();
    }

    while (recompui::try_deque_event(cur_event)) {
        bool menu_is_open = cur_menu != recompui::Menu::None;

        if (!recomp::all_input_disabled()) {
            // Implement some additional behavior for specific events on top of what RmlUi normally does with them.
            switch (cur_event.type) {
            case SDL_EventType::SDL_MOUSEMOTION: {
                int *last_mouse_pos = ui_context->rml.last_active_mouse_position;

                if (!ui_context->rml.mouse_is_active) {
                    float xD = cur_event.motion.x - last_mouse_pos[0];
                    float yD = cur_event.motion.y - last_mouse_pos[1];
                    if (sqrt(xD * xD + yD * yD) < 100) {
                        break;
                    }
                }
                last_mouse_pos[0] = cur_event.motion.x;
                last_mouse_pos[1] = cur_event.motion.y;

                // if controller is the primary input, don't use mouse movement to allow cursor to reactivate
                if (recompui::get_cont_active()) {
                    break;
                }
            }
            // fallthrough
            case SDL_EventType::SDL_MOUSEBUTTONDOWN:
                mouse_moved = true;
                mouse_clicked = true;
                break;
                
            case SDL_EventType::SDL_CONTROLLERBUTTONDOWN: {
                int rml_key = cont_button_to_key(cur_event.cbutton);
                if (menu_is_open && rml_key) {
                    ui_context->rml.context->ProcessKeyDown(RmlSDL::ConvertKey(rml_key), 0);
                }
                non_mouse_interacted = true;
                cont_interacted = true;
                break;
            }
            case SDL_EventType::SDL_KEYDOWN:
                non_mouse_interacted = true;
                kb_interacted = true;
                break;
            case SDL_EventType::SDL_USEREVENT:
                if (cur_event.user.code == SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY) {
                    ui_context->rml.await_stick_return_y = true;
                } else if (cur_event.user.code == SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX) {
                    ui_context->rml.await_stick_return_x = true;
                }
                break;
            case SDL_EventType::SDL_CONTROLLERAXISMOTION:
                SDL_ControllerAxisEvent* axis_event = &cur_event.caxis;
                if (axis_event->axis != SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY && axis_event->axis != SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX) {
                    break;
                }

                float axis_value = axis_event->value * (1 / 32768.0f);
                bool* await_stick_return = axis_event->axis == SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY
                        ? &ui_context->rml.await_stick_return_y
                        : &ui_context->rml.await_stick_return_x;
                if (fabsf(axis_value) > 0.5f) {
                    if (!*await_stick_return) {
                        *await_stick_return = true;
                        non_mouse_interacted = true;
                        int rml_key = cont_axis_to_key(cur_event.caxis, axis_value);
                        if (menu_is_open && rml_key) {
                            ui_context->rml.context->ProcessKeyDown(RmlSDL::ConvertKey(rml_key), 0);
                        }
                    }
                    non_mouse_interacted = true;
                    cont_interacted = true;
                }
                else if (*await_stick_return && fabsf(axis_value) < 0.15f) {
                    *await_stick_return = false;
                }
                break;
            }

            if (menu_is_open) {
                RmlSDL::InputEventHandler(ui_context->rml.context, cur_event);
            }
        }

        // If no menu is open and the game has been started and either the escape key or select button are pressed, open the config menu.
        if (!menu_is_open && ultramodern::is_game_started()) {
            bool open_config = false;

            switch (cur_event.type) {
            case SDL_EventType::SDL_KEYDOWN:
                if (cur_event.key.keysym.scancode == SDL_Scancode::SDL_SCANCODE_ESCAPE) {
                    open_config = true;
                }
                break;
            case SDL_EventType::SDL_CONTROLLERBUTTONDOWN:
                auto menuToggleBinding0 = recomp::get_input_binding(recomp::GameInput::TOGGLE_MENU, 0, recomp::InputDevice::Controller);
                auto menuToggleBinding1 = recomp::get_input_binding(recomp::GameInput::TOGGLE_MENU, 1, recomp::InputDevice::Controller);
                // note - magic number: 0 is InputType::None
                if ((menuToggleBinding0.input_type != 0 && cur_event.cbutton.button == menuToggleBinding0.input_id) ||
                    (menuToggleBinding1.input_type != 0 && cur_event.cbutton.button == menuToggleBinding1.input_id)) {
                    open_config = true;
                }
                break;
            }

            if (open_config) {
                cur_menu = recompui::Menu::Config;
                open_menu.store(recompui::Menu::Config);
                ui_context->rml.swap_document(cur_menu);
            }
        }
    } // end dequeue event loop

    if (cont_interacted || kb_interacted || mouse_clicked) {
        recompui::set_cont_active(cont_interacted);
    }
    recomp::config_menu_set_cont_or_kb(ui_context->rml.cont_is_active);

    recomp::InputField scanned_field = recomp::get_scanned_input();
    if (scanned_field != recomp::InputField{}) {
        recomp::finish_scanning_input(scanned_field);
    }

    ui_context->rml.update_primary_input(mouse_moved, non_mouse_interacted);
    ui_context->rml.update_focus(mouse_moved, non_mouse_interacted);

    if (cur_menu != recompui::Menu::None) {
        int width = swap_chain_framebuffer->getWidth();
        int height = swap_chain_framebuffer->getHeight();

        // Scale the UI based on the window size with 1080 vertical resolution as the reference point.
        ui_context->rml.context->SetDensityIndependentPixelRatio((height) / 1080.0f);

        ui_context->rml.render_interface->start(command_list, width, height);

        static int prev_width = 0;
        static int prev_height = 0;

        if (prev_width != width || prev_height != height) {
            ui_context->rml.context->SetDimensions({ width, height });
        }
        prev_width = width;
        prev_height = height;

        ui_context->rml.context->Update();
        ui_context->rml.context->Render();
        ui_context->rml.render_interface->end(command_list, swap_chain_framebuffer);
    }
}

void deinit_hook() {
    std::lock_guard lock {ui_context_mutex};
    Rml::Debugger::Shutdown();
    Rml::Shutdown();
    ui_context->rml.unload();
    ui_context.reset();
}

void recompui::set_render_hooks() {
    RT64::SetRenderHooks(init_hook, draw_hook, deinit_hook);
}

void recompui::set_current_menu(Menu menu) {
    open_menu.store(menu);
    if (menu == recompui::Menu::None) {
        ui_context->rml.system_interface->SetMouseCursor("arrow");
    }
}

void recompui::set_config_submenu(recompui::ConfigSubmenu submenu) {
	open_config_submenu.store(submenu);
}

void recompui::destroy_ui() {
}

recompui::Menu recompui::get_current_menu() {
    return open_menu.load();
}

void recompui::message_box(const char* msg) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, zelda64::program_name.data(), msg, nullptr);
    printf("[ERROR] %s\n", msg);
}
