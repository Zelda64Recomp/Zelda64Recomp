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

#ifndef RMLUI_CORE_FONTENGINESCALED_FREETYPEINTERFACE_H
#define RMLUI_CORE_FONTENGINESCALED_FREETYPEINTERFACE_H

#include "RmlUi/Core/FontMetrics.h"
#include "FontTypes.h"

namespace RecompRml {

using namespace Rml;

namespace FreeType {

	// Initialize FreeType library.
	bool Initialise();
	// Shutdown FreeType library.
	void Shutdown();

	// Returns a sorted list of available font variations for the font face located in memory.
	bool GetFaceVariations(const byte* data, int data_length, Vector<FaceVariation>& out_face_variations);

	// Loads a FreeType face from memory, 'source' is only used for logging.
	FontFaceHandleFreetype LoadFace(const byte* data, int data_length, const String& source, int named_instance_index = 0);

	// Releases the FreeType face.
	bool ReleaseFace(FontFaceHandleFreetype face);

	// Retrieves the font family, style and weight of the given font face. Use nullptr to ignore a property.
	void GetFaceStyle(FontFaceHandleFreetype face, String* font_family, Style::FontStyle* style, Style::FontWeight* weight);

	// Initializes a face for a given font size. Glyphs are filled with the ASCII subset, and the font face metrics are set.
	bool InitialiseFaceHandle(FontFaceHandleFreetype face, int font_size, FontGlyphMap& glyphs, FontMetrics& metrics, bool load_default_glyphs);

	// Build a new glyph representing the given code point and append to 'glyphs'.
	bool AppendGlyph(FontFaceHandleFreetype face, int font_size, Character character, FontGlyphMap& glyphs);

	// Returns the kerning between two characters.
	// 'font_size' value of zero assumes the font size is already set on the face, and skips this step for performance reasons.
	int GetKerning(FontFaceHandleFreetype face, int font_size, Character lhs, Character rhs);

	// Returns true if the font face has kerning.
	bool HasKerning(FontFaceHandleFreetype face);

} // namespace FreeType
} // namespace Rml
#endif
