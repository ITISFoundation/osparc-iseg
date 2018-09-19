/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include <array>

namespace iseg {

class Color
{
public:
	union {
		std::array<float,3> rgb;
		struct { float r, g, b; };
	};

	Color() : r(0.f), g(0.f), b(0.f) {}
	Color(float R, float G, float B) : r(R), g(G), b(B) {}
	Color(unsigned char R, unsigned char G, unsigned char B) : r(R/255.f), g(G/255.f), b(B/255.f) {}

	static std::tuple<float, float, float> rgb2hsl(const Color& i)
	{
		const float fmax = std::max( i.r, std::max( i.g, i.b ) );
		const float fmin = std::min( i.r, std::min( i.g, i.b ) );

		float fh = 0.0f;
		float fs = 0.0f;
		float fl = ( fmax + fmin ) * 0.5f;
	
		if( fmax != fmin )
		{
			const float fdiff = fmax - fmin;
			if( fl > 0.5f )
			{
				fs = fdiff / ( 2.0f - fmax - fmin );
			}
			else
			{
				fs = fdiff / ( fmax + fmin );
			}
			if( fmax == i.r )
			{
				float tmpVal = 0.0f;
				if( i.g < i.b )
				{
					tmpVal = 6.0f;
				}
				fh = (i.g - i.b) / fdiff + tmpVal;
			}
			else if( fmax == i.g )
			{
				fh = (i.b - i.r) / fdiff + 2.0f;
			}
			else if( fmax == i.b )
			{
				fh = (i.r - i.g) / fdiff + 4.0f;
			}
			fh = fh / 6.0f;
		}
		return std::tie(fh, fs, fl);
	}

	static Color hsl2rgb(float fh, float fs, float fl)
	{
		float fr = fl;
		float fg = fl;
		float fb = fl;

		if( fs != 0.0f )
		{
			float q = 0.0f;
			if( fl < 0.5f )
			{
				q = fl * ( 1.0f + fs );
			}
			else
			{
				q = fl + fs - fl * fs;
			}
			float p = 2.0f * fl - q;
			fr = hue2rgb( p, q, fh + 1.0f / 3.0f );
			fg = hue2rgb( p, q ,fh );
			fb = hue2rgb( p, q, fh - 1.0f / 3.0f );
		}
		return Color(fr, fg, fb);
	}

private:

	static float hue2rgb(float p, float q, float t)
	{
		if( t < 0.0f )
			t += 1.0f;
		if( t > 1.0f )
			t -= 1.0f;
		if( t < 1.0f/6.0f )
			return p + ( q - p ) * 6.0f * t;
		if( t < 0.5f )
			return q;
		if( t < 2.0f/3.0f )
			return p + ( q - p ) * ( 2.0f / 3.0f - t ) * 6.0f;
		return p;
	}
};

} // namespace iseg
