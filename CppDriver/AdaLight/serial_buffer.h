#pragma once

#include <vector>

#include "settings.h"

struct serial_buffer
{
	serial_buffer(const settings& parameters);

	typedef std::vector<uint8_t> vector_type;

	vector_type::iterator begin();

	vector_type::const_pointer data() const;
	size_t size() const;

	void clear();

private:
	struct header
	{
		header(size_t totalLedCount);

		vector_type::const_pointer data() const;
		size_t size() const;

	private:
		vector_type _buffer;
	};

	const header _offset;
	vector_type _buffer;
};
