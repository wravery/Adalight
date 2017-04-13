#pragma once

#include <vector>

#include "settings.h"

struct opc_buffer
{
	opc_buffer(const settings::opc_channel& channel);

	typedef std::vector<uint8_t> vector_type;

	vector_type::iterator begin();

	vector_type::const_pointer data() const;
	size_t size() const;

	void clear();

private:
	struct header
	{
		header(const settings::opc_channel& channel);

		vector_type::const_pointer data() const;
		size_t size() const;

	private:
		vector_type _buffer;
	};

	const header _offset;
	vector_type _buffer;
};
