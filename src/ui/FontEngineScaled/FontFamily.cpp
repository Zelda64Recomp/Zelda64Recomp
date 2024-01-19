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

#include "FontFamily.h"
#include "RmlUi/Core/ComputedValues.h"
#include "RmlUi/Core/Math.h"
#include "FontFace.h"
#include <limits.h>

namespace RecompRml {

using namespace Rml;

FontFamily::FontFamily(const String& name) : name(name) {}

FontFamily::~FontFamily()
{
	// Multiple face entries may share memory within a single font family, although only one of them owns it. Here we make sure that all the face
	// destructors are run before all the memory is released. This way we don't leave any hanging references to invalidated memory.
	for (FontFaceEntry& entry : font_faces)
		entry.face.reset();
}

FontFaceHandleScaled* FontFamily::GetFaceHandle(Style::FontStyle style, Style::FontWeight weight, int size)
{
	int best_dist = INT_MAX;
	FontFace* matching_face = nullptr;
	for (size_t i = 0; i < font_faces.size(); i++)
	{
		FontFace* face = font_faces[i].face.get();

		if (face->GetStyle() == style)
		{
			const int dist = Math::Absolute((int)face->GetWeight() - (int)weight);
			if (dist == 0)
			{
				// Direct match for weight, break the loop early.
				matching_face = face;
				break;
			}
			else if (dist < best_dist)
			{
				// Best match so far for weight, store the face and dist.
				matching_face = face;
				best_dist = dist;
			}
		}
	}

	if (!matching_face)
		return nullptr;

	return matching_face->GetHandle(size, true);
}

FontFace* FontFamily::AddFace(FontFaceHandleFreetype ft_face, Style::FontStyle style, Style::FontWeight weight, UniquePtr<byte[]> face_memory)
{
	auto face = MakeUnique<FontFace>(ft_face, style, weight);
	FontFace* result = face.get();

	font_faces.push_back(FontFaceEntry{std::move(face), std::move(face_memory)});

	return result;
}

void FontFamily::ReleaseFontResources()
{
	for (auto& entry : font_faces)
		entry.face->ReleaseFontResources();
}

} // namespace Rml
