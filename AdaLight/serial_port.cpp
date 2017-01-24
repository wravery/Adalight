#include "stdafx.h"
#include "serial_port.h"

#include <string>
#include <sstream>

serial_port::serial_port(const settings& parameters)
	: _parameters(parameters)
{
}

bool serial_port::open()
{
	if (INVALID_HANDLE_VALUE == _portHandle)
	{
		// TODO: Use overlapped I/O to test the cookie on multiple ports simultaneously.
		if (0 == _portNumber)
		{
			// [1, 255]: Terminate the loop when the byte overflows from 255 back to 0.
			for (uint8_t i = 1; i != 0; ++i)
			{
				_portHandle = get_handle(i, true);

				if (INVALID_HANDLE_VALUE != _portHandle)
				{
					_portNumber = i;
					break;
				}
			}
		}
		else
		{
			// Once we find the right port we can just open it directly.
			_portHandle = get_handle(_portNumber, false);
		}
	}

	return INVALID_HANDLE_VALUE != _portHandle;
}

bool serial_port::send(const serial_buffer& buffer)
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

HANDLE serial_port::get_handle(uint8_t portNumber, bool testCookie)
{
	std::wostringstream oss;

	oss << L"COM" << static_cast<int>(portNumber);

	std::wstring portName(oss.str());
	HANDLE portHandle = CreateFileW(portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	DCB configuration = { sizeof(configuration) };

	if (INVALID_HANDLE_VALUE != portHandle
		&& GetCommState(portHandle, &configuration))
	{
		COMMTIMEOUTS timeouts = {
			0,						// ReadIntervalTimeout
			0,						// ReadTotalTimeoutMultiplier
			_parameters.timeout,	// ReadTotalTimeoutConstant
			0,						// WriteTotalTimeoutMultiplier
			_parameters.delay		// WriteTotalTimeoutConstant
		};
		uint8_t cookie[] = { 'A', 'd', 'a', '\n' };
		uint8_t buffer[_countof(cookie)] = {};
		DWORD cb = 0;
		DCB original = configuration;

		configuration.BaudRate = CBR_115200;
		configuration.ByteSize = 8;
		configuration.StopBits = ONESTOPBIT;
		configuration.Parity = NOPARITY;

		// Configure the port.
		if (!SetCommState(portHandle, &configuration)
			|| !SetCommTimeouts(portHandle, &timeouts)
			|| (testCookie
				&& (!ReadFile(portHandle, buffer, sizeof(buffer), &cb, nullptr)
					|| sizeof(buffer) != cb
					|| 0 != memcmp(cookie, buffer, sizeof(cb)))))
		{
			SetCommState(portHandle, &original);
			CloseHandle(portHandle);
			portHandle = INVALID_HANDLE_VALUE;
		}
	}

	return portHandle;
}
