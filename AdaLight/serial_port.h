#pragma once

#include "settings.h"
#include "serial_buffer.h"

class serial_port
{
public:
	serial_port(const settings& parameters);

	bool open();
	bool send(const serial_buffer& buffer);
	void close();

private:
	HANDLE get_handle(uint8_t portNumber, bool testCookie);

	const settings& _parameters;
	HANDLE _portHandle = INVALID_HANDLE_VALUE;
	uint8_t _portNumber = 0;
};
