#pragma once

#include <tuple>

#include "settings.h"
#include "pixel_buffer.h"

class serial_port
{
public:
	serial_port(const settings& parameters);

	bool open();
	bool send(const pixel_buffer& buffer);
	void close();

private:
	std::pair<HANDLE, DCB> get_port(uint8_t portNumber, bool readTest);

	const settings& _parameters;
	HANDLE _portHandle = INVALID_HANDLE_VALUE;
	uint8_t _portNumber = 0;
};
