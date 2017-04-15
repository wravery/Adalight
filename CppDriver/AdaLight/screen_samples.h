#pragma once

#include <comdef.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <vector>
#include <array>

#include "settings.h"
#include "gamma_correction.h"
#include "pixel_buffer.h"

_COM_SMARTPTR_TYPEDEF(IDXGIFactory1, __uuidof(IDXGIFactory1));
_COM_SMARTPTR_TYPEDEF(IDXGIAdapter1, __uuidof(IDXGIAdapter1));
_COM_SMARTPTR_TYPEDEF(IDXGIOutput, __uuidof(IDXGIOutput));
_COM_SMARTPTR_TYPEDEF(IDXGIOutput1, __uuidof(IDXGIOutput1));
_COM_SMARTPTR_TYPEDEF(IDXGIOutputDuplication, __uuidof(IDXGIOutputDuplication));
_COM_SMARTPTR_TYPEDEF(IDXGIResource, __uuidof(IDXGIResource));
_COM_SMARTPTR_TYPEDEF(ID3D11Device, __uuidof(ID3D11Device));
_COM_SMARTPTR_TYPEDEF(ID3D11DeviceContext, __uuidof(ID3D11DeviceContext));
_COM_SMARTPTR_TYPEDEF(ID3D11Texture2D, __uuidof(ID3D11Texture2D));

class screen_samples
{
public:
	screen_samples(const settings& parameters, const gamma_correction& gamma);

	bool create_resources();
	bool take_samples();
	bool render_serial(serial_buffer& serial) const;
	bool render_channel(const settings::opc_channel, pixel_buffer& pixels) const;
	void free_resources();

	bool empty() const;

private:
	bool get_factory();

	struct display_resources
	{
		IDXGIAdapter1Ptr adapter;
		ID3D11DevicePtr device;
		ID3D11DeviceContextPtr context;
		IDXGIOutputDuplicationPtr duplication;
		ID3D11Texture2DPtr staging;
		bool acquiredFrame;
		SIZE bounds;
	};

	struct pixel_offset
	{
		size_t x;
		size_t y;
	};

	typedef std::array<pixel_offset, 256> offset_array;

	const settings& _parameters;
	const gamma_correction& _gamma;
	IDXGIFactory1Ptr _factory;
	std::vector<display_resources> _displays;
	std::vector<std::vector<offset_array>> _pixelOffsets;
	std::vector<uint32_t> _previousColors;
	bool _acquiredResources = false;
	size_t _frameCount = 0;
	ULONGLONG _startTick = 0;
	double _frameRate = 0.0;
};
