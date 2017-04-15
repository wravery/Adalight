#pragma once

#include <vector>

#include "settings.h"

struct pixel_buffer
{
	typedef std::vector<uint8_t> vector_type;

	pixel_buffer& operator<<(uint32_t rgbaPixel);

	vector_type::const_pointer data() const;
	size_t size() const;

	void clear();

protected:
	pixel_buffer(vector_type&& header, size_t pixelCount, bool alphaChannel = false);

private:
	struct header
	{
		header(vector_type&& header);

		vector_type::const_pointer data() const;
		size_t size() const;

	private:
		vector_type _buffer;
	};

	const bool _alphaChannel = false;
	const header _offset;
	vector_type _buffer;

	vector_type::iterator _position;
};

struct serial_buffer
	: public pixel_buffer
{
	serial_buffer(const settings& parameters);

private:
	vector_type make_header(const settings& parameters);
};


struct opc_buffer
	: public pixel_buffer
{
	opc_buffer(const settings::opc_channel& channel);

private:
	vector_type make_header(const settings::opc_channel& channel);
};


struct bob_buffer
	: public pixel_buffer
{
	bob_buffer(const settings::opc_channel& channel);

private:
	vector_type make_header(const settings::opc_channel& channel);
};