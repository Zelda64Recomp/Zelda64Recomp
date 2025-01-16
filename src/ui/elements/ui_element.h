#pragma once

#include "ui_style.h"

#include <unordered_set>

namespace recompui {

class Element : public Style, public Rml::EventListener {
    friend class Element;
private:
    std::vector<Style *> styles;
    std::vector<uint32_t> styles_counter;
    std::unordered_set<std::string_view> style_active_set;
    std::unordered_multimap<std::string_view, uint32_t> style_name_index_map;

    void add_child(Element *child);
    void register_event_listeners(uint32_t events_enabled);
    void apply_style(Style *style);
    void apply_styles();
    void propagate_disabled(bool disabled);

    // Style overrides.
    virtual void set_property(Rml::PropertyId property_id, const Rml::Property &property, Animation animation) override;

    // Rml::EventListener overrides.
    virtual void ProcessEvent(Rml::Event &event) override;
protected:
    Rml::Element *base = nullptr;
    std::vector<std::unique_ptr<Element>> children;
    uint32_t events_enabled = 0;
    bool owner = false;
    bool orphaned = false;
    bool enabled = true;
    bool disabled_attribute = false;
    bool disabled_from_parent = false;

    virtual void process_event(const Event &e);
public:
    // Used for backwards compatibility with legacy UI elements.
    Element(Rml::Element *base);

    // Used to actually construct elements.
    Element(Element *parent, uint32_t events_enabled = 0, Rml::String base_class = "div");
    virtual ~Element();
    void clear_children();
    void add_style(Style *style, const std::initializer_list<std::string_view> &style_names);
    void set_enabled(bool enabled);
    void set_text(const std::string &text);
    void set_style_enabled(const std::string_view &style_name, bool enabled);
};

} // namespace recompui