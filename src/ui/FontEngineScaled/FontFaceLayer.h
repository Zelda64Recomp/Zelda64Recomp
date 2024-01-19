/*
 * This source file is modified from a part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUI_CORE_FONTENGINESCALED_FONTFACELAYER_H
#define RMLUI_CORE_FONTENGINESCALED_FONTFACELAYER_H

#include "RmlUi/Core.h"
#include "RmlUi/Core/FontGlyph.h"
#include "RmlUi/../../Source/Core/TextureLayout.h"

namespace Rml {
	class FontEffect;
}

namespace RecompRml {

using namespace Rml;

class FontFaceHandleScaled;

/**
    A textured layer stored as part of a font face handle. Each handle will have at least a base
    layer for the standard font. Further layers can be added to allow rendering of text effects.

    @author Peter Curry
 */

class FontFaceLayer {
public:
	FontFaceLayer(const SharedPtr<const FontEffect>& _effect);
	~FontFaceLayer();

	/// Generates or re-generates the character and texture data for the layer.
	/// @param[in] handle The handle generating this layer.
	/// @param[in] effect The effect to initialise the layer with.
	/// @param[in] clone The layer to optionally clone geometry and texture data from.
	/// @return True if the layer was generated successfully, false if not.
	bool Generate(const FontFaceHandleScaled* handle, const FontFaceLayer* clone = nullptr, bool clone_glyph_origins = false);

	/// Generates the texture data for a layer (for the texture database).
	/// @param[out] texture_data The pointer to be set to the generated texture data.
	/// @param[out] texture_dimensions The dimensions of the texture.
	/// @param[in] texture_id The index of the texture within the layer to generate.
	/// @param[in] glyphs The glyphs required by the font face handle.
	bool GenerateTexture(UniquePtr<const byte[]>& texture_data, Vector2i& texture_dimensions, int texture_id, const FontGlyphMap& glyphs);

	/// Generates the geometry required to render a single character.
	/// @param[out] geometry An array of geometries this layer will write to. It must be at least as big as the number of textures in this layer.
	/// @param[in] character_code The character to generate geometry for.
	/// @param[in] position The position of the baseline.
	/// @param[in] colour The colour of the string.
	inline void GenerateGeometry(Geometry* geometry, const Character character_code, const Vector2f position, const Colourb colour) const
	{
		auto it = character_boxes.find(character_code);
		if (it == character_boxes.end())
			return;

		const TextureBox& box = it->second;

		if (box.texture_index < 0)
			return;

		// Generate the geometry for the character.
		Vector<Vertex>& character_vertices = geometry[box.texture_index].GetVertices();
		Vector<int>& character_indices = geometry[box.texture_index].GetIndices();

		character_vertices.resize(character_vertices.size() + 4);
		character_indices.resize(character_indices.size() + 6);
		GeometryUtilities::GenerateQuad(&character_vertices[0] + (character_vertices.size() - 4),
			&character_indices[0] + (character_indices.size() - 6), Vector2f(position.x + box.origin.x, position.y + box.origin.y).Round(),
			box.dimensions, colour, box.texcoords[0], box.texcoords[1], (int)character_vertices.size() - 4);
	}

	/// Returns the effect used to generate the layer.
	const FontEffect* GetFontEffect() const;

	/// Returns one of the layer's textures.
	const Texture* GetTexture(int index);
	/// Returns the number of textures employed by this layer.
	int GetNumTextures() const;

	/// Returns the layer's colour.
	Colourb GetColour() const;

private:
	struct TextureBox {
		TextureBox() : texture_index(-1) {}

		// The offset, in pixels, of the baseline from the start of this character's geometry.
		Vector2f origin;
		// The width and height, in pixels, of this character's geometry.
		Vector2f dimensions;
		// The texture coordinates for the character's geometry.
		Vector2f texcoords[2];

		// The texture this character renders from.
		int texture_index;
	};

	using CharacterMap = UnorderedMap<Character, TextureBox>;
	using TextureList = Vector<Texture>;

	SharedPtr<const FontEffect> effect;

	TextureLayout texture_layout;

	CharacterMap character_boxes;
	TextureList textures;
	Colourb colour;
};

} // namespace Rml
#endif
