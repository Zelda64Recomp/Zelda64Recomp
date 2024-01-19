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

#ifndef RMLUI_CORE_FONTENGINESCALED_FONTFAMILY_H
#define RMLUI_CORE_FONTENGINESCALED_FONTFAMILY_H

#include "FontTypes.h"

namespace RecompRml {

using namespace Rml;

class FontFace;
class FontFaceHandleScaled;

/**
    @author Peter Curry
 */

class FontFamily {
public:
	FontFamily(const String& name);
	~FontFamily();

	/// Returns a handle to the most appropriate font in the family, at the correct size.
	/// @param[in] style The style of the desired handle.
	/// @param[in] weight The weight of the desired handle.
	/// @param[in] size The size of desired handle, in points.
	/// @return A valid handle if a matching (or closely matching) font face was found, nullptr otherwise.
	FontFaceHandleScaled* GetFaceHandle(Style::FontStyle style, Style::FontWeight weight, int size);

	/// Adds a new face to the family.
	/// @param[in] ft_face The previously loaded FreeType face.
	/// @param[in] style The style of the new face.
	/// @param[in] weight The weight of the new face.
	/// @param[in] face_memory Optionally pass ownership of the face's memory to the face itself, automatically releasing it on destruction.
	/// @return True if the face was loaded successfully, false otherwise.
	FontFace* AddFace(FontFaceHandleFreetype ft_face, Style::FontStyle style, Style::FontWeight weight, UniquePtr<byte[]> face_memory);

	/// Releases resources owned by sized font faces, including their textures and rendered glyphs.
	void ReleaseFontResources();

protected:
	String name;

	struct FontFaceEntry {
		UniquePtr<FontFace> face;
		// Only filled if we own the memory used by the face's FreeType handle. May be shared with other faces in this family.
		UniquePtr<byte[]> face_memory;
	};

	using FontFaceList = Vector<FontFaceEntry>;
	FontFaceList font_faces;
};

} // namespace Rml
#endif
