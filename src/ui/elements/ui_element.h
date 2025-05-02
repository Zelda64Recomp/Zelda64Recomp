#pragma once

#include "ui_style.h"
#include "../core/ui_context.h"

#include "recomp.h"
#include <ultramodern/ultra64.h>

#include <unordered_set>
#include <variant>

namespace recompui {
struct UICallback {
    ContextId context;
    PTR(void) callback;
    PTR(void) userdata;
};

using ElementValue = std::variant<uint32_t, float, double, std::monostate>;

class ContextId;
class Element : public Style, public Rml::EventListener {
    friend ContextId create_context(const std::filesystem::path& path);
    friend ContextId create_context();
    friend class ContextId; // To allow ContextId to call the handle_event method directly.
private:
    Rml::Element *base = nullptr;
    Rml::ElementPtr base_owning = {};
    uint32_t events_enabled = 0;
    std::vector<Style *> styles;
    std::vector<uint32_t> styles_counter;
    std::unordered_set<std::string_view> style_active_set;
    std::unordered_multimap<std::string_view, uint32_t> style_name_index_map;
    std::vector<UICallback> callbacks;
    std::vector<Element *> children;
    std::string id;
    bool shim = false;
    bool enabled = true;
    bool disabled_attribute = false;
    bool disabled_from_parent = false;
    bool can_set_text = false;

    void add_child(Element *child);
    void register_event_listeners(uint32_t events_enabled);
    void apply_style(Style *style);
    void propagate_disabled(bool disabled);
    void handle_event(const Event &e);
    void set_id(const std::string& new_id);

    // Style overrides.
    virtual void set_property(Rml::PropertyId property_id, const Rml::Property &property) override;

    // Rml::EventListener overrides.
    void ProcessEvent(Rml::Event &event) override final;
protected:
    // Use of this method in inherited classes is discouraged unless it's necessary.
    void set_attribute(const Rml::String &attribute_key, const Rml::String &attribute_value);
    virtual void process_event(const Event &e);
    virtual ElementValue get_element_value() { return std::monostate{}; }
    virtual void set_input_value(const ElementValue&) {}
    virtual std::string_view get_type_name() { return "Element"; }
public:
    // Used for backwards compatibility with legacy UI elements.
    Element(Rml::Element *base);

    // Used to actually construct elements.
    Element(Element* parent, uint32_t events_enabled = 0, Rml::String base_class = "div", bool can_set_text = false);
    virtual ~Element();
    void clear_children();
    bool remove_child(ResourceId child);
    bool remove_child(Element *child) { return remove_child(child->get_resource_id()); }
    void add_style(Style *style, std::string_view style_name);
    void add_style(Style *style, const std::initializer_list<std::string_view> &style_names);
    void set_enabled(bool enabled);
    bool is_enabled() const;
    void set_text(std::string_view text);
    std::string get_input_text();
    void set_input_text(std::string_view text);
    void set_src(std::string_view src);
    void set_style_enabled(std::string_view style_name, bool enabled);
    bool is_style_enabled(std::string_view style_name);
    void apply_styles();
    bool is_element() override { return true; }
    float get_absolute_left();
    float get_absolute_top();
    float get_client_left();
    float get_client_top();
    float get_client_width();
    float get_client_height();
    void enable_focus();
    void focus();
    void blur();
    void queue_update();
    void register_callback(ContextId context, PTR(void) callback, PTR(void) userdata);
    uint32_t get_input_value_u32();
    float get_input_value_float();
    double get_input_value_double();
    void set_input_value_u32(uint32_t val) { set_input_value(val); }
    void set_input_value_float(float val) { set_input_value(val); }
    void set_input_value_double(double val) { set_input_value(val); }
    const std::string& get_id() { return id; }
};

void queue_ui_callback(recompui::ResourceId resource, const Event& e, const UICallback& callback);

} // namespace recompui