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

#include "FontEngineInterfaceScaled.h"
#include "FontFaceHandleScaled.h"
#include "FontProvider.h"

namespace RecompRml {

using namespace Rml;

FontEngineInterfaceScaled::FontEngineInterfaceScaled()
{
	FontProvider::Initialise();
}

FontEngineInterfaceScaled::~FontEngineInterfaceScaled()
{
	FontProvider::Shutdown();
}

bool FontEngineInterfaceScaled::LoadFontFace(const String& file_name, bool fallback_face, Style::FontWeight weight)
{
	return FontProvider::LoadFontFace(file_name, fallback_face, weight);
}

bool FontEngineInterfaceScaled::LoadFontFace(const byte* data, int data_size, const String& font_family, Style::FontStyle style,
	Style::FontWeight weight, bool fallback_face)
{
	return FontProvider::LoadFontFace(data, data_size, font_family, style, weight, fallback_face);
}

FontFaceHandle FontEngineInterfaceScaled::GetFontFaceHandle(const String& family, Style::FontStyle style, Style::FontWeight weight, int size)
{
	auto handle = FontProvider::GetFontFaceHandle(family, style, weight, size);
	return reinterpret_cast<FontFaceHandle>(handle);
}

FontEffectsHandle FontEngineInterfaceScaled::PrepareFontEffects(FontFaceHandle handle, const FontEffectList& font_effects)
{
	auto handle_scaled = reinterpret_cast<FontFaceHandleScaled*>(handle);
	return (FontEffectsHandle)handle_scaled->GenerateLayerConfiguration(font_effects);
}

const FontMetrics& FontEngineInterfaceScaled::GetFontMetrics(FontFaceHandle handle)
{
	auto handle_scaled = reinterpret_cast<FontFaceHandleScaled*>(handle);
	return handle_scaled->GetFontMetrics();
}

int FontEngineInterfaceScaled::GetStringWidth(FontFaceHandle handle, const String& string, float letter_spacing, Character prior_character)
{
	auto handle_scaled = reinterpret_cast<FontFaceHandleScaled*>(handle);
	return handle_scaled->GetStringWidth(string, letter_spacing, prior_character);
}

int FontEngineInterfaceScaled::GenerateString(FontFaceHandle handle, FontEffectsHandle font_effects_handle, const String& string,
	const Vector2f& position, const Colourb& colour, float opacity, float letter_spacing, GeometryList& geometry)
{
	auto handle_scaled = reinterpret_cast<FontFaceHandleScaled*>(handle);
	return handle_scaled->GenerateString(geometry, string, position, colour, opacity, letter_spacing, (int)font_effects_handle);
}

int FontEngineInterfaceScaled::GetVersion(FontFaceHandle handle)
{
	auto handle_scaled = reinterpret_cast<FontFaceHandleScaled*>(handle);
	return handle_scaled->GetVersion();
}

void FontEngineInterfaceScaled::ReleaseFontResources()
{
	FontProvider::ReleaseFontResources();
}

} // namespace Rml
