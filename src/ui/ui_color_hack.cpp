
#include "RmlUi/Core.h"
#include "RmlUi/../../Source/Core/PropertyParserColour.h"
#include "recomp_ui.h"
#include <string.h>

using ColourMap = Rml::UnorderedMap<Rml::String, Rml::Colourb>;

namespace recompui {
	class PropertyParserColorHack : public Rml::PropertyParser {
	public:
		PropertyParserColorHack();
		virtual ~PropertyParserColorHack();
		bool ParseValue(Rml::Property& property, const Rml::String& value, const Rml::ParameterMap& /*parameters*/) const override;
	private:
		static ColourMap html_colours;
	};
	static_assert(sizeof(PropertyParserColorHack) == sizeof(Rml::PropertyParserColour));
	PropertyParserColorHack::PropertyParserColorHack() {
		html_colours["black"] = Rml::Colourb(0, 0, 0);
		html_colours["silver"] = Rml::Colourb(192, 192, 192);
		html_colours["gray"] = Rml::Colourb(128, 128, 128);
		html_colours["grey"] = Rml::Colourb(128, 128, 128);
		html_colours["white"] = Rml::Colourb(255, 255, 255);
		html_colours["maroon"] = Rml::Colourb(128, 0, 0);
		html_colours["red"] = Rml::Colourb(255, 0, 0);
		html_colours["orange"] = Rml::Colourb(255, 165, 0);
		html_colours["purple"] = Rml::Colourb(128, 0, 128);
		html_colours["fuchsia"] = Rml::Colourb(255, 0, 255);
		html_colours["green"] = Rml::Colourb(0, 128, 0);
		html_colours["lime"] = Rml::Colourb(0, 255, 0);
		html_colours["olive"] = Rml::Colourb(128, 128, 0);
		html_colours["yellow"] = Rml::Colourb(255, 255, 0);
		html_colours["navy"] = Rml::Colourb(0, 0, 128);
		html_colours["blue"] = Rml::Colourb(0, 0, 255);
		html_colours["teal"] = Rml::Colourb(0, 128, 128);
		html_colours["aqua"] = Rml::Colourb(0, 255, 255);
		html_colours["transparent"] = Rml::Colourb(0, 0, 0, 0);
		html_colours["whitesmoke"] = Rml::Colourb(245, 245, 245);
	}

	PropertyParserColorHack::~PropertyParserColorHack() {}

	bool PropertyParserColorHack::ParseValue(Rml::Property& property, const Rml::String& value, const Rml::ParameterMap& /*parameters*/) const {
		if (value.empty())
			return false;

		Rml::Colourb colour;

		// Check for a hex colour.
		if (value[0] == '#')
		{
			char hex_values[4][2] = { {'f', 'f'}, {'f', 'f'}, {'f', 'f'}, {'f', 'f'} };

			switch (value.size())
			{
				// Single hex digit per channel, RGB and alpha.
			case 5:
				hex_values[3][0] = hex_values[3][1] = value[4];
				//-fallthrough
			// Single hex digit per channel, RGB only.
			case 4:
				hex_values[0][0] = hex_values[0][1] = value[1];
				hex_values[1][0] = hex_values[1][1] = value[2];
				hex_values[2][0] = hex_values[2][1] = value[3];
				break;

				// Two hex digits per channel, RGB and alpha.
			case 9:
				hex_values[3][0] = value[7];
				hex_values[3][1] = value[8];
				//-fallthrough
			// Two hex digits per channel, RGB only.
			case 7: memcpy(hex_values, &value.c_str()[1], sizeof(char) * 6); break;

			default: return false;
			}

			// Parse each of the colour elements.
			for (int i = 0; i < 4; i++)
			{
				int tens = Rml::Math::HexToDecimal(hex_values[i][0]);
				int ones = Rml::Math::HexToDecimal(hex_values[i][1]);
				if (tens == -1 || ones == -1)
					return false;

				colour[i] = (Rml::byte)(tens * 16 + ones);
			}
		}
		else if (value.substr(0, 3) == "rgb")
		{
			Rml::StringList values;
			values.reserve(4);

			size_t find = value.find('(');
			if (find == Rml::String::npos)
				return false;

			size_t begin_values = find + 1;

			Rml::StringUtilities::ExpandString(values, value.substr(begin_values, value.rfind(')') - begin_values), ',');

			// Check if we're parsing an 'rgba' or 'rgb' colour declaration.
			if (value.size() > 3 && value[3] == 'a')
			{
				if (values.size() != 4)
					return false;
			}
			else
			{
				if (values.size() != 3)
					return false;

				values.push_back("255");
			}

			// Parse the three RGB values.
			for (int i = 0; i < 3; ++i)
			{
				int component;

				// We're parsing a percentage value.
				if (values[i].size() > 0 && values[i][values[i].size() - 1] == '%')
					component = int((float)atof(values[i].substr(0, values[i].size() - 1).c_str()) * (255.0f / 100.0f));
				// We're parsing a 0 -> 255 integer value.
				else
					component = atoi(values[i].c_str());

				colour[i] = (Rml::byte)(Rml::Math::Clamp(component, 0, 255));
			}
			// Parse the alpha value. Modified from the original RmlUi implementation to use 0-1 instead of 0-255.
			{
				int component;

				// We're parsing a percentage value.
				if (values[3].size() > 0 && values[3][values[3].size() - 1] == '%')
					component = ((float)atof(values[3].substr(0, values[3].size() - 1).c_str()) * (255.0f / 100.0f));
				// We're parsing a 0 -> 1 float value.
				else
					component = atof(values[3].c_str()) * 255.0f;

				colour[3] = (Rml::byte)(Rml::Math::Clamp(component, 0, 255));
			}
		}
		else
		{
			// Check for the specification of an HTML colour.
			ColourMap::const_iterator iterator = html_colours.find(Rml::StringUtilities::ToLower(value));
			if (iterator == html_colours.end())
				return false;
			else
				colour = (*iterator).second;
		}

		property.value = Rml::Variant(colour);
		property.unit = Rml::Unit::COLOUR;

		return true;
	}

	// This hack overwrites the contents of a property parser pointer for "color" (which is known to point to a valid Rml::PropertyParserColour) with the contents of a PropertyParserColorHack.
	// This overwrites the object's vtable, allowing us to override color parsing behavior to use 0-1 alpha instead of 0-255.
	// Ideally we'd just replace the pointer itself, but RmlUi doesn't provide a way to do that currently.
	void apply_color_hack() {
		// Allocate and leak a parser to act as a vtable source.
		PropertyParserColorHack* new_parser = new PropertyParserColorHack();
		// Copy the allocated object into the color parser pointer to overwrite its vtable.
		memcpy((void*)Rml::StyleSheetSpecification::GetParser("color"), (void*)new_parser, sizeof(*new_parser));
	}

	ColourMap PropertyParserColorHack::html_colours{};
}
