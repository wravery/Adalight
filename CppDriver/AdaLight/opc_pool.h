#pragma once

#include "settings.h"
#include "pixel_buffer.h"

#include <vector>

class opc_pool
{
public:
	opc_pool(const settings& parameters);

	bool open();
	bool send(size_t server, const pixel_buffer& buffer);
	void close();

private:
	class opc_connection
	{
	public:
		opc_connection(const settings::opc_server& server);

		bool open();
		bool send(const pixel_buffer& buffer);
		void close();

	private:
		const settings::opc_server& _server;
		ADDRINFOW* _addressInfo = nullptr;
		SOCKET _socket = INVALID_SOCKET;
	};

	const settings& _parameters;
	std::vector<opc_connection> _connections;

	static size_t _openCount;
};
