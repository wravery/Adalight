#include "stdafx.h"
#include "opc_pool.h"

#include <iostream>

size_t opc_pool::_openCount = 0;

opc_pool::opc_pool(const settings& parameters)
	: _parameters(parameters)
{
}

bool opc_pool::open()
{
	if (!_openCount)
	{
		WSADATA data = {};
		const int result = WSAStartup(WINSOCK_VERSION, &data);

		if (result)
		{
			std::cerr << "Failed to initialize winsock2: "
				<< result << std::endl;
			return false;
		}
	}

	++_openCount;

	if (_connections.empty())
	{
		_connections.reserve(_parameters.servers.size());

		for (const auto& server : _parameters.servers)
		{
			_connections.push_back({ server });
		}
	}

	bool opened = false;

	for (auto& connection : _connections)
	{
		opened = connection.open() || opened;
	}

	return opened;
}

bool opc_pool::send(size_t server, const pixel_buffer& buffer)
{
	return server < _connections.size()
		&& _connections[server].send(buffer);
}

void opc_pool::close()
{
	for (auto& connection : _connections)
	{
		connection.close();
	}

	_connections.clear();

	if (!--_openCount)
	{
		WSACleanup();
	}
}


opc_pool::opc_connection::opc_connection(const settings::opc_server& server)
	: _server(server)
{
}

bool opc_pool::opc_connection::open()
{
	if (_socket != INVALID_SOCKET)
	{
		return true;
	}

	// Try to resolve the host and port names.
	if (!_addressInfo)
	{
		ADDRINFOW hints = {};

		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		const int result = GetAddrInfoW(_server.host.c_str(), _server.port.c_str(), &hints, &_addressInfo);

		if (result)
		{
			std::wcerr << L"Failed to resolve the server name \""
				<< _server.host << L":" << _server.port << L"\": "
				<< result << std::endl;
			return false;
		}
	}

	// Try each of the address results until we get a successful connection or run out of addresses.
	for (auto address = _addressInfo; address && _socket == INVALID_SOCKET; address = address->ai_next)
	{
		_socket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);

		if (_socket != INVALID_SOCKET)
		{
			const int result = connect(_socket, address->ai_addr, address->ai_addrlen);

			if (!result)
			{
				// We never need to receive data from the OPC server, so shutdown that direction immediately.
				shutdown(_socket, SD_RECEIVE);
			}
			else
			{
				// Try the next address.
				closesocket(_socket);
				_socket = INVALID_SOCKET;
			}
		}
	}

	if (_socket == INVALID_SOCKET)
	{
		std::wcerr << L"Failed to connect to \""
			<< _server.host << L":" << _server.port << L"\""
			<< std::endl;
		return false;
	}

	return true;
}

bool opc_pool::opc_connection::send(const pixel_buffer& buffer)
{
	if (_socket == INVALID_SOCKET)
	{
		return false;
	}

	const int result = ::send(_socket, reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()), 0);

	if (result == SOCKET_ERROR)
	{
		std::wcerr << L"Error sending data to \""
			<< _server.host << L":" << _server.port << L"\": "
			<< WSAGetLastError() << std::endl;
		close();
		return false;
	}

	return true;
}

void opc_pool::opc_connection::close()
{
	if (_socket != INVALID_SOCKET)
	{
		shutdown(_socket, SD_SEND);
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}

	if (_addressInfo)
	{
		FreeAddrInfoW(_addressInfo);
		_addressInfo = nullptr;
	}
}
