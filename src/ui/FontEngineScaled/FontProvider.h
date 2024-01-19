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

#ifndef RMLUI_CORE_FONTENGINESCALED_FONTPROVIDER_H
#define RMLUI_CORE_FONTENGINESCALED_FONTPROVIDER_H

#include "RmlUi/Core/StyleTypes.h"
#include "RmlUi/Core/Types.h"
#include "FontTypes.h"

namespace RecompRml {

using namespace Rml;

class FontFace;
class FontFamily;
class FontFaceHandleScaled;

/**
    The font provider contains all font families currently in use by RmlUi.
    @author Peter Curry
 */

class FontProvider {
public:
	static bool Initialise();
	static void Shutdown();

	/// Returns a handle to a font face that can be used to position and render text. This will return the closest match
	/// it can find, but in the event a font family is requested that does not exist, nullptr will be returned instead of a
	/// valid handle.
	/// @param[in] family The family of the desired font handle.
	/// @param[in] style The style of the desired font handle.
	/// @param[in] weight The weight of the desired font handle.
	/// @param[in] size The size of desired handle, in points.
	/// @return A valid handle if a matching (or closely matching) font face was found, nullptr otherwise.
	static FontFaceHandleScaled* GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size);

	/// Adds a new font face to the database. The face's family, style and weight will be determined from the face itself.
	static bool LoadFontFace(const String& file_name, bool fallback_face, Style::FontWeight weight = Style::FontWeight::Auto);

	/// Adds a new font face from memory.
	static bool LoadFontFace(const byte* data, int data_size, const String& font_family, Style::FontStyle style, Style::FontWeight weight,
		bool fallback_face);

	/// Return the number of fallback font faces.
	static int CountFallbackFontFaces();

	/// Return a font face handle with the given index, at the given font size.
	static FontFaceHandleScaled* GetFallbackFontFace(int index, int font_size);

	/// Releases resources owned by sized font faces, including their textures and rendered glyphs.
	static void ReleaseFontResources();

private:
	FontProvider();
	~FontProvider();

	static FontProvider& Get();

	bool LoadFontFace(const byte* data, int data_size, bool fallback_face, UniquePtr<byte[]> face_memory, const String& source, String font_family,
		Style::FontStyle style, Style::FontWeight weight);

	bool AddFace(FontFaceHandleFreetype face, const String& family, Style::FontStyle style, Style::FontWeight weight, bool fallback_face,
		UniquePtr<byte[]> face_memory);

	using FontFaceList = Vector<FontFace*>;
	using FontFamilyMap = UnorderedMap<String, UniquePtr<FontFamily>>;

	FontFamilyMap font_families;
	FontFaceList fallback_font_faces;

	static const String debugger_font_family_name;
};

} // namespace Rml
#endif
