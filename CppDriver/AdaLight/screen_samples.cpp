#include "stdafx.h"
#include "screen_samples.h"

#ifdef _DEBUG
#include <string>
#include <sstream>
#endif

#undef min
#undef max

screen_samples::screen_samples(const settings& parameters, const gamma_correction& gamma)
	: _parameters(parameters)
	, _gamma(gamma)
{
}

bool screen_samples::create_resources()
{
	if (_acquiredResources)
	{
		return true;
	}
	else if (!get_factory())
	{
		return false;
	}

	_displays.reserve(_parameters.displays.size());

	// Get the display dimensions and devices.
	IDXGIAdapter1Ptr adapter;

	for (UINT i = 0; _displays.size() < _parameters.displays.size() && SUCCEEDED(_factory->EnumAdapters1(i, &adapter)); ++i)
	{
		IDXGIOutputPtr output;

		for (UINT j = 0; _displays.size() < _parameters.displays.size() && SUCCEEDED(adapter->EnumOutputs(j, &output)); ++j)
		{
			IDXGIOutput1Ptr output1(output);
			DXGI_OUTPUT_DESC outputDescription;
			IDXGIOutputDuplicationPtr duplication;
			ID3D11DevicePtr device;
			ID3D11DeviceContextPtr context;

			constexpr D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_UNKNOWN;
			constexpr ULONG createFlags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT;

			if (output1
				&& SUCCEEDED(output1->GetDesc(&outputDescription))
				&& outputDescription.AttachedToDesktop
				&& SUCCEEDED(D3D11CreateDevice(adapter, driverType, NULL, createFlags, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context))
				&& SUCCEEDED(output1->DuplicateOutput(device, &duplication)))
			{
				DXGI_OUTDUPL_DESC duplicationDescription;

				duplication->GetDesc(&duplicationDescription);

				const bool useMapDesktopSurface = !!duplicationDescription.DesktopImageInSystemMemory;
				ID3D11Texture2DPtr staging;
				const RECT& bounds = outputDescription.DesktopCoordinates;
				const LONG width = bounds.right - bounds.left;
				const LONG height = bounds.bottom - bounds.top;

				if (!useMapDesktopSurface)
				{
					D3D11_TEXTURE2D_DESC textureDescription;

					textureDescription.Width = static_cast<UINT>(width);
					textureDescription.Height = static_cast<UINT>(height);
					textureDescription.MipLevels = 1;
					textureDescription.ArraySize = 1;
					textureDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
					textureDescription.SampleDesc.Count = 1;
					textureDescription.SampleDesc.Quality = 0;
					textureDescription.Usage = D3D11_USAGE_STAGING;
					textureDescription.BindFlags = 0;
					textureDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
					textureDescription.MiscFlags = 0;

					device->CreateTexture2D(&textureDescription, NULL, &staging);
				}

				if (useMapDesktopSurface || staging)
				{
					_displays.push_back({
						adapter,
						device,
						context,
						duplication,
						staging,
						false,
						{ width, height }
					});
				}
			}

			output.Release();
		}

		adapter.Release();
	}

	if (_displays.empty())
	{
		return false;
	}

	// Samples take the center point of each cell in a 16x16 grid
	constexpr size_t pixel_samples = 16;
	static_assert(pixel_samples * pixel_samples == offset_array().size(), "size mismatch!");

	_pixelOffsets.resize(_displays.size());

	// Calculate the sub-sampled pixel offsets
	for (size_t i = 0; i < _displays.size(); ++i)
	{
		const auto& display = _parameters.displays[i];

		_pixelOffsets[i].resize(display.positions.size());

		for (size_t j = 0; j < display.positions.size(); ++j)
		{
			auto& offsets = _pixelOffsets[i][j];
			const auto& led = display.positions[j];
			const auto& displayBounds = _displays[i].bounds;
			const double rangeX = (static_cast<double>(displayBounds.cx) / static_cast<double>(display.horizontalCount));
			const double stepX = rangeX / static_cast<double>(pixel_samples);
			const double startX = (rangeX * static_cast<double>(led.x)) + (stepX / 2.0);
			const double rangeY = (static_cast<double>(displayBounds.cy) / static_cast<double>(display.verticalCount));
			const double stepY = rangeY / static_cast<double>(pixel_samples);
			const double startY = (rangeY * static_cast<double>(led.y)) + (stepY / 2.0);

			size_t x[pixel_samples];
			size_t y[pixel_samples];

			for (size_t j = 0; j < pixel_samples; ++j)
			{
				x[j] = static_cast<size_t>(startX + (stepX * static_cast<double>(j)));
				y[j] = static_cast<size_t>(startY + (stepY * static_cast<double>(j)));
			}

			for (size_t row = 0; row < pixel_samples; ++row)
			{
				for (size_t col = 0; col < pixel_samples; ++col)
				{
					const size_t pixelIndex = (row * pixel_samples) + col;
					const size_t pixelOffset = y[row] * static_cast<size_t>(displayBounds.cx) + x[col];

					offsets[pixelIndex] = { x[col], y[row] };
				}
			}
		}
	}

	// Re-initialize the previous colors for fades.
	if (_previousColors.empty())
	{
		_previousColors.resize(_parameters.totalLedCount, _parameters.minBrightnessColor);
	}
	else
	{
		for (auto& previousColor : _previousColors)
		{
			previousColor = _parameters.minBrightnessColor;
		}
	}

	_acquiredResources = true;
	_startTick = GetTickCount64();

	return true;
}

bool screen_samples::take_samples(serial_buffer& serial)
{
	if (!_acquiredResources)
	{
		return false;
	}

	// Take a screenshot for all of the devices that require a staging texture.
	for (auto& device : _displays)
	{
		if (!device.staging)
		{
			continue;
		}

		const auto& displayBounds = device.bounds;
		const UINT displayWidth = static_cast<UINT>(displayBounds.cx);
		const UINT displayHeight = static_cast<UINT>(displayBounds.cy);
		IDXGIResourcePtr resource;
		DXGI_OUTDUPL_FRAME_INFO info;
		ID3D11Texture2DPtr screenTexture;

		if (device.acquiredFrame)
		{
			device.duplication->ReleaseFrame();
			device.acquiredFrame = false;
		}

		HRESULT hr = device.duplication->AcquireNextFrame(_parameters.delay, &info, &resource);

		if (SUCCEEDED(hr))
		{
			device.acquiredFrame = true;
			screenTexture = resource;

			if (screenTexture)
			{
				device.context->CopyResource(device.staging, screenTexture);
			}

			screenTexture.Release();
			resource.Release();
		}
		else if (DXGI_ERROR_ACCESS_LOST == hr
			|| DXGI_ERROR_INVALID_CALL == hr)
		{
			// Recreate the duplication interface if this fails with with an expected error that invalidates
			// the duplication interface or that might allow us to switch to MapDesktopSurface.
			free_resources();
			return false;
		}
	}

	auto output = serial.begin();
	auto previousColor = _previousColors.begin();

	for (size_t i = 0; i < _displays.size(); ++i)
	{
		const auto& display = _parameters.displays[i];
		const auto& device = _displays[i];

		for (size_t j = 0; j < display.positions.size(); ++j)
		{
			const auto& offsets = _pixelOffsets[i][j];
			const auto& led = display.positions[j];

			D3D11_MAPPED_SUBRESOURCE stagingMap;
			DXGI_MAPPED_RECT desktopMap;
			const uint8_t* pixels = nullptr;
			size_t pitch = 0;

			if (device.staging)
			{
				if (FAILED(device.context->Map(device.staging, 0, D3D11_MAP_READ, 0, &stagingMap)))
				{
					continue;
				}

				pixels = reinterpret_cast<const uint8_t*>(stagingMap.pData);
				pitch = static_cast<size_t>(stagingMap.RowPitch);
			}
			else
			{
				HRESULT hr = device.duplication->MapDesktopSurface(&desktopMap);

				if (DXGI_ERROR_ACCESS_LOST == hr
					|| DXGI_ERROR_UNSUPPORTED == hr
					|| DXGI_ERROR_INVALID_CALL == hr)
				{
					// Recreate the duplication interface if this fails with with an expected error that invalidates
					// the duplication interface or requires that we switch to AcquireNextFrame.
					free_resources();
					return false;
				}
				else if (FAILED(hr))
				{
					continue;
				}

				pixels = reinterpret_cast<const uint8_t*>(desktopMap.pBits);
				pitch = static_cast<size_t>(desktopMap.Pitch);
			}

			// Get the average RGB values for the sampled pixels.
			constexpr double divisor = static_cast<double>(offset_array().size());
			double r = 0.0;
			double g = 0.0;
			double b = 0.0;

			for (auto offset : offsets)
			{
				const size_t byteOffset = (offset.y * pitch) + (offset.x * sizeof(DWORD));

				b += static_cast<double>(pixels[byteOffset]);
				g += static_cast<double>(pixels[byteOffset + 1]);
				r += static_cast<double>(pixels[byteOffset + 2]);
			}

			if (device.staging)
			{
				device.context->Unmap(device.staging, 0);
			}
			else
			{
				device.duplication->UnMapDesktopSurface();
			}

			r /= divisor;
			g /= divisor;
			b /= divisor;

			// Average in the previous color if fading is enabled.
			if (_parameters.fade > 0.0)
			{
				r = (r * _parameters.weight) + (static_cast<double>((*previousColor & 0xFF000000) >> 24) * _parameters.fade);
				g = (g * _parameters.weight) + (static_cast<double>((*previousColor & 0xFF0000) >> 16) * _parameters.fade);
				b = (b * _parameters.weight) + (static_cast<double>((*previousColor & 0xFF00) >> 8) * _parameters.fade);
			}

			const double minBrightness = static_cast<double>(_parameters.minBrightness);
			const double sum = r + g + b;

			// Boost pixels that fall below the minimum brightness.
			if (sum < minBrightness)
			{
				if (sum == 0.0)
				{
					// Spread equally to R, G, and B.
					const double value = minBrightness / 3.0;

					r = value;
					g = value;
					b = value;
				}
				else
				{
					// Spread the "brightness deficit" back into R, G, and B in proportion
					// to their individual contribition to that deficit.  Rather than simply
					// boosting all pixels at the low end, this allows deep (but saturated)
					// colors to stay saturated...they don't "pink out."
					const double deficit = minBrightness - sum;
					const double sum2 = 2.0 * sum;

					r += (deficit * (sum - r)) / sum2;
					g += (deficit * (sum - g)) / sum2;
					b += (deficit * (sum - b)) / sum2;
				}
			}

			const uint8_t ledR = static_cast<uint8_t>(r);
			const uint8_t ledG = static_cast<uint8_t>(g);
			const uint8_t ledB = static_cast<uint8_t>(b);

			*(previousColor++) = (ledR << 24) | (ledG << 16) | (ledB << 8) | 0xFF;

			// Write the _gamma corrected values to the serial data.
			*(output++) = _gamma.red(ledR);
			*(output++) = _gamma.green(ledG);
			*(output++) = _gamma.blue(ledB);
		}
	}

	++_frameCount;

	return true;
}

void screen_samples::free_resources()
{
	if (!_acquiredResources)
	{
		return;
	}

	for (auto& display : _displays)
	{
		if (display.acquiredFrame)
		{
			display.duplication->ReleaseFrame();
			display.acquiredFrame = false;
		}
	}

	_displays.clear();
	_pixelOffsets.clear();

	if (_startTick > 0)
	{
		_frameRate = static_cast<double>(_frameCount * 1000) / static_cast<double>(GetTickCount64() - _startTick);
		_frameCount = 0;
		_startTick = 0;

#ifdef _DEBUG
		std::wostringstream oss;

		oss << L"Frame Rate: " << _frameRate << std::endl;
		OutputDebugStringW(oss.str().c_str());
#endif
	}

	_acquiredResources = false;
}

bool screen_samples::empty() const
{
	return !_acquiredResources;
}

bool screen_samples::get_factory()
{
	if (!_factory)
	{
		CreateDXGIFactory1(_uuidof(IDXGIFactory1), reinterpret_cast<void**>(&_factory));
	}

	return _factory;
}
