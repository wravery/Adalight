#pragma once

#include <cstdint>

class gamma_correction
{
public:
	gamma_correction();

	uint8_t red(uint8_t r) const;
	uint8_t green(uint8_t g) const;
	uint8_t blue(uint8_t b) const;

private:
	struct levels
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};

	levels _table[256];
};
