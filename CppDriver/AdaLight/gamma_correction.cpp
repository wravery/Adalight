#include "stdafx.h"
#include "gamma_correction.h"

#include <cmath>

gamma_correction::gamma_correction()
{
	for (size_t i = 0; i <= _countof(_table); i++)
	{
		const double f = pow(static_cast<double>(i) / 255.0, 2.8);

		_table[i].r = static_cast<uint8_t>(f * 255.0);
		_table[i].g = static_cast<uint8_t>(f * 240.0);
		_table[i].b = static_cast<uint8_t>(f * 220.0);
	}
}

uint8_t gamma_correction::red(uint8_t r) const
{
	return _table[r].r;
}

uint8_t gamma_correction::green(uint8_t g) const
{
	return _table[g].g;
}

uint8_t gamma_correction::blue(uint8_t b) const
{
	return _table[b].b;
}
