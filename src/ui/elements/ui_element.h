#pragma once

#include "ui_style.h"
#include "../core/ui_context.h"

#include <unordered_set>

namespace recompui {
class ContextId;
class Element : public Style, public Rml::EventListener {
    friend ContextId create_context(const std::filesystem::path& path);
    friend ContextId create_context();
    friend class ContextId; // To allow ContextId to call the process_event method directly.
private:
    Rml::Element *base = nullptr;
    Rml::ElementPtr base_owning = {};
    uint32_t events_enabled = 0;
    std::vector<Style *> styles;
    std::vector<uint32_t> styles_counter;
    std::unordered_set<std::string_view> style_active_set;
    std::unordered_multimap<std::string_view, uint32_t> style_name_index_map;
    std::vector<Element *> children;
    bool shim = false;
    bool enabled = true;
    bool disabled_attribute = false;
    bool disabled_from_parent = false;

    void add_child(Element *child);
    void register_event_listeners(uint32_t events_enabled);
    void apply_style(Style *style);
    void apply_styles();
    void propagate_disabled(bool disabled);

    // Style overrides.
    virtual void set_property(Rml::PropertyId property_id, const Rml::Property &property) override;

    // Rml::EventListener overrides.
    void ProcessEvent(Rml::Event &event) override final;
protected:
    // Use of this method in inherited classes is discouraged unless it's necessary.
    void set_attribute(const Rml::String &attribute_key, const Rml::String &attribute_value);
    virtual void process_event(const Event &e);
public:
    // Used for backwards compatibility with legacy UI elements.
    Element(Rml::Element *base);

    // Used to actually construct elements.
    Element(Element* parent, uint32_t events_enabled = 0, Rml::String base_class = "div");
    virtual ~Element();
    void clear_children();
    void add_style(Style *style, std::string_view style_name);
    void add_style(Style *style, const std::initializer_list<std::string_view> &style_names);
    void set_enabled(bool enabled);
    bool is_enabled() const;
    void set_text(std::string_view text);
    void set_src(std::string_view src);
    void set_style_enabled(std::string_view style_name, bool enabled);
    bool is_element() override { return true; }
    float get_absolute_left();
    float get_absolute_top();
    float get_client_left();
    float get_client_top();
    float get_client_width();
    float get_client_height();
    void queue_update();
};

} // namespace recompui