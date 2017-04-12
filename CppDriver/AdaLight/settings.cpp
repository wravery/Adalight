#include "stdafx.h"
#include "settings.h"

#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>
#include <cpprest/json.h>

using namespace web::json;

const value& operator>>(const value& positionValue, settings::led_pos& position)
{
	const auto& positionObject = positionValue.as_object();

	position.x = static_cast<size_t>(positionObject.at(U("x")).as_integer());
	position.y = static_cast<size_t>(positionObject.at(U("y")).as_integer());

	return positionValue;
}

value& operator<<(value& positionValue, const settings::led_pos& position)
{
	positionValue = value::object(true);
	auto& positionObject = positionValue.as_object();

	positionObject[U("x")] = position.x;
	positionObject[U("y")] = position.y;

	return positionValue;
}

const value& operator>>(const value& displayValue, settings::display_config& display)
{
	const auto& displayObject = displayValue.as_object();

	display.horizontalCount = static_cast<size_t>(displayObject.at(U("horizontalCount")).as_integer());
	display.verticalCount = static_cast<size_t>(displayObject.at(U("verticalCount")).as_integer());

	const auto& positionArray = displayObject.at(U("positions")).as_array();

	display.positions.resize(positionArray.size());
	std::transform(positionArray.cbegin(), positionArray.cend(), display.positions.begin(), [](const value& positionEntry)
	{
		settings::led_pos position;

		positionEntry >> position;

		return position;
	});

	return displayValue;
}

value& operator<<(value& displayValue, const settings::display_config& display)
{
	displayValue = value::object(true);
	auto& displayObject = displayValue.as_object();

	displayObject[U("horizontalCount")] = display.horizontalCount;
	displayObject[U("verticalCount")] = display.verticalCount;

	auto& positionArray = displayObject[U("positions")];

	positionArray = value::array(display.positions.size());
	std::transform(display.positions.cbegin(), display.positions.cend(), positionArray.as_array().begin(), [](const settings::led_pos& position)
	{
		auto positionEntry = value::object(true);

		positionEntry << position;

		return positionEntry;
	});

	return displayValue;
}

const value& operator>>(const value& pixelValue, settings::opc_pixel_range& pixel)
{
	const auto& pixelObject = pixelValue.as_object();

	pixel.channel = static_cast<uint8_t>(pixelObject.at(U("channel")).as_integer());
	pixel.firstPixel = static_cast<size_t>(pixelObject.at(U("firstPixel")).as_integer());
	pixel.pixelCount = static_cast<size_t>(pixelObject.at(U("pixelCount")).as_integer());

	const auto& displayArray = pixelObject.at(U("displays")).as_array();

	std::transform(displayArray.cbegin(), displayArray.cend(), pixel.displays.begin(), [](const value& displayEntry)
	{
		settings::display_config display;

		displayEntry >> display;

		return display;
	});

	return pixelValue;
}

value& operator<<(value& pixelValue, const settings::opc_pixel_range& pixel)
{
	pixelValue = value::object(true);
	auto& pixelObject = pixelValue.as_object();

	pixelObject[U("channel")] = pixel.channel;
	pixelObject[U("firstPixel")] = pixel.firstPixel;
	pixelObject[U("pixelCount")] = pixel.pixelCount;

	auto& displayArray = pixelObject[U("displays")];

	displayArray = value::array(pixel.displays.size());
	std::transform(pixel.displays.cbegin(), pixel.displays.cend(), displayArray.as_array().begin(), [](const settings::display_config& display)
	{
		auto displayEntry = value::object(true);

		displayEntry << display;

		return displayEntry;
	});

	return pixelValue;
}

const value& operator>>(const value& serverValue, settings::opc_configuration& server)
{
	const auto& serverObject = serverValue.as_object();

	server.host = serverObject.at(U("host")).as_string();
	server.port = serverObject.at(U("port")).as_string();

	const auto& pixelArray = serverObject.at(U("pixels")).as_array();

	std::transform(pixelArray.cbegin(), pixelArray.cend(), server.pixels.begin(), [](const value& pixelEntry)
	{
		settings::opc_pixel_range pixel;

		pixelEntry >> pixel;

		return pixel;
	});

	return serverValue;
}

value& operator<<(value& serverValue, const settings::opc_configuration& server)
{
	serverValue = value::object(true);
	auto& serverObject = serverValue.as_object();

	serverObject[U("host")] = value::string(server.host);
	serverObject[U("port")] = value::string(server.port);

	auto& pixelArray = serverObject[U("pixels")];

	pixelArray = value::array(server.pixels.size());
	std::transform(server.pixels.cbegin(), server.pixels.cend(), pixelArray.as_array().begin(), [](const settings::opc_pixel_range& pixel)
	{
		auto pixelEntry = value::object(true);

		pixelEntry << pixel;

		return pixelEntry;
	});

	return serverValue;
}

const value& operator>>(const value& settingsValue, settings& settings)
{
	const auto& settingsObject = settingsValue.as_object();

	settings.minBrightness = static_cast<uint8_t>(settingsObject.at(U("minBrightness")).as_integer());
	settings.fade = settingsObject.at(U("fade")).as_double();
	settings.timeout = static_cast<DWORD>(settingsObject.at(U("timeout")).as_integer());
	settings.fpsMax = static_cast<UINT>(settingsObject.at(U("fpsMax")).as_integer());
	settings.throttleTimer = static_cast<UINT>(settingsObject.at(U("throttleTimer")).as_integer());

	const auto& displayArray = settingsObject.at(U("displays")).as_array();

	settings.displays.resize(displayArray.size());
	std::transform(displayArray.cbegin(), displayArray.cend(), settings.displays.begin(), [](const value& displayEntry)
	{
		settings::display_config display;

		displayEntry >> display;

		return display;
	});

	const auto& serverArray = settingsObject.at(U("servers")).as_array();

	settings.servers.resize(serverArray.size());
	std::transform(serverArray.cbegin(), serverArray.cend(), settings.servers.begin(), [](const value& serverEntry)
	{
		settings::opc_configuration server;

		serverEntry >> server;

		return server;
	});

	return settingsValue;
}

value& operator<<(value& settingsValue, const settings& settings)
{
	settingsValue = value::object(true);
	auto& settingsObject = settingsValue.as_object();

	settingsObject[U("minBrightness")] = settings.minBrightness;
	settingsObject[U("fade")] = settings.fade;
	settingsObject[U("timeout")] = static_cast<uint32_t>(settings.timeout);
	settingsObject[U("fpsMax")] = settings.fpsMax;
	settingsObject[U("throttleTimer")] = settings.throttleTimer;

	auto& displayArray = settingsObject[U("displays")];

	displayArray = value::array(settings.displays.size());
	std::transform(settings.displays.cbegin(), settings.displays.cend(), displayArray.as_array().begin(), [](const settings::display_config& display)
	{
		value displayEntry;

		displayEntry << display;

		return displayEntry;
	});

	auto& serverArray = settingsObject[U("servers")];

	serverArray = value::array(settings.servers.size());
	std::transform(settings.servers.cbegin(), settings.servers.cend(), serverArray.as_array().begin(), [](const settings::opc_configuration& server)
	{
		value serverEntry;

		serverEntry << server;

		return serverEntry;
	});

	return settingsValue;
}

settings::settings(std::wstring&& configFilePath)
	: _configFilePath(std::move(configFilePath))
{
	auto root = value::null();

	// Read the config file if we have one, otherwise use the default values.
	if (!_configFilePath.empty())
	{
		utility::ifstream_t ifs(_configFilePath);

		if (ifs.is_open())
		{
			ifs >> root;

			try
			{
				root >> *this;
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

		if (root.is_null())
		{
			// Write the current values back out to the config file for customization.
			utility::ofstream_t ofs(_configFilePath, std::ios::out | std::ios::trunc);

			if (ofs.is_open())
			{
				root << *this;
				ofs << root;
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
}
