#include "stdafx.h"
#include "opc_buffer.h"

opc_buffer::opc_buffer(const settings::opc_channel& channel)
	: _offset(channel)
{
	const size_t opcDataSize = 3 * channel.totalPixelCount;

	_buffer.resize(_offset.size() + opcDataSize, 0);
	memcpy_s(_buffer.data(), _buffer.size(), _offset.data(), _offset.size());
}

opc_buffer::vector_type::iterator opc_buffer::begin()
{
	return _buffer.begin() + _offset.size();
}

opc_buffer::vector_type::const_pointer opc_buffer::data() const
{
	return _buffer.data();
}

size_t opc_buffer::size() const
{
	return _buffer.size();
}

void opc_buffer::clear()
{
	memset(_buffer.data() + _offset.size(), 0, _buffer.size() - _offset.size());
}

opc_buffer::header::header(const settings::opc_channel& channel)
{
	const uint8_t channelByte = channel.channel;
	const uint8_t commandByte = 0;
	const size_t opcDataSize = 3 * channel.totalPixelCount;
	const uint8_t lengthHi = (opcDataSize & 0xFF00) >> 8;
	const uint8_t lengthLo = opcDataSize & 0xFF;

	_buffer = {
		channelByte,
		commandByte,
		lengthHi,
		lengthLo,
	};
}

opc_buffer::vector_type::const_pointer opc_buffer::header::data() const
{
	return _buffer.data();
}

size_t opc_buffer::header::size() const
{
	return _buffer.size();
}
