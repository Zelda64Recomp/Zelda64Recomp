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

#include "FreeTypeInterface.h"
#include "RmlUi/Core/ComputedValues.h"
#include "RmlUi/Core/FontMetrics.h"
#include "RmlUi/Core/Log.h"
#include <algorithm>
#include <ft2build.h>
#include <limits.h>
#include <string.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H
#include FT_TRUETYPE_TABLES_H

namespace RecompRml {

using namespace Rml;

static FT_Library ft_library = nullptr;

static bool BuildGlyph(FT_Face ft_face, Character character, FontGlyphMap& glyphs, float bitmap_scaling_factor);
static void BuildGlyphMap(FT_Face ft_face, int size, FontGlyphMap& glyphs, float bitmap_scaling_factor, bool load_default_glyphs);
static void GenerateMetrics(FT_Face ft_face, FontMetrics& metrics, float bitmap_scaling_factor);
static bool SetFontSize(FT_Face ft_face, int font_size, float& out_bitmap_scaling_factor);
static void BitmapDownscale(byte* bitmap_new, int new_width, int new_height, const byte* bitmap_source, int width, int height, int pitch,
	ColorFormat color_format);

static int ConvertFixed16_16ToInt(int32_t fx)
{
	return fx / 0x10000;
}

bool FreeType::Initialise()
{
	RMLUI_ASSERT(!ft_library);

	FT_Error result = FT_Init_FreeType(&ft_library);
	if (result != 0)
	{
		Log::Message(Log::LT_ERROR, "Failed to initialise FreeType, error %d.", result);
		Shutdown();
		return false;
	}

	return true;
}

void FreeType::Shutdown()
{
	if (ft_library != nullptr)
	{
		FT_Done_FreeType(ft_library);
		ft_library = nullptr;
	}
}

bool FreeType::GetFaceVariations(const byte* data, int data_length, Vector<FaceVariation>& out_face_variations)
{
	RMLUI_ASSERT(ft_library);

	FT_Face face = nullptr;
	FT_Error error = FT_New_Memory_Face(ft_library, (const FT_Byte*)data, data_length, 0, &face);
	if (error)
		return false;

	FT_MM_Var* var = nullptr;
	error = FT_Get_MM_Var(face, &var);
	if (error)
		return true;

	unsigned int axis_index_weight = 0;
	unsigned int axis_index_width = 1;

	const unsigned int num_axis = var->num_axis;
	for (unsigned int i = 0; i < num_axis; i++)
	{
		switch (var->axis[i].tag)
		{
		case 0x77676874: // 'wght'
			axis_index_weight = i;
			break;
		case 0x77647468: // 'wdth'
			axis_index_width = i;
			break;
		}
	}

	if (num_axis > 0)
	{
		for (unsigned int i = 0; i < var->num_namedstyles; i++)
		{
			uint16_t weight = (axis_index_weight < num_axis ? (uint16_t)ConvertFixed16_16ToInt(var->namedstyle[i].coords[axis_index_weight]) : 0);
			uint16_t width = (axis_index_width < num_axis ? (uint16_t)ConvertFixed16_16ToInt(var->namedstyle[i].coords[axis_index_width]) : 0);
			int named_instance_index = (i + 1);

			out_face_variations.push_back(FaceVariation{weight == 0 ? Style::FontWeight::Normal : (Style::FontWeight)weight,
				width == 0 ? (uint16_t)100 : width, named_instance_index});
		}
	}

	std::sort(out_face_variations.begin(), out_face_variations.end());

#if FREETYPE_MAJOR >= 2 && FREETYPE_MINOR >= 9
	FT_Done_MM_Var(ft_library, var);
#endif

	FT_Done_Face(face);

	return true;
}

FontFaceHandleFreetype FreeType::LoadFace(const byte* data, int data_length, const String& source, int named_style_index)
{
	RMLUI_ASSERT(ft_library);

	FT_Face face = nullptr;
	FT_Error error = FT_New_Memory_Face(ft_library, (const FT_Byte*)data, data_length, (named_style_index << 16), &face);
	if (error)
	{
		Log::Message(Log::LT_ERROR, "FreeType error %d while loading face from %s.", error, source.c_str());
		return 0;
	}

	// Initialise the character mapping on the face.
	if (face->charmap == nullptr)
	{
		FT_Select_Charmap(face, FT_ENCODING_APPLE_ROMAN);
		if (face->charmap == nullptr)
		{
			Log::Message(Log::LT_ERROR, "Font face (from %s) does not contain a Unicode or Apple Roman character map.", source.c_str());
			FT_Done_Face(face);
			return 0;
		}
	}

	return (FontFaceHandleFreetype)face;
}

bool FreeType::ReleaseFace(FontFaceHandleFreetype in_face)
{
	FT_Face face = (FT_Face)in_face;
	FT_Error error = FT_Done_Face(face);

	return (error == 0);
}

void FreeType::GetFaceStyle(FontFaceHandleFreetype in_face, String* font_family, Style::FontStyle* style, Style::FontWeight* weight)
{
	FT_Face face = (FT_Face)in_face;

	if (font_family)
		*font_family = face->family_name;
	if (style)
		*style = face->style_flags & FT_STYLE_FLAG_ITALIC ? Style::FontStyle::Italic : Style::FontStyle::Normal;

	if (weight)
	{
		TT_OS2* font_table = (TT_OS2*)FT_Get_Sfnt_Table(face, FT_SFNT_OS2);
		if (font_table && font_table->usWeightClass != 0)
			*weight = (Style::FontWeight)font_table->usWeightClass;
		else
			*weight = (face->style_flags & FT_STYLE_FLAG_BOLD) == FT_STYLE_FLAG_BOLD ? Style::FontWeight::Bold : Style::FontWeight::Normal;
	}
}

bool FreeType::InitialiseFaceHandle(FontFaceHandleFreetype face, int font_size, FontGlyphMap& glyphs, FontMetrics& metrics, bool load_default_glyphs)
{
	FT_Face ft_face = (FT_Face)face;

	metrics.size = font_size;

	float bitmap_scaling_factor = 1.0f;
	if (!SetFontSize(ft_face, font_size, bitmap_scaling_factor))
		return false;

	// Construct the initial list of glyphs.
	BuildGlyphMap(ft_face, font_size / 4, glyphs, bitmap_scaling_factor, load_default_glyphs);

	// Generate the metrics for the handle.
	GenerateMetrics(ft_face, metrics, bitmap_scaling_factor);

	return true;
}

bool FreeType::AppendGlyph(FontFaceHandleFreetype face, int font_size, Character character, FontGlyphMap& glyphs)
{
	FT_Face ft_face = (FT_Face)face;

	RMLUI_ASSERT(glyphs.find(character) == glyphs.end());
	RMLUI_ASSERT(ft_face);

	// Set face size again in case it was used at another size in another font face handle.
	float bitmap_scaling_factor = 1.0f;
	if (!SetFontSize(ft_face, font_size, bitmap_scaling_factor))
		return false;

	if (!BuildGlyph(ft_face, character, glyphs, bitmap_scaling_factor))
		return false;

	return true;
}

int FreeType::GetKerning(FontFaceHandleFreetype face, int font_size, Character lhs, Character rhs)
{
	FT_Face ft_face = (FT_Face)face;

	RMLUI_ASSERT(FT_HAS_KERNING(ft_face));

	// Set face size again in case it was used at another size in another font face handle.
	// Font size value of zero assumes it is already set.
	if (font_size > 0)
	{
		float bitmap_scaling_factor = 1.0f;
		if (!SetFontSize(ft_face, font_size, bitmap_scaling_factor) || bitmap_scaling_factor != 1.0f)
			return 0;
	}

	FT_Vector ft_kerning;

	FT_Error ft_error = FT_Get_Kerning(ft_face, FT_Get_Char_Index(ft_face, (FT_ULong)lhs), FT_Get_Char_Index(ft_face, (FT_ULong)rhs),
		FT_KERNING_DEFAULT, &ft_kerning);

	if (ft_error)
		return 0;

	int kerning = ft_kerning.x >> 6;
	return kerning;
}

bool FreeType::HasKerning(FontFaceHandleFreetype face)
{
	FT_Face ft_face = (FT_Face)face;

	return FT_HAS_KERNING(ft_face);
}

static void BuildGlyphMap(FT_Face ft_face, int size, FontGlyphMap& glyphs, const float bitmap_scaling_factor, const bool load_default_glyphs)
{
	if (load_default_glyphs)
	{
		glyphs.reserve(128);

		// Add the ASCII characters now. Other characters are added later as needed.
		FT_ULong code_min = 32;
		FT_ULong code_max = 126;

		for (FT_ULong character_code = code_min; character_code <= code_max; ++character_code)
			BuildGlyph(ft_face, (Character)character_code, glyphs, bitmap_scaling_factor);
	}

	// Add a replacement character for rendering unknown characters.
	Character replacement_character = Character::Replacement;
	auto it = glyphs.find(replacement_character);
	if (it == glyphs.end())
	{
		FontGlyph glyph;
		glyph.dimensions = {size / 3, (size * 2) / 3};
		glyph.bitmap_dimensions = glyph.dimensions;
		glyph.advance = glyph.dimensions.x + 2;
		glyph.bearing = {1, glyph.dimensions.y};

		glyph.bitmap_owned_data.reset(new byte[glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y]);
		glyph.bitmap_data = glyph.bitmap_owned_data.get();

		for (int y = 0; y < glyph.bitmap_dimensions.y; y++)
		{
			for (int x = 0; x < glyph.bitmap_dimensions.x; x++)
			{
				constexpr int stroke = 1;
				int i = y * glyph.bitmap_dimensions.x + x;
				bool near_edge = (x < stroke || x >= glyph.bitmap_dimensions.x - stroke || y < stroke || y >= glyph.bitmap_dimensions.y - stroke);
				glyph.bitmap_owned_data[i] = (near_edge ? 0xdd : 0);
			}
		}

		glyphs[replacement_character] = std::move(glyph);
	}
}

static bool BuildGlyph(FT_Face ft_face, const Character character, FontGlyphMap& glyphs, const float bitmap_scaling_factor)
{
	FT_UInt index = FT_Get_Char_Index(ft_face, (FT_ULong)character);
	if (index == 0)
		return false;

	FT_Error error = FT_Load_Glyph(ft_face, index, FT_LOAD_COLOR);
	if (error != 0)
	{
		Log::Message(Log::LT_WARNING, "Unable to load glyph for character '%u' on the font face '%s %s'; error code: %d.", (unsigned int)character,
			ft_face->family_name, ft_face->style_name, error);
		return false;
	}

	error = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);
	if (error != 0)
	{
		Log::Message(Log::LT_WARNING, "Unable to render glyph for character '%u' on the font face '%s %s'; error code: %d.", (unsigned int)character,
			ft_face->family_name, ft_face->style_name, error);
		return false;
	}

	auto result = glyphs.emplace(character, FontGlyph{});
	if (!result.second)
	{
		Log::Message(Log::LT_WARNING, "Glyph character '%u' is already loaded in the font face '%s %s'.", (unsigned int)character,
			ft_face->family_name, ft_face->style_name);
		return false;
	}

	FontGlyph& glyph = result.first->second;

	FT_GlyphSlot ft_glyph = ft_face->glyph;

	// Set the glyph's dimensions.
	glyph.dimensions.x = (ft_glyph->metrics.width * global_font_scale) >> 6;
	glyph.dimensions.y = (ft_glyph->metrics.height * global_font_scale) >> 6;

	// Set the glyph's bearing.
	glyph.bearing.x = (ft_glyph->metrics.horiBearingX * global_font_scale) >> 6;
	glyph.bearing.y = (ft_glyph->metrics.horiBearingY * global_font_scale) >> 6;

	// Set the glyph's advance.
	glyph.advance = (ft_glyph->metrics.horiAdvance * global_font_scale) >> 6;

	// Set the glyph's bitmap dimensions.
	glyph.bitmap_dimensions.x = ft_glyph->bitmap.width;
	glyph.bitmap_dimensions.y = ft_glyph->bitmap.rows;

	// Determine new metrics if we need to scale the bitmap received from FreeType. Only allow bitmap downscaling.
	const bool scale_bitmap = (bitmap_scaling_factor < 1.f);
	if (scale_bitmap)
	{
		glyph.dimensions = Vector2i(Vector2f(glyph.dimensions) * bitmap_scaling_factor);
		glyph.bearing = Vector2i(Vector2f(glyph.bearing) * bitmap_scaling_factor);
		glyph.advance = int(float(glyph.advance) * bitmap_scaling_factor);
		glyph.bitmap_dimensions = Vector2i(Vector2f(glyph.bitmap_dimensions) * bitmap_scaling_factor);
	}

	// Copy the glyph's bitmap data from the FreeType glyph handle to our glyph handle.
	if (glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y != 0)
	{
		// Check if the pixel mode is supported.
		if (ft_glyph->bitmap.pixel_mode != FT_PIXEL_MODE_MONO && ft_glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY &&
			ft_glyph->bitmap.pixel_mode != FT_PIXEL_MODE_BGRA)
		{
			Log::Message(Log::LT_WARNING, "Unable to render glyph on the font face '%s %s': unsupported pixel mode (%d).",
				ft_glyph->face->family_name, ft_glyph->face->style_name, ft_glyph->bitmap.pixel_mode);
		}
		else if (ft_glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO && scale_bitmap)
		{
			Log::Message(Log::LT_WARNING, "Unable to render glyph on the font face '%s %s': bitmap scaling unsupported in mono pixel mode.",
				ft_glyph->face->family_name, ft_glyph->face->style_name);
		}
		else
		{
			const int num_bytes_per_pixel = (ft_glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA ? 4 : 1);
			glyph.color_format = (ft_glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA ? ColorFormat::RGBA8 : ColorFormat::A8);

			glyph.bitmap_owned_data.reset(new byte[glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y * num_bytes_per_pixel]);
			glyph.bitmap_data = glyph.bitmap_owned_data.get();
			byte* destination_bitmap = glyph.bitmap_owned_data.get();
			const byte* source_bitmap = ft_glyph->bitmap.buffer;

			// Copy the bitmap data into the newly-allocated space on our glyph.
			switch (ft_glyph->bitmap.pixel_mode)
			{
			case FT_PIXEL_MODE_MONO:
			{
				// Unpack 1-bit data into 8-bit.
				for (int i = 0; i < glyph.bitmap_dimensions.y; ++i)
				{
					int mask = 0x80;
					const byte* source_byte = source_bitmap;
					for (int j = 0; j < glyph.bitmap_dimensions.x; ++j)
					{
						if ((*source_byte & mask) == mask)
							destination_bitmap[j] = 255;
						else
							destination_bitmap[j] = 0;

						mask >>= 1;
						if (mask <= 0)
						{
							mask = 0x80;
							++source_byte;
						}
					}

					destination_bitmap += glyph.bitmap_dimensions.x;
					source_bitmap += ft_glyph->bitmap.pitch;
				}
			}
			break;
			case FT_PIXEL_MODE_GRAY:
			case FT_PIXEL_MODE_BGRA:
			{
				if (scale_bitmap)
				{
					// Resize the glyph data to the new dimensions.
					BitmapDownscale(destination_bitmap, glyph.bitmap_dimensions.x, glyph.bitmap_dimensions.y, source_bitmap,
						(int)ft_glyph->bitmap.width, (int)ft_glyph->bitmap.rows, ft_glyph->bitmap.pitch, glyph.color_format);
				}
				else
				{
					// Copy the glyph data directly.
					const int num_bytes_per_line = glyph.bitmap_dimensions.x * num_bytes_per_pixel;
					for (int i = 0; i < glyph.bitmap_dimensions.y; ++i)
					{
						memcpy(destination_bitmap, source_bitmap, num_bytes_per_line);
						destination_bitmap += num_bytes_per_line;
						source_bitmap += ft_glyph->bitmap.pitch;
					}
				}

				if (glyph.color_format == ColorFormat::RGBA8)
				{
					// Swizzle channels (BGRA -> RGBA) and un-premultiply alpha.
					destination_bitmap = glyph.bitmap_owned_data.get();

					for (int k = 0; k < glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y * num_bytes_per_pixel; k += 4)
					{
						byte b = destination_bitmap[k];
						byte g = destination_bitmap[k + 1];
						byte r = destination_bitmap[k + 2];
						const byte alpha = destination_bitmap[k + 3];
						RMLUI_ASSERTMSG(b <= alpha && g <= alpha && r <= alpha, "Assumption of glyph data being premultiplied is broken.");

						if (alpha > 0 && alpha < 255)
						{
							b = byte((b * 255) / alpha);
							g = byte((g * 255) / alpha);
							r = byte((r * 255) / alpha);
						}

						destination_bitmap[k] = r;
						destination_bitmap[k + 1] = g;
						destination_bitmap[k + 2] = b;
						destination_bitmap[k + 3] = alpha;
					}
				}
			}
			break;
			}
		}
	}

	return true;
}

static void GenerateMetrics(FT_Face ft_face, FontMetrics& metrics, float bitmap_scaling_factor)
{
	metrics.ascent = ft_face->size->metrics.ascender * bitmap_scaling_factor * global_font_scale / float(1 << 6);
	metrics.descent = -ft_face->size->metrics.descender * bitmap_scaling_factor * global_font_scale / float(1 << 6);
	metrics.line_spacing = ft_face->size->metrics.height * bitmap_scaling_factor * global_font_scale / float(1 << 6);

	metrics.underline_position = FT_MulFix(-ft_face->underline_position, ft_face->size->metrics.y_scale) * bitmap_scaling_factor * global_font_scale / float(1 << 6);
	metrics.underline_thickness = FT_MulFix(ft_face->underline_thickness, ft_face->size->metrics.y_scale) * bitmap_scaling_factor * global_font_scale / float(1 << 6);
	metrics.underline_thickness = Math::Max(metrics.underline_thickness, 1.0f);

	// Determine the x-height of this font face.
	FT_UInt index = FT_Get_Char_Index(ft_face, 'x');
	if (index != 0 && FT_Load_Glyph(ft_face, index, 0) == 0)
		metrics.x_height = ft_face->glyph->metrics.height * bitmap_scaling_factor * global_font_scale / float(1 << 6);
	else
		metrics.x_height = 0.5f * metrics.line_spacing;
}

static bool SetFontSize(FT_Face ft_face, int font_size, float& out_bitmap_scaling_factor)
{
	RMLUI_ASSERT(out_bitmap_scaling_factor == 1.f);
	font_size /= global_font_scale;

	FT_Error error = 0;

	// Set the character size on the font face.
	error = FT_Set_Char_Size(ft_face, 0, font_size << 6, 0, 0);

	// If setting char size fails, try to select a bitmap strike instead when available.
	if (error != 0 && FT_HAS_FIXED_SIZES(ft_face))
	{
		constexpr int a_big_number = INT_MAX / 2;
		int heuristic_min = INT_MAX;
		int index_min = -1;

		// Select the bitmap strike with the smallest size *above* font_size, or else the largest size.
		for (int i = 0; i < ft_face->num_fixed_sizes; i++)
		{
			const int size_diff = ft_face->available_sizes[i].height - font_size;
			const int heuristic = (size_diff < 0 ? a_big_number - size_diff : size_diff);

			if (heuristic < heuristic_min)
			{
				index_min = i;
				heuristic_min = heuristic;
			}
		}

		if (index_min >= 0)
		{
			out_bitmap_scaling_factor = float(font_size) / ft_face->available_sizes[index_min].height;

			// Add some tolerance to the scaling factor to avoid unnecessary scaling. Only allow downscaling.
			constexpr float bitmap_scaling_factor_threshold = 0.95f;
			if (out_bitmap_scaling_factor >= bitmap_scaling_factor_threshold)
				out_bitmap_scaling_factor = 1.f;

			error = FT_Select_Size(ft_face, index_min);
		}
	}

	if (error != 0)
	{
		Log::Message(Log::LT_ERROR, "Unable to set the character size '%d' on the font face '%s %s'.", font_size, ft_face->family_name,
			ft_face->style_name);
		return false;
	}

	return true;
}

static void BitmapDownscale(byte* bitmap_new, const int new_width, const int new_height, const byte* bitmap_source, const int width, const int height,
	const int pitch, const ColorFormat color_format)
{
	// Average filter for downscaling bitmap images, based on https://stackoverflow.com/a/9571580
	constexpr int max_num_channels = 4;
	const int num_channels = (color_format == ColorFormat::RGBA8 ? 4 : 1);

	const float xscale = float(new_width) / width;
	const float yscale = float(new_height) / height;
	const float sumscale = xscale * yscale;

	float yend = 0.0f;
	for (int f = 0; f < new_height; f++) // y on output
	{
		const float ystart = yend;
		yend = (f + 1) * (1.f / yscale);
		if (yend >= height)
			yend = height - 0.001f;

		float xend = 0.0;
		for (int g = 0; g < new_width; g++) // x on output
		{
			float xstart = xend;
			xend = (g + 1) * (1.f / xscale);
			if (xend >= width)
				xend = width - 0.001f;

			float sum[max_num_channels] = {};
			for (int y = (int)ystart; y <= (int)yend; ++y)
			{
				float yportion = 1.0f;
				if (y == (int)ystart)
					yportion -= ystart - y;
				if (y == (int)yend)
					yportion -= y + 1 - yend;

				for (int x = (int)xstart; x <= (int)xend; ++x)
				{
					float xportion = 1.0f;
					if (x == (int)xstart)
						xportion -= xstart - x;
					if (x == (int)xend)
						xportion -= x + 1 - xend;

					for (int i = 0; i < num_channels; i++)
						sum[i] += bitmap_source[y * pitch + x * num_channels + i] * yportion * xportion;
				}
			}

			for (int i = 0; i < num_channels; i++)
				bitmap_new[(f * new_width + g) * num_channels + i] = (byte)Math::Min(sum[i] * sumscale, 255.f);
		}
	}
}

} // namespace Rml
