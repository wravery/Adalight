#include "stdafx.h"
#include "serial_buffer.h"

serial_buffer::serial_buffer(const settings& parameters)
	: _offset(parameters.totalLedCount)
{
	const size_t serialDataSize = 3 * parameters.totalLedCount;

	_buffer.resize(_offset.size() + serialDataSize, 0);
	memcpy_s(_buffer.data(), _buffer.size(), _offset.data(), _offset.size());
}

serial_buffer::vector_type::iterator serial_buffer::begin()
{
	return _buffer.begin() + _offset.size();
}

serial_buffer::vector_type::const_pointer serial_buffer::data() const
{
	return _buffer.data();
}

size_t serial_buffer::size() const
{
	return _buffer.size();
}

void serial_buffer::clear()
{
	memset(_buffer.data() + _offset.size(), 0, _buffer.size() - _offset.size());
}

serial_buffer::header::header(size_t totalLedCount)
{
	const uint8_t ledCountHi = ((totalLedCount - 1) & 0xFF00) >> 8;
	const uint8_t ledCountLo = (totalLedCount - 1) & 0xFF;
	const uint8_t ledCountChecksum = ledCountHi ^ ledCountLo ^ 0x55;
	_buffer = {
		'A', 'd', 'a',		// Magic word
		ledCountHi,			// LED count high byte
		ledCountLo,			// LED count low byte
		ledCountChecksum,	// Checksum
	};
}

serial_buffer::vector_type::const_pointer serial_buffer::header::data() const
{
	return _buffer.data();
}

size_t serial_buffer::header::size() const
{
	return _buffer.size();
}
