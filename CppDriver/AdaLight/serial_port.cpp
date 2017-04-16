#include "stdafx.h"
#include "serial_port.h"

#include <array>
#include <memory>
#include <list>
#include <string>
#include <sstream>

constexpr uint8_t cookie[] = { 'A', 'd', 'a', '\n' };

struct port_resources
{
	~port_resources();

	HANDLE portHandle = INVALID_HANDLE_VALUE;
	DCB configuration = { sizeof(configuration) };
	uint8_t portNumber = 0;
	HANDLE waitHandle = INVALID_HANDLE_VALUE;
	std::array<uint8_t, _countof(cookie)> buffer;
	size_t cb = 0;
	OVERLAPPED overlapped = {};
};

port_resources::~port_resources()
{
	if (INVALID_HANDLE_VALUE != portHandle)
	{
		CancelIo(portHandle);
		SetCommState(portHandle, &configuration);
		CloseHandle(portHandle);
	}

	if (INVALID_HANDLE_VALUE != waitHandle)
	{
		CloseHandle(waitHandle);
	}
}

serial_port::serial_port(const settings& parameters)
	: _parameters(parameters)
{
}

bool serial_port::open()
{
	if (INVALID_HANDLE_VALUE == _portHandle)
	{
		if (0 == _portNumber)
		{
			constexpr uint8_t maxPortNumber = 255;
			std::list<std::unique_ptr<port_resources>> pendingPorts;
			size_t portCount = 0;
			DWORD cb = 0;

			// Try to open every possible port from COM1 - COM255
			for (uint8_t i = 0; i < maxPortNumber; ++i)
			{
				// See if any pending asynch reads have finished.
				if (!pendingPorts.empty())
				{
					auto itr = pendingPorts.cbegin();

					while (itr != pendingPorts.cend())
					{
						if (GetOverlappedResult((*itr)->portHandle, &(*itr)->overlapped, &cb, false))
						{
							if (sizeof(cookie) == cb
								&& 0 == memcmp(cookie, (*itr)->buffer.data(), sizeof(cookie)))
							{
								// We found a match!
								_portNumber = (*itr)->portNumber;
								break;
							}
						}
						else if (ERROR_IO_INCOMPLETE == GetLastError())
						{
							// Still pending, go on to the next port.
							++itr;
							continue;
						}

						// Any mismatched data or other error means we can't read from the port at all.
						itr = pendingPorts.erase(itr);
					}

					if (0 != _portNumber)
					{
						// If we found a match, we can skip waiting for the rest of the I/O to complete below.
						pendingPorts.clear();
						break;
					}
				}

				// Try opening the next port.
				auto port = std::make_unique<port_resources>();

				port->portNumber = i + 1;
				std::tie(port->portHandle, port->configuration) = get_port(port->portNumber, true);
				if (INVALID_HANDLE_VALUE == port->portHandle)
				{
					continue;
				}

				// Start an overlapped I/O call to look for the cookie sent from the Arduino.
				port->waitHandle = CreateEventW(nullptr, true, false, nullptr);
				port->overlapped.hEvent = port->waitHandle;
				if (!ReadFile(port->portHandle, reinterpret_cast<void*>(port->buffer.data()), sizeof(port->buffer), nullptr, &port->overlapped)
					&& ERROR_IO_PENDING != GetLastError())
				{
					// Any other error means we can't read from the port at all.
					continue;
				}

				// Add the new port to the list for the next iteration.
				pendingPorts.push_back(std::move(port));
			}

			// Finish waiting for any outstanding I/O.
			for (const auto& port : pendingPorts)
			{
				if (GetOverlappedResult(port->portHandle, &port->overlapped, &cb, true)
					&& sizeof(cookie) == cb
					&& 0 == memcmp(cookie, port->buffer.data(), sizeof(cookie)))
				{
					// We found a match!
					_portNumber = port->portNumber;
					break;
				}
			}
		}

		if (0 != _portNumber)
		{
			// Once we find the right port we can just open it directly.
			std::tie(_portHandle, std::ignore) = get_port(_portNumber, false);
		}
	}

	return INVALID_HANDLE_VALUE != _portHandle;
}

bool serial_port::send(const pixel_buffer& buffer)
{
	if (INVALID_HANDLE_VALUE == _portHandle)
	{
		return false;
	}

	DWORD cbWritten = 0;

	if (!WriteFile(_portHandle, reinterpret_cast<const void*>(buffer.data()), buffer.size(), &cbWritten, nullptr)
		|| buffer.size() != static_cast<size_t>(cbWritten))
	{
		close();
		return false;
	}

	return true;
}

void serial_port::close()
{
	if (INVALID_HANDLE_VALUE != _portHandle)
	{
		CloseHandle(_portHandle);
		_portHandle = INVALID_HANDLE_VALUE;
	}
}

std::pair<HANDLE, DCB> serial_port::get_port(uint8_t portNumber, bool readTest)
{
	std::wostringstream oss;

	oss << L"COM" << static_cast<int>(portNumber);

	std::wstring portName(oss.str());
	const DWORD desiredAccess = readTest ? GENERIC_READ : GENERIC_WRITE;
	const DWORD flagsAndAttributes = readTest ? FILE_FLAG_OVERLAPPED : FILE_ATTRIBUTE_NORMAL;
	HANDLE portHandle = CreateFileW(portName.c_str(), desiredAccess, 0, nullptr, OPEN_EXISTING, flagsAndAttributes, NULL);
	DCB configuration = { sizeof(configuration) };

	if (INVALID_HANDLE_VALUE != portHandle)
	{
		if (GetCommState(portHandle, &configuration))
		{
			DCB reconfigured = configuration;

			reconfigured.BaudRate = CBR_115200;
			reconfigured.ByteSize = 8;
			reconfigured.StopBits = ONESTOPBIT;
			reconfigured.Parity = NOPARITY;

			COMMTIMEOUTS timeouts = {
				0,						// ReadIntervalTimeout
				0,						// ReadTotalTimeoutMultiplier
				_parameters.timeout,	// ReadTotalTimeoutConstant
				0,						// WriteTotalTimeoutMultiplier
				_parameters.delay		// WriteTotalTimeoutConstant
			};

			// Configure the port.
			if (SetCommState(portHandle, &reconfigured)
				&& SetCommTimeouts(portHandle, &timeouts))
			{
				return { portHandle, configuration };
			}

			SetCommState(portHandle, &configuration);
		}

		CloseHandle(portHandle);
		portHandle = INVALID_HANDLE_VALUE;
	}

	return { portHandle, configuration };
}
