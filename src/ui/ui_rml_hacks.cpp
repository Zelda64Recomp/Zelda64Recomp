#include "recomp_ui.h"
#include "RmlUi/Core.h"
#include "ui_rml_hacks.hpp"

//! these are hidden methods not exposed by RmlUi
//! they may need to be updated eventually with RmlUi

RecompRml::CanFocus RecompRml::CanFocusElement(Rml::Element* element)
{
	if (!element->IsVisible())
		return RecompRml::CanFocus::NoAndNoChildren;

	const Rml::ComputedValues& computed = element->GetComputedValues();

	if (computed.focus() == Rml::Style::Focus::None)
		return RecompRml::CanFocus::NoAndNoChildren;

	if (computed.tab_index() == Rml::Style::TabIndex::Auto)
		return RecompRml::CanFocus::Yes;

	return RecompRml::CanFocus::No;
}

Rml::Element* SearchFocusSubtree(Rml::Element* element, bool forward)
{
	auto can_focus = RecompRml::CanFocusElement(element);
	if (can_focus == RecompRml::CanFocus::Yes)
		return element;
	else if (can_focus == RecompRml::CanFocus::NoAndNoChildren)
		return nullptr;

	for (int i = 0; i < element->GetNumChildren(); i++)
	{
		int child_index = i;
		if (!forward)
			child_index = element->GetNumChildren() - i - 1;
		if (Rml::Element* result = SearchFocusSubtree(element->GetChild(child_index), forward))
			return result;
	}

	return nullptr;
}

Rml::Element* RecompRml::FindNextTabElement(Rml::Element* current_element, bool forward)
{
	// This algorithm is quite sneaky, I originally thought a depth first search would work, but it appears not. What is
	// required is to cut the tree in half along the nodes from current_element up the root and then either traverse the
	// tree in a clockwise or anticlock wise direction depending if you're searching forward or backward respectively.

	// If we're searching forward, check the immediate children of this node first off.
	if (forward)
	{
		for (int i = 0; i < current_element->GetNumChildren(); i++)
			if (Rml::Element* result = SearchFocusSubtree(current_element->GetChild(i), forward))
				return result;
	}

	// Now walk up the tree, testing either the bottom or top
	// of the tree, depending on whether we're going forward
	// or backward respectively.
	bool search_enabled = false;
	Rml::Element* document = current_element->GetOwnerDocument();
	Rml::Element* child = current_element;
	Rml::Element* parent = current_element->GetParentNode();
	while (child != document)
	{
		const int num_children = parent->GetNumChildren();
		for (int i = 0; i < num_children; i++)
		{
			// Calculate index into children
			const int child_index = forward ? i : (num_children - i - 1);
			Rml::Element* search_child = parent->GetChild(child_index);

			// Do a search if its enabled
			if (search_enabled)
				if (Rml::Element* result = SearchFocusSubtree(search_child, forward))
					return result;

			// Enable searching when we reach the child.
			if (search_child == child)
				search_enabled = true;
		}

		// Advance up the tree
		child = parent;
		parent = parent->GetParentNode();
		search_enabled = false;
	}

	// We could not find anything to focus along this direction.

	// If we can focus the document, then focus that now.
	if (current_element != document && RecompRml::CanFocusElement(document) == RecompRml::CanFocus::Yes)
		return document;

	// Otherwise, search the entire document tree. This way we will wrap around.
	const int num_children = document->GetNumChildren();
	for (int i = 0; i < num_children; i++)
	{
		const int child_index = forward ? i : (num_children - i - 1);
		if (Rml::Element* result = SearchFocusSubtree(document->GetChild(child_index), forward))
			return result;
	}

	return nullptr;
}
