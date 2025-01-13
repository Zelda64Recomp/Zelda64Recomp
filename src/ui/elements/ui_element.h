#pragma once

#include "ui_style.h"

namespace recompui {

class Element : public Style, public Rml::EventListener {
    friend class Element;
private:
    std::vector<Style *> styles;
    std::vector<bool> styles_active;
    std::map<std::string, uint32_t> style_name_index_map;

    void add_child(Element *child);
    void register_event_listeners(uint32_t events_enabled);
    void apply_style(Style *style);
    void apply_styles();

    // Style overrides.
    virtual void set_property(Rml::PropertyId property_id, const Rml::Property &property, Animation animation) override;

    // Rml::EventListener overrides.
    virtual void ProcessEvent(Rml::Event &event) override;
protected:
    Rml::Element *base = nullptr;
    std::vector<std::unique_ptr<Element>> children;
    bool owner = false;
    bool orphaned = false;

    virtual void process_event(const Event &e);
public:
    // Used for backwards compatibility with legacy UI elements.
    Element(Rml::Element *base);

    // Used to actually construct elements.
    Element(Element *parent, uint32_t events_enabled = 0, Rml::String base_class = "div");
    virtual ~Element();
    void clear_children();
    void add_style(const std::string &style_name, Style *style);
    void enable_style(const std::string &style_name, bool enable);
    void set_text(const std::string &text);
};

} // namespace recompui