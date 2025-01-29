#ifndef __UI_RENDERER_H__
#define __UI_RENDERER_H__

#include <memory>

namespace RT64 {
    struct RenderInterface;
    struct RenderDevice;
    struct RenderCommandList;
    struct RenderFramebuffer;
};

namespace Rml {
    class RenderInterface;
}

namespace recompui {
    class RmlRenderInterface_RT64_impl;

    class RmlRenderInterface_RT64 {
    private:
        std::unique_ptr<RmlRenderInterface_RT64_impl> impl;
    public:
        RmlRenderInterface_RT64();
        ~RmlRenderInterface_RT64();
        void reset();
        void init(RT64::RenderInterface* interface, RT64::RenderDevice* device);
        Rml::RenderInterface* get_rml_interface();
        
        void start(RT64::RenderCommandList* list, int image_width, int image_height);
        void end(RT64::RenderCommandList* list, RT64::RenderFramebuffer* framebuffer);
        void queue_image_from_bytes(const std::string &src, const std::vector<char> &bytes);
    };
} // namespace recompui

#endif
