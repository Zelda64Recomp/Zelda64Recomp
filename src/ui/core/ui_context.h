#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <filesystem>
#include <functional>

#include "RmlUi/Core.h"

#include "ui_resource.h"

namespace recompui {
    class Style;
    class Element;
    class ContextId {
        Style* add_resource_impl(std::unique_ptr<Style>&& resource);
    public:
        uint32_t slot_id;
        auto operator<=>(const ContextId& rhs) const = default;

        template <typename T, typename... Args>
        T* create_element(Args... args) {
            return static_cast<T*>(add_resource_impl(std::make_unique<T>(std::forward<Args>(args)...)));
        }
        
        template <typename T>
        T* create_element(T&& element) {
            return static_cast<T*>(add_resource_impl(std::make_unique<T>(std::move(element))));
        }

        void add_loose_element(Element* element);
        void queue_element_update(ResourceId element);
        void queue_set_text(Element* element, std::string&& text);

        Style* create_style();

        void destroy_resource(Style* resource);
        void destroy_resource(ResourceId resource);
        void clear_children();

        Rml::ElementDocument* get_document();
        Element* get_root_element();
        Element* get_autofocus_element();
        void set_autofocus_element(Element* element);

        void open();
        bool open_if_not_already();
        void close();
        void process_updates();

        static constexpr ContextId null() { return ContextId{ .slot_id = uint32_t(-1) }; }

        bool captures_input();
        bool captures_mouse();

        void set_captures_input(bool captures_input);
        void set_captures_mouse(bool captures_input);
    };

    ContextId create_context(const std::filesystem::path& path);
    ContextId create_context(Rml::ElementDocument* document);
    ContextId create_context();
    void destroy_context(ContextId id);
    ContextId get_current_context();
    ContextId get_context_from_document(Rml::ElementDocument* document);
    void destroy_all_contexts();

    void register_ui_exports();
} // namespace recompui
