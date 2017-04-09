#include "stdafx.h"
#include "settings.h"

#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>
#include <cpprest/json.h>

using namespace web::json;

settings::settings(std::wstring&& configFilePath)
	: _configFilePath(std::move(configFilePath))
{
	auto root = value::null();

	// Read the config file if we have one, otherwise use the default values.
	if (!_configFilePath.empty())
	{
		std::wifstream ifs(_configFilePath);

		if (ifs.is_open())
		{
			ifs >> root;

			try
			{
				const auto& read = root.as_object();

				minBrightness = static_cast<uint8_t>(read.at(L"minBrightness").as_integer());
				fade = read.at(L"fade").as_double();
				timeout = static_cast<DWORD>(read.at(L"timeout").as_integer());
				fpsMax = static_cast<UINT>(read.at(L"fpsMax").as_integer());
				throttleTimer = static_cast<UINT>(read.at(L"throttleTimer").as_integer());

				const auto& displayArray = read.at(L"displays").as_array();

				displays.resize(displayArray.size());
				std::transform(displayArray.cbegin(), displayArray.cend(), displays.begin(), [](const value& displayEntry)
				{
					const auto& displayObject = displayEntry.as_object();
					display_config display;

					display.horizontalCount = static_cast<size_t>(displayObject.at(L"horizontalCount").as_integer());
					display.verticalCount = static_cast<size_t>(displayObject.at(L"verticalCount").as_integer());

					const auto& positionArray = displayEntry.at(L"positions").as_array();

					display.positions.resize(positionArray.size());
					std::transform(positionArray.cbegin(), positionArray.cend(), display.positions.begin(), [](const value& positionEntry)
					{
						const auto& positionObject = positionEntry.as_object();
						led_pos position;

						position.x = static_cast<size_t>(positionObject.at(L"x").as_integer());
						position.y = static_cast<size_t>(positionObject.at(L"y").as_integer());

						return position;
					});

					return display;
				});
			}
			catch (const json_exception& ex)
			{
				std::cerr << "Error parsing the config file: "
					<< ex.what()
					<< std::endl;

				// reset root to a null value and continue
				root = value::null();
			}
		}
	}

	minBrightnessColor = ((((minBrightness / 3) & 0xFF) << 24) // red
		| (((minBrightness / 3) & 0xFF) << 16) // green
		| (((minBrightness / 3) & 0xFF) << 8) // blue
		| 0xFF); // alpha

	totalLedCount = std::accumulate(displays.cbegin(), displays.cend(), size_t(),
		[](size_t count, const display_config& display)
	{
		return count + display.positions.size();
	});

	weight = 1.0 - fade;
	delay = 1000 / fpsMax;

	if (!_configFilePath.empty()
		&& root.is_null())
	{
		// Write the current values back out to the config file for customization.
		std::wofstream ofs(_configFilePath, std::ios::out | std::ios::trunc);

		if (ofs.is_open())
		{
			root = value::object(true);

			auto& write = root.as_object();

			write[L"minBrightness"] = minBrightness;
			write[L"fade"] = fade;
			write[L"timeout"] = static_cast<uint32_t>(timeout);
			write[L"fpsMax"] = fpsMax;
			write[L"throttleTimer"] = throttleTimer;

			auto& displayArray = write[L"displays"];

			displayArray = value::array(displays.size());
			std::transform(displays.cbegin(), displays.cend(), displayArray.as_array().begin(), [](const display_config& display)
			{
				auto displayEntry = value::object(true);

				displayEntry[L"horizontalCount"] = display.horizontalCount;
				displayEntry[L"verticalCount"] = display.verticalCount;

				auto& positionArray = displayEntry[L"positions"];

				positionArray = value::array(display.positions.size());
				std::transform(display.positions.cbegin(), display.positions.cend(), positionArray.as_array().begin(), [](const led_pos& position)
				{
					auto positionEntry = value::object(true);

					positionEntry[L"x"] = position.x;
					positionEntry[L"y"] = position.y;

					return positionEntry;
				});

				return displayEntry;
			});

			ofs << root;
		}
	}
}
