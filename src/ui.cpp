#include "rt64_layer.h"
#include "rt64_render_hooks.h"
#include "rt64_render_interface_builders.h"

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
        ((format) == RT64::RenderShaderFormat::SPIRV ? std::size(name##BlobSPIRV) : nullptr)
#    define GET_SHADER_SIZE(name, format) \
        ((format) == RT64::RenderShaderFormat::SPIRV ? std::size(name##BlobSPIRV) : 0)
#endif

struct {
    struct {
        std::unique_ptr<RT64::RenderPipelineLayout> layout;
        std::unique_ptr<RT64::RenderShader> vertex_shader;
        std::unique_ptr<RT64::RenderShader> pixel_shader;
        std::unique_ptr<RT64::RenderPipeline> pipeline;
        std::unique_ptr<RT64::RenderBuffer> index_buffer;
        RT64::RenderIndexBufferView index_buffer_view;
    } Interface;
} UIContext;

void init_hook(RT64::RenderInterface* interface, RT64::RenderDevice* device) {
    printf("RT64 hook init\n");

    RT64::RenderPipelineLayoutBuilder layout_builder{};
    layout_builder.begin();

    layout_builder.end();
    UIContext.Interface.layout = layout_builder.create(device);

    RT64::RenderShaderFormat shaderFormat = interface->getCapabilities().shaderFormat;

    UIContext.Interface.vertex_shader = device->createShader(GET_SHADER_BLOB(InterfaceVS, shaderFormat), GET_SHADER_SIZE(InterfaceVS, shaderFormat), "VSMain", shaderFormat);
    UIContext.Interface.pixel_shader = device->createShader(GET_SHADER_BLOB(InterfacePS, shaderFormat), GET_SHADER_SIZE(InterfacePS, shaderFormat), "PSMain", shaderFormat);

    RT64::RenderGraphicsPipelineDesc pipeline_desc{};

    pipeline_desc.renderTargetBlend[0] = RT64::RenderBlendDesc::Copy();
    pipeline_desc.renderTargetFormat[0] = RT64::RenderFormat::B8G8R8A8_UNORM; // TODO: Use whatever format the swap chain was created with.;
    pipeline_desc.renderTargetCount = 1;
    pipeline_desc.cullMode = RT64::RenderCullMode::NONE;
    pipeline_desc.depthClipEnabled = false;
    pipeline_desc.depthEnabled = false;
    pipeline_desc.depthWriteEnabled = false;
    pipeline_desc.depthTargetFormat = RT64::RenderFormat::D32_FLOAT;
    pipeline_desc.inputSlots = nullptr;
    pipeline_desc.inputSlotsCount = 0;
    pipeline_desc.inputElements = nullptr;
    pipeline_desc.inputElementsCount = 0;
    pipeline_desc.pipelineLayout = UIContext.Interface.layout.get();
    pipeline_desc.primitiveTopology = RT64::RenderPrimitiveTopology::TRIANGLE_LIST;
    pipeline_desc.vertexShader = UIContext.Interface.vertex_shader.get();
    pipeline_desc.pixelShader = UIContext.Interface.pixel_shader.get();

    UIContext.Interface.pipeline = device->createGraphicsPipeline(pipeline_desc);


    static const uint32_t indices[] = {0, 1, 2,  1, 3, 2};
    UIContext.Interface.index_buffer = device->createBuffer(RT64::RenderBufferDesc::IndexBuffer(sizeof(indices), RT64::RenderHeapType::UPLOAD));

    {
        void* bufferData = UIContext.Interface.index_buffer->map();
        memcpy(bufferData, indices, sizeof(indices));
        UIContext.Interface.index_buffer->unmap();
    }

    UIContext.Interface.index_buffer_view = RT64::RenderIndexBufferView(UIContext.Interface.index_buffer.get(), sizeof(indices), RT64::RenderFormat::R32_UINT);
}

void draw_hook(RT64::RenderCommandList* command_list, RT64::RenderTexture* swap_chain_texture) {
    printf("RT64 hook draw\n");

    command_list->barriers(RT64::RenderTextureBarrier::Transition(swap_chain_texture, RT64::RenderTextureState::RENDER_TARGET));
    command_list->setGraphicsPipelineLayout(UIContext.Interface.layout.get());
    command_list->setPipeline(UIContext.Interface.pipeline.get());
    command_list->setIndexBuffer(&UIContext.Interface.index_buffer_view);
    command_list->drawIndexedInstanced(6, 1, 0, 0, 0);
}

void deinit_hook() {

}

void set_rt64_hooks() {
    RT64::SetRenderHooks(init_hook, draw_hook, deinit_hook);
}
