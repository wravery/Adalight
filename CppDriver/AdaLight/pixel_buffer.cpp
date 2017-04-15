#include "stdafx.h"
#include "pixel_buffer.h"

pixel_buffer::pixel_buffer(vector_type&& header, size_t pixelCount, bool alphaChannel /*= false*/)
	: _alphaChannel(alphaChannel)
	, _offset(std::move(header))
	, _buffer(_offset.size() + (_alphaChannel ? 4 : 3) * pixelCount)
{
	memcpy_s(_buffer.data(), _buffer.size(), _offset.data(), _offset.size());
	clear();
}

pixel_buffer& pixel_buffer::operator<<(uint32_t rgbaPixel)
{
	*(_position++) = static_cast<uint8_t>((rgbaPixel >> 24) & 0xFF);
	*(_position++) = static_cast<uint8_t>((rgbaPixel >> 16) & 0xFF);
	*(_position++) = static_cast<uint8_t>((rgbaPixel >> 8) & 0xFF);

	if (_alphaChannel)
	{
		*(_position++) = static_cast<uint8_t>(rgbaPixel & 0xFF);
	}

	return *this;
}

pixel_buffer::vector_type::const_pointer pixel_buffer::data() const
{
	return _buffer.data();
}

size_t pixel_buffer::size() const
{
	return _buffer.size();
}

void pixel_buffer::clear()
{
	memset(_buffer.data() + _offset.size(), 0, _buffer.size() - _offset.size());
	_position = _buffer.begin() + _offset.size();
}

pixel_buffer::header::header(vector_type&& header)
	: _buffer(std::move(header))
{
}

pixel_buffer::vector_type::const_pointer pixel_buffer::header::data() const
{
	return _buffer.data();
}

size_t pixel_buffer::header::size() const
{
	return _buffer.size();
}


serial_buffer::serial_buffer(const settings& parameters)
	: pixel_buffer(make_header(parameters), parameters.totalLedCount)
{
}

pixel_buffer::vector_type serial_buffer::make_header(const settings& parameters)
{
	const uint8_t ledCountHi = ((parameters.totalLedCount - 1) & 0xFF00) >> 8;
	const uint8_t ledCountLo = (parameters.totalLedCount - 1) & 0xFF;
	const uint8_t ledCountChecksum = ledCountHi ^ ledCountLo ^ 0x55;

	return {
		'A', 'd', 'a',		// Magic word
		ledCountHi,			// LED count high byte
		ledCountLo,			// LED count low byte
		ledCountChecksum,	// Checksum
	};
}


opc_buffer::opc_buffer(const settings::opc_channel& channel)
	: pixel_buffer(make_header(channel), channel.totalPixelCount)
{
}

pixel_buffer::vector_type opc_buffer::make_header(const settings::opc_channel& channel)
{
	const uint8_t channelByte = channel.channel;
	const uint8_t commandByte = 0;
	const uint16_t opcDataSize = static_cast<uint16_t>(3 * channel.totalPixelCount);
	const uint8_t lengthHi = (opcDataSize & 0xFF00) >> 8;
	const uint8_t lengthLo = opcDataSize & 0xFF;

	return {
		channelByte,	// Channel
		commandByte,	// Command
		lengthHi,		// Byte count high byte
		lengthLo,		// Byte count low byte
	};
}


bob_buffer::bob_buffer(const settings::opc_channel& channel)
	: pixel_buffer(make_header(channel), channel.totalPixelCount, true)
{
}

pixel_buffer::vector_type bob_buffer::make_header(const settings::opc_channel& channel)
{
	const uint8_t channelByte = channel.channel;
	const uint8_t commandByte = 255;
	const uint16_t opcDataSize = static_cast<uint16_t>(2 + 4 * channel.totalPixelCount);
	const uint8_t lengthHi = (opcDataSize & 0xFF00) >> 8;
	const uint8_t lengthLo = opcDataSize & 0xFF;
	constexpr uint16_t systemId = 0xB0B;
	constexpr uint8_t systemIdHi = (systemId & 0xFF00) >> 8;
	constexpr uint8_t systemIdLo = systemId & 0xFF;

	return {
		channelByte,	// Channel
		commandByte,	// Command
		lengthHi,		// Byte count high byte
		lengthLo,		// Byte count low byte
		systemIdHi,		// System ID high byte
		systemIdLo,		// System ID low byte
	};
}
