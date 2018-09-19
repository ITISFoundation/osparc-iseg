#include "Color.h"

namespace iseg {

std::tuple<unsigned char, unsigned char, unsigned char> Color::toUChar() const
{
	return std::make_tuple(
			static_cast<unsigned char>(255.0f * r),
			static_cast<unsigned char>(255.0f * g),
			static_cast<unsigned char>(255.0f * b));
}

std::tuple<float, float, float> Color::toHSL() const
{
	const float fmax = std::max(r, std::max(g, b));
	const float fmin = std::min(r, std::min(g, b));

	float fh = 0.0f;
	float fs = 0.0f;
	float fl = (fmax + fmin) * 0.5f;

	if (fmax != fmin)
	{
		const float fdiff = fmax - fmin;
		if (fl > 0.5f)
		{
			fs = fdiff / (2.0f - fmax - fmin);
		}
		else
		{
			fs = fdiff / (fmax + fmin);
		}
		if (fmax == r)
		{
			float tmpVal = 0.0f;
			if (g < b)
			{
				tmpVal = 6.0f;
			}
			fh = (g - b) / fdiff + tmpVal;
		}
		else if (fmax == g)
		{
			fh = (b - r) / fdiff + 2.0f;
		}
		else if (fmax == b)
		{
			fh = (r - g) / fdiff + 4.0f;
		}
		fh = fh / 6.0f;
	}
	return std::tie(fh, fs, fl);
}

Color Color::fromHSL(float fh, float fs, float fl)
{
	float fr = fl;
	float fg = fl;
	float fb = fl;

	if (fs != 0.0f)
	{
		float q = 0.0f;
		if (fl < 0.5f)
		{
			q = fl * (1.0f + fs);
		}
		else
		{
			q = fl + fs - fl * fs;
		}
		float p = 2.0f * fl - q;
		fr = hue2rgb(p, q, fh + 1.0f / 3.0f);
		fg = hue2rgb(p, q, fh);
		fb = hue2rgb(p, q, fh - 1.0f / 3.0f);
	}
	return Color(fr, fg, fb);
}

float Color::hue2rgb(float p, float q, float t)
{
	if (t < 0.0f)
		t += 1.0f;
	if (t > 1.0f)
		t -= 1.0f;
	if (t < 1.0f / 6.0f)
		return p + (q - p) * 6.0f * t;
	if (t < 0.5f)
		return q;
	if (t < 2.0f / 3.0f)
		return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
	return p;
}

} // namespace iseg
