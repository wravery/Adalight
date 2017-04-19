#include "stdafx.h"
#include "settings.h"

#include <algorithm>
#include <numeric>
#include <cmath>
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
		value positionEntry;

		positionEntry << position;

		return positionEntry;
	});

	return displayValue;
}

const value& operator>>(const value& pixelValue, settings::opc_pixel_range& pixel)
{
	const auto& pixelObject = pixelValue.as_object();

	pixel.pixelCount = static_cast<size_t>(pixelObject.at(U("pixelCount")).as_integer());

	const auto& displayArray = pixelObject.at(U("displayIndex")).as_array();

	pixel.displayIndex.resize(displayArray.size());
	std::transform(displayArray.cbegin(), displayArray.cend(), pixel.displayIndex.begin(), [](const value& displayEntry)
	{
		const auto& indexArray = displayEntry.as_array();
		std::vector<size_t> index(indexArray.size());

		std::transform(indexArray.cbegin(), indexArray.cend(), index.begin(), [](const value& indexValue)
		{
			return indexValue.as_integer();
		});

		return index;
	});

	return pixelValue;
}

value& operator<<(value& pixelValue, const settings::opc_pixel_range& pixel)
{
	pixelValue = value::object(true);
	auto& pixelObject = pixelValue.as_object();

	pixelObject[U("pixelCount")] = pixel.pixelCount;

	auto& displayArray = pixelObject[U("displayIndex")];

	displayArray = value::array(pixel.displayIndex.size());
	std::transform(pixel.displayIndex.cbegin(), pixel.displayIndex.cend(), displayArray.as_array().begin(), [](const std::vector<size_t>& index)
	{
		auto& indexArray = value::array(index.size());

		std::transform(index.cbegin(), index.cend(), indexArray.as_array().begin(), [](size_t entry)
		{
			return value::number(entry);
		});

		return indexArray;
	});

	return pixelValue;
}

const value& operator>>(const value& channelValue, settings::opc_channel& channel)
{
	const auto& channelObject = channelValue.as_object();

	channel.channel = static_cast<uint8_t>(channelObject.at(U("channel")).as_integer());

	const auto& pixelArray = channelObject.at(U("pixels")).as_array();

	channel.pixels.resize(pixelArray.size());
	std::transform(pixelArray.cbegin(), pixelArray.cend(), channel.pixels.begin(), [](const value& pixelEntry)
	{
		settings::opc_pixel_range pixel;

		pixelEntry >> pixel;

		return pixel;
	});

	return channelValue;
}

value& operator<<(value& channelValue, const settings::opc_channel& channel)
{
	channelValue = value::object(true);
	auto& channelObject = channelValue.as_object();

	channelObject[U("channel")] = channel.channel;

	auto& pixelArray = channelObject[U("pixels")];

	pixelArray = value::array(channel.pixels.size());
	std::transform(channel.pixels.cbegin(), channel.pixels.cend(), pixelArray.as_array().begin(), [](const settings::opc_pixel_range& pixel)
	{
		value pixelEntry;

		pixelEntry << pixel;

		return pixelEntry;
	});

	return channelValue;
}

const value& operator>>(const value& serverValue, settings::opc_server& server)
{
	const auto& serverObject = serverValue.as_object();

	server.host = serverObject.at(U("host")).as_string();
	server.port = serverObject.at(U("port")).as_string();
	server.alphaChannel = serverObject.at(U("alphaChannel")).as_bool();

	const auto& channelArray = serverObject.at(U("channels")).as_array();

	server.channels.resize(channelArray.size());
	std::transform(channelArray.cbegin(), channelArray.cend(), server.channels.begin(), [](const value& channelEntry)
	{
		settings::opc_channel channel;

		channelEntry >> channel;

		return channel;
	});

	return serverValue;
}

value& operator<<(value& serverValue, const settings::opc_server& server)
{
	serverValue = value::object(true);
	auto& serverObject = serverValue.as_object();

	serverObject[U("host")] = value::string(server.host);
	serverObject[U("port")] = value::string(server.port);
	serverObject[U("alphaChannel")] = server.alphaChannel;

	auto& channelArray = serverObject[U("channels")];

	channelArray = value::array(server.channels.size());
	std::transform(server.channels.cbegin(), server.channels.cend(), channelArray.as_array().begin(), [](const settings::opc_channel& channel)
	{
		value channelEntry;

		channelEntry << channel;

		return channelEntry;
	});

	return serverValue;
}

const value& operator>>(const value& settingsValue, settings& settings)
{
	const auto& settingsObject = settingsValue.as_object();

	settings.minBrightness = static_cast<uint8_t>(settingsObject.at(U("minBrightness")).as_integer());
	settings.fade = settingsObject.at(U("fade")).as_double();
	settings.timeout = static_cast<uint32_t>(settingsObject.at(U("timeout")).as_integer());
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
		settings::opc_server server;

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
	std::transform(settings.servers.cbegin(), settings.servers.cend(), serverArray.as_array().begin(), [](const settings::opc_server& server)
	{
		value serverEntry;

		serverEntry << server;

		return serverEntry;
	});

	return settingsValue;
}

settings::settings(const std::wstring& configFilePath)
{
	auto root = value::null();

	// Read the config file if we have one, otherwise use the default values.
	if (!configFilePath.empty())
	{
		utility::ifstream_t ifs(configFilePath);

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

			ifs.close();
		}

		if (root.is_null())
		{
			// Write the current values back out to the config file for customization.
			utility::ofstream_t ofs(configFilePath, std::ios::trunc);

			if (ofs.is_open())
			{
				root << *this;
				ofs << root;
			}
		}
	}

	for (auto& server : servers)
	{
		for (auto& channel : server.channels)
		{
			for (auto& pixel : channel.pixels)
			{
				pixel.sampleCount = std::accumulate(pixel.displayIndex.cbegin(), pixel.displayIndex.cend(), size_t(),
					[](size_t count, const std::vector<size_t>& index)
				{
					return count + index.size();
				});

				// It's not worth applying the blur if we don't have at least 3 pixels to average each sample.
				if (pixel.sampleCount > 1 && pixel.pixelCount >= 3 * pixel.sampleCount)
				{
					// Build the 1 dimensional Gaussian kernel for this range. The standard deviation term plugged into
					// the Gaussian function is 1/3 of the radius since the curve approaches 0 beyond 3 standard deviations.
					pixel.kernelRadius = pixel.pixelCount / (2 * pixel.sampleCount);

					const size_t samples = (2 * pixel.kernelRadius) + 1;
					const double denominator = static_cast<double>(pixel.kernelRadius * pixel.kernelRadius) / 4.5;
					double total = 1.0;

					// The midpoint is always 1.
					pixel.kernelWeights.resize(samples);
					pixel.kernelWeights[pixel.kernelRadius] = 1.0;

					// We only need to compute the first half, the second half is a mirror of those values.
					for (size_t x = 0; x < pixel.kernelRadius; ++x)
					{
						const double diff = static_cast<double>(x) - static_cast<double>(pixel.kernelRadius);
						const double weight = std::exp(-(diff * diff) / denominator);

						// Set the weight on both sides of the curve.
						total += 2.0 * weight;
						pixel.kernelWeights[x] = weight;
						pixel.kernelWeights[samples - x - 1] = weight;
					}

					// Normalize the weights so the area under the curve is 1.
					for (auto& weight : pixel.kernelWeights)
					{
						weight /= total;
					}
				}

				channel.totalSampleCount += pixel.sampleCount;
				channel.totalPixelCount += pixel.pixelCount;
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
