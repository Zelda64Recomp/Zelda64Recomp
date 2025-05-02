#include <mutex>
#include <string>
#include <unordered_map>
#include <fstream>

#include "slot_map.h"
#include "RmlUi/Core/StreamMemory.h"

#include "ultramodern/error_handling.hpp"
#include "recomp_ui.h"
#include "ui_context.h"
#include "../elements/ui_element.h"

// Hash implementations for ContextId and ResourceId.
template <>
struct std::hash<recompui::ContextId> {
    std::size_t operator()(const recompui::ContextId& id) const {
        return std::hash<uint32_t>()(id.slot_id);
    }
};

template <>
struct std::hash<recompui::ResourceId> {
    std::size_t operator()(const recompui::ResourceId& id) const {
        return std::hash<uint32_t>()(id.slot_id);
    }
};

using resource_slotmap = dod::slot_map32<std::unique_ptr<recompui::Style>>;

namespace recompui {
    struct Context {
        std::mutex context_lock;
        resource_slotmap resources;
        Rml::ElementDocument* document;
        Element root_element;
        Element* autofocus_element = nullptr;
        std::vector<Element*> loose_elements;
        std::unordered_set<ResourceId> to_update;
        std::vector<std::tuple<Element*, ResourceId, std::string>> to_set_text;     
        bool captures_input = true;
        bool captures_mouse = true;
        Context(Rml::ElementDocument* document) : document(document), root_element(document) {}
    };
} // namespace recompui

using context_slotmap = dod::slot_map32<recompui::Context>;

static struct {
    std::recursive_mutex all_contexts_lock;
    context_slotmap all_contexts;
    std::unordered_set<recompui::ContextId> opened_contexts;
    std::unordered_map<Rml::ElementDocument*, recompui::ContextId> documents_to_contexts;
    Rml::SharedPtr<Rml::StyleSheetContainer> style_sheet;
} context_state;

thread_local recompui::Context* opened_context = nullptr;
thread_local recompui::ContextId opened_context_id = recompui::ContextId::null();

enum class ContextErrorType {
    OpenWithoutClose,
    OpenInvalidContext,
    CloseWithoutOpen,
    CloseWrongContext,
    DestroyInvalidContext,
    GetContextWithoutOpen,
    AddResourceWithoutOpen,
    AddResourceToWrongContext,
    UpdateElementWithoutContext,
    UpdateElementInWrongContext,
    SetTextElementWithoutContext,
    SetTextElementInWrongContext,
    GetResourceWithoutOpen,
    GetResourceFailed,
    DestroyResourceWithoutOpen,
    DestroyResourceInWrongContext,
    DestroyResourceNotFound,
    GetDocumentInvalidContext,
    GetAutofocusInvalidContext,
    SetAutofocusInvalidContext,
    InternalError,
};

enum class SlotTag : uint8_t {
    Style = 0,
    Element = 1,
};

void context_error(recompui::ContextId id, ContextErrorType type) {
    (void)id;

    const char* error_message = "";

    switch (type) {
        case ContextErrorType::OpenWithoutClose:
            error_message = "Attempted to open a UI context without closing another UI context";
            break;
        case ContextErrorType::OpenInvalidContext:
            error_message = "Attempted to open an invalid UI context";
            break;
        case ContextErrorType::CloseWithoutOpen:
            error_message = "Attempted to close a UI context without one being open";
            break;
        case ContextErrorType::CloseWrongContext:
            error_message = "Attempted to close a different UI context than the one that's open";
            break;
        case ContextErrorType::DestroyInvalidContext:
            error_message = "Attempted to destroy an invalid UI element";
            break;
        case ContextErrorType::GetContextWithoutOpen:
            error_message = "Attempted to get the current UI context with no UI context open";
            break;
        case ContextErrorType::AddResourceWithoutOpen:
            error_message = "Attempted to create a UI resource with no open UI context";
            break;
        case ContextErrorType::AddResourceToWrongContext:
            error_message = "Attempted to create a UI resource in a different UI context than the one that's open";
            break;
        case ContextErrorType::UpdateElementWithoutContext:
            error_message = "Attempted to update a UI element with no open UI context";
            break;
        case ContextErrorType::UpdateElementInWrongContext:
            error_message = "Attempted to update a UI element in a different UI context than the one that's open";
            break;
        case ContextErrorType::SetTextElementWithoutContext:
            error_message = "Attempted to set the text of a UI element with no open UI context";
            break;
        case ContextErrorType::SetTextElementInWrongContext:
            error_message = "Attempted to set the text of a UI element in a different UI context than the one that's open";
            break;
        case ContextErrorType::GetResourceWithoutOpen:
            error_message = "Attempted to get a UI resource with no open UI context";
            break;
        case ContextErrorType::GetResourceFailed:
            error_message = "Failed to get a UI resource from the current open UI context";
            break;
        case ContextErrorType::DestroyResourceWithoutOpen:
            error_message = "Attempted to destroy a UI resource with no open UI context";
            break;
        case ContextErrorType::DestroyResourceInWrongContext:
            error_message = "Attempted to destroy a UI resource in a different UI context than the one that's open";
            break;
        case ContextErrorType::DestroyResourceNotFound:
            error_message = "Attempted to destroy a UI resource that doesn't exist in the current context";
            break;
        case ContextErrorType::GetDocumentInvalidContext:
            error_message = "Attempted to get the document of an invalid UI context";
            break;
        case ContextErrorType::GetAutofocusInvalidContext:
            error_message = "Attempted to get the autofocus element of an invalid UI context";
            break;
        case ContextErrorType::SetAutofocusInvalidContext:
            error_message = "Attempted to set the autofocus element of an invalid UI context";
            break;
        case ContextErrorType::InternalError:
            error_message = "Internal error in UI context";
            break;
        default:
            error_message = "Unknown UI context error";
            break;
    }

    // This assumes the error is coming from a mod, as it's unlikely that an end user will see a UI context error
    // in the base recomp.
    recompui::message_box((std::string{"Fatal error in mod - "} + error_message + ".").c_str());
    assert(false);
    ultramodern::error_handling::quick_exit(__FILE__, __LINE__, __FUNCTION__);
}

recompui::ContextId create_context_impl(Rml::ElementDocument* document) {
    static Rml::ElementDocument dummy_document{""};
    bool add_to_dict = true;

    if (document == nullptr) {
        document = &dummy_document;
        add_to_dict = false;
    }

    recompui::ContextId ret;
    {
        std::lock_guard lock{ context_state.all_contexts_lock };
        ret = { context_state.all_contexts.emplace(document).raw };

        if (add_to_dict) {
            context_state.documents_to_contexts.emplace(document, ret);
        }
    }

    return ret;
}

void recompui::init_styling(const std::filesystem::path& rcss_file) {
    std::string style{};
    {
        std::ifstream style_stream{rcss_file};
        style_stream.seekg(0, std::ios::end);
        style.resize(style_stream.tellg());
        style_stream.seekg(0, std::ios::beg);

        style_stream.read(style.data(), style.size());
    }
    std::unique_ptr<Rml::StreamMemory> rml_stream = std::make_unique<Rml::StreamMemory>(reinterpret_cast<Rml::byte*>(style.data()), style.size());
    rml_stream->SetSourceURL(rcss_file.filename().string());
    context_state.style_sheet = Rml::Factory::InstanceStyleSheetStream(rml_stream.get());
}

recompui::ContextId recompui::create_context(const std::filesystem::path& path) {
    ContextId new_context = create_context_impl(nullptr);

    auto workingdir = std::filesystem::current_path();

    new_context.open();
    Rml::ElementDocument* doc = recompui::load_document(path.string());
    opened_context->document = doc;
    opened_context->root_element.base = doc;
    new_context.close();
    
    {
        std::lock_guard lock{ context_state.all_contexts_lock };
        context_state.documents_to_contexts.emplace(doc, new_context);
    }

    return new_context;
}

recompui::ContextId recompui::create_context(Rml::ElementDocument* document) {
    assert(document != nullptr);

    return create_context_impl(document);
}

recompui::ContextId recompui::create_context() {
    Rml::ElementDocument* doc = create_empty_document();
    doc->SetStyleSheetContainer(context_state.style_sheet);
    ContextId ret = create_context_impl(doc);
    Element* root = ret.get_root_element();
    // Mark the root element as not being a shim, as that's only needed for elements that were parented to Rml ones manually.
    root->shim = false;

    ret.open();
    root->set_width(100.0f, Unit::Percent);
    root->set_height(100.0f, Unit::Percent);
    root->set_display(Display::Flex);
    ret.close();

    doc->Hide();

    return ret;
}

void recompui::destroy_context(ContextId id) {
    bool existed = false;

    // TODO prevent deletion of a context while its mutex is in use. Second lock on the context's mutex before popping
    // from the slotmap?
    
    // Check if the provided id exists.
    {
        std::lock_guard lock{ context_state.all_contexts_lock };
        // Check if the target context is currently open.
        existed = context_state.all_contexts.has_key(context_slotmap::key{ id.slot_id });
    }

    
    // Raise an error if the context didn't exist.
    if (!existed) {
        context_error(id, ContextErrorType::DestroyInvalidContext);
    }

    id.open();
    id.clear_children();
    id.close();

    // Delete the provided id.
    {
        std::lock_guard lock{ context_state.all_contexts_lock };
        context_state.all_contexts.erase(context_slotmap::key{ id.slot_id });
    }
}

void recompui::destroy_all_contexts() {
    recompui::hide_all_contexts();

    std::lock_guard lock{ context_state.all_contexts_lock };

    // TODO prevent deletion of a context while its mutex is in use. Second lock on the context's mutex before popping
    // from the slotmap

    std::vector<context_slotmap::key> keys{};
    for (const auto& [key, item] : context_state.all_contexts.items()) {
        keys.push_back(key);
    }

    for (auto key : keys) {
        Context* ctx = context_state.all_contexts.get(key);
        
        std::lock_guard context_lock{ ctx->context_lock };
        opened_context = ctx;
        opened_context_id = ContextId{ key };

        opened_context_id.clear_children();

        opened_context = nullptr;
        opened_context_id = ContextId::null();
    }

    context_state.all_contexts.reset();
    context_state.documents_to_contexts.clear();
}

void recompui::ContextId::open() {
    // Ensure no other context is opened by this thread already.
    if (opened_context_id != ContextId::null()) {
        context_error(*this, ContextErrorType::OpenWithoutClose);
    }

    // Get the context with this id.
    Context* ctx;
    {
        std::lock_guard lock{ context_state.all_contexts_lock };
        ctx = context_state.all_contexts.get(context_slotmap::key{ slot_id });
        // If the context was found, add it to the opened contexts.
        if (ctx != nullptr) {
            context_state.opened_contexts.emplace(*this);
        }
    }

    // Check if the context exists.
    if (ctx == nullptr) {
        context_error(*this, ContextErrorType::OpenInvalidContext);
    }

    // Take ownership of the target context.
    ctx->context_lock.lock();
    opened_context = ctx;
    opened_context_id = *this;
}

bool recompui::ContextId::open_if_not_already() {
    if (opened_context_id == *this) {
        return false;
    }

    open();
    return true;
}

void recompui::ContextId::close() {
    // Ensure a context is currently opened by this thread.
    if (opened_context_id == ContextId::null()) {
        context_error(*this, ContextErrorType::CloseWithoutOpen);
    }

    // Check that the context that was specified is the same one that's currently open.
    if (*this != opened_context_id) {
        context_error(*this, ContextErrorType::CloseWrongContext);
    }

    // Release ownership of the target context.
    opened_context->context_lock.unlock();
    opened_context = nullptr;
    opened_context_id = ContextId::null();

    // Remove this context from the opened contexts.
    {
        std::lock_guard lock{ context_state.all_contexts_lock };
        context_state.opened_contexts.erase(*this);
    }
}

recompui::ContextId recompui::try_close_current_context() {
    if (opened_context_id != ContextId::null()) {
        ContextId prev_context = opened_context_id;
        opened_context_id.close();
        return prev_context;
    }
    return ContextId::null();
}

void recompui::ContextId::process_updates() {
    // Ensure a context is currently opened by this thread.
    if (opened_context_id == ContextId::null()) {
        context_error(*this, ContextErrorType::InternalError);
    }

    // Check that the context that was specified is the same one that's currently open.
    if (*this != opened_context_id) {
        context_error(*this, ContextErrorType::InternalError);
    }

    // Move the current update set into a local variable. This clears the update set
    // and allows it to be used to queue updates from any element callbacks.
    std::unordered_set<ResourceId> to_update = std::move(opened_context->to_update);

    Event update_event = Event::update_event();

    for (auto cur_resource_id : to_update) {
        resource_slotmap::key cur_key{ cur_resource_id.slot_id };

        // Ignore any resources that aren't elements.
        if (cur_key.get_tag() != static_cast<uint8_t>(SlotTag::Element)) {
            // Assert to catch errors of queueing other resource types for update.
            // This isn't an actual error, so there's no issue with continuing in release builds.
            assert(false);
            continue;
        }

        // Get the resource being updaten from the context.
        std::unique_ptr<Style>* cur_resource = opened_context->resources.get(cur_key);

        // Make sure the resource exists before dispatching the event. It may have been deleted
        // after being queued for a update, so just continue to the next element if it doesn't exist.
        if (cur_resource == nullptr) {
            continue;
        }

        static_cast<Element*>(cur_resource->get())->handle_event(update_event);
    }

    std::vector<std::tuple<Element*, ResourceId, std::string>> to_set_text = std::move(opened_context->to_set_text);

    // Delete the Rml elements that are pending deletion.
    for (auto cur_text_update : to_set_text) {
        Element* element_ptr = std::get<0>(cur_text_update);
        ResourceId resource = std::get<1>(cur_text_update);
        std::string& text = std::get<2>(cur_text_update);

        // If the resource ID is valid, prefer that as we can quickly validate if the resource still exists.
        if (resource != ResourceId::null()) {
            resource_slotmap::key cur_key{ resource.slot_id };
            std::unique_ptr<Style>* cur_resource = opened_context->resources.get(cur_key);

            // Make sure the resource exists before setting its text, as it may have been deleted.
            if (cur_resource == nullptr) {
                continue;
            }

            // Perform the text update.
            static_cast<Element*>(cur_resource->get())->base->SetInnerRML(text);
        }
        // Otherwise we use the element pointer, but we need to validate that it still exists before doing so.
        else {
            // Scan the current resources to find the target element.
            for (const std::unique_ptr<Style>& cur_e : opened_context->resources) {
                if (cur_e.get() == element_ptr) {
                    element_ptr->base->SetInnerRML(text);
                    // We can stop after finding the element.
                    break;
                }
            }
        }
    }
}

bool recompui::ContextId::captures_input() {
    std::lock_guard lock{ context_state.all_contexts_lock };

    Context* ctx = context_state.all_contexts.get(context_slotmap::key{ slot_id });
    if (ctx == nullptr) {
        return false;
    }
    return ctx->captures_input;

}

bool recompui::ContextId::captures_mouse() {
    std::lock_guard lock{ context_state.all_contexts_lock };

    Context* ctx = context_state.all_contexts.get(context_slotmap::key{ slot_id });
    if (ctx == nullptr) {
        return false;
    }
    return ctx->captures_mouse;
}

void recompui::ContextId::set_captures_input(bool captures_input) {
    std::lock_guard lock{ context_state.all_contexts_lock };

    Context* ctx = context_state.all_contexts.get(context_slotmap::key{ slot_id });
    if (ctx == nullptr) {
        return;
    }
    ctx->captures_input = captures_input;
}

void recompui::ContextId::set_captures_mouse(bool captures_mouse) {
    std::lock_guard lock{ context_state.all_contexts_lock };

    Context* ctx = context_state.all_contexts.get(context_slotmap::key{ slot_id });
    if (ctx == nullptr) {
        return;
    }
    ctx->captures_mouse = captures_mouse;
}

recompui::Style* recompui::ContextId::add_resource_impl(std::unique_ptr<Style>&& resource) {
    // Ensure a context is currently opened by this thread.
    if (opened_context_id == ContextId::null()) {
        context_error(*this, ContextErrorType::AddResourceWithoutOpen);
    }

    // Check that the context that was specified is the same one that's currently open.
    if (*this != opened_context_id) {
        context_error(*this, ContextErrorType::AddResourceToWrongContext);
    }

    bool is_element = resource->is_element();
    Style* resource_ptr = resource.get();
    auto key = opened_context->resources.emplace(std::move(resource));

    if (is_element) {
        Element* element_ptr = static_cast<Element*>(resource_ptr);
        element_ptr->set_id(std::string{element_ptr->get_type_name()} + "-" + std::to_string(key.raw));
        key.set_tag(static_cast<uint8_t>(SlotTag::Element));
        // Send one update to the element.
        opened_context->to_update.emplace(ResourceId{ key.raw });
    }
    else {
        key.set_tag(static_cast<uint8_t>(SlotTag::Style));
    }

    resource_ptr->resource_id = { key.raw };
    return resource_ptr;
}

void recompui::ContextId::add_loose_element(Element* element) {
    // Ensure a context is currently opened by this thread.
    if (opened_context_id == ContextId::null()) {
        context_error(*this, ContextErrorType::AddResourceWithoutOpen);
    }

    // Check that the context that was specified is the same one that's currently open.
    if (*this != opened_context_id) {
        context_error(*this, ContextErrorType::AddResourceToWrongContext);
    }

    opened_context->loose_elements.emplace_back(element);
}

void recompui::ContextId::queue_element_update(ResourceId element) {
    // Ensure a context is currently opened by this thread.
    if (opened_context_id == ContextId::null()) {
        context_error(*this, ContextErrorType::UpdateElementWithoutContext);
    }

    // Check that the context that was specified is the same one that's currently open.
    if (*this != opened_context_id) {
        context_error(*this, ContextErrorType::UpdateElementInWrongContext);
    }

    // Check that the element that was specified is in the open context.
    auto* elementPtr = opened_context->resources.get(resource_slotmap::key{ element.slot_id });
    if (elementPtr == nullptr) {
        context_error(*this, ContextErrorType::UpdateElementInWrongContext);
    }

    opened_context->to_update.emplace(element);
}

void recompui::ContextId::queue_set_text(Element* element, std::string&& text) {
    // Ensure a context is currently opened by this thread.
    if (opened_context_id == ContextId::null()) {
        context_error(*this, ContextErrorType::SetTextElementWithoutContext);
    }

    // Check that the context that was specified is the same one that's currently open.
    if (*this != opened_context_id) {
        context_error(*this, ContextErrorType::SetTextElementInWrongContext);
    }

    opened_context->to_set_text.emplace_back(std::make_tuple(element, element->resource_id, std::move(text)));
}

recompui::Style* recompui::ContextId::create_style() {
    return add_resource_impl(std::make_unique<Style>());
}

void recompui::ContextId::destroy_resource(Style* resource) {
    destroy_resource(resource->resource_id);
}

void recompui::ContextId::destroy_resource(ResourceId resource) {
    // Ensure a context is currently opened by this thread.
    if (opened_context_id == ContextId::null()) {
        context_error(*this, ContextErrorType::DestroyResourceWithoutOpen);
    }

    // Check that the context that was specified is the same one that's currently open.
    if (*this != opened_context_id) {
        context_error(*this, ContextErrorType::DestroyResourceInWrongContext);
    }

    // Try to remove the resource from the current context.
    auto pop_result = opened_context->resources.pop(resource_slotmap::key{ resource.slot_id });
    if (!pop_result.has_value()) {
        context_error(*this, ContextErrorType::DestroyResourceNotFound);
    }
}

void recompui::ContextId::clear_children() {
    // Ensure a context is currently opened by this thread.
    if (opened_context_id == ContextId::null()) {
        context_error(*this, ContextErrorType::DestroyResourceWithoutOpen);
    }

    // Check that the context that was specified is the same one that's currently open.
    if (*this != opened_context_id) {
        context_error(*this, ContextErrorType::DestroyResourceInWrongContext);
    }

    // Remove the root element's children.
    opened_context->root_element.clear_children();

    // Remove any loose resources.
    for (Element* e : opened_context->loose_elements) {
        destroy_resource(e->resource_id);
    }
    opened_context->loose_elements.clear();
}

Rml::ElementDocument* recompui::ContextId::get_document() {
    std::lock_guard lock{ context_state.all_contexts_lock };

    Context* ctx = context_state.all_contexts.get(context_slotmap::key{ slot_id });
    if (ctx == nullptr) {
        context_error(*this, ContextErrorType::GetDocumentInvalidContext);
    }

    return ctx->document;
}

recompui::Element* recompui::ContextId::get_root_element() {
    std::lock_guard lock{ context_state.all_contexts_lock };

    Context* ctx = context_state.all_contexts.get(context_slotmap::key{ slot_id });
    if (ctx == nullptr) {
        context_error(*this, ContextErrorType::GetDocumentInvalidContext);
    }

    return &ctx->root_element;
}

recompui::Element* recompui::ContextId::get_autofocus_element() {
    std::lock_guard lock{ context_state.all_contexts_lock };

    Context* ctx = context_state.all_contexts.get(context_slotmap::key{ slot_id });
    if (ctx == nullptr) {
        context_error(*this, ContextErrorType::GetAutofocusInvalidContext);
    }
    
    return ctx->autofocus_element;
}

void recompui::ContextId::set_autofocus_element(Element* element) {
    std::lock_guard lock{ context_state.all_contexts_lock };

    Context* ctx = context_state.all_contexts.get(context_slotmap::key{ slot_id });
    if (ctx == nullptr) {
        context_error(*this, ContextErrorType::SetAutofocusInvalidContext);
    }

    ctx->autofocus_element = element;
}

recompui::ContextId recompui::get_current_context() {
    // Ensure a context is currently opened by this thread.
    if (opened_context_id == ContextId::null()) {
        context_error(ContextId::null(), ContextErrorType::GetContextWithoutOpen);
    }

    return opened_context_id;
}

recompui::Style* get_resource_from_current_context(resource_slotmap::key key) {
    // Ensure a context is currently opened by this thread.
    if (opened_context_id == recompui::ContextId::null()) {
        context_error(recompui::ContextId::null(), ContextErrorType::GetResourceWithoutOpen);
    }

    auto* value = opened_context->resources.get(key);
    if (value == nullptr) {
        context_error(opened_context_id, ContextErrorType::GetResourceFailed);
    }

    return value->get();
}

const recompui::Style* recompui::ResourceId::operator*() const {
    resource_slotmap::key key{ slot_id };

    return get_resource_from_current_context(key);
}

recompui::Style* recompui::ResourceId::operator*() {
    resource_slotmap::key key{ slot_id };

    return get_resource_from_current_context(key);
}

const recompui::Element* recompui::ResourceId::as_element() const {
    resource_slotmap::key key{ slot_id };
    uint8_t tag = key.get_tag();

    assert(tag == static_cast<uint8_t>(SlotTag::Element));

    return static_cast<Element*>(get_resource_from_current_context(key));
}

recompui::Element* recompui::ResourceId::as_element() {
    resource_slotmap::key key{ slot_id };
    uint8_t tag = key.get_tag();

    assert(tag == static_cast<uint8_t>(SlotTag::Element));

    return static_cast<Element*>(get_resource_from_current_context(key));
}

recompui::ContextId recompui::get_context_from_document(Rml::ElementDocument* document) {
    std::lock_guard lock{ context_state.all_contexts_lock };
    auto find_it = context_state.documents_to_contexts.find(document);
    if (find_it == context_state.documents_to_contexts.end()) {
        return ContextId::null();
    }
    return find_it->second;
}
