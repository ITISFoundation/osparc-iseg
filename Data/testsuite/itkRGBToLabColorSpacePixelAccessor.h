/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkRGBToLabColorSpacePixelAccessor.h,v $
  Language:  C++
  Date:      $Date: 2009-03-03 15:08:46 $
  Version:   $Revision: 1.4 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkRGBToLabColorSpacePixelAccessor_h
#define __itkRGBToLabColorSpacePixelAccessor_h

#include "itkNumericTraits.h"
#include "itkRGBPixel.h"
#include "itkVector.h"
#include "vnl/vnl_math.h"

namespace itk {
namespace Accessor {
/**
 * \class RGBToLabColorSpacePixelAccessor
 * \brief Give access to a RGBPixel as if it were in Lab (CIE L*a*b*) Color Space as a Vector type.
 *
 * This class is intended to be used as parameter of 
 * an ImageAdaptor to make an RGBPixel image appear as being
 * an image of Vector pixel type in Lab Color Space.
 *
 * \sa ImageAdaptor
 * \ingroup ImageAdaptors
 *
 */

template<class TInput, class TOutput>
class ITK_EXPORT RGBToLabColorSpacePixelAccessor
{
public:
	/** Standard class type aliases. */
	using Self = RGBToLabColorSpacePixelAccessor;

	/** External types. It defines the external aspect
	* that this class will exhibit */
	using ExternalType = Vector<TOutput, 3>;

	/** Internal types. It defines the internal real
	* representation of data */
	using InternalType = RGBPixel<TInput>;

	/** Write access to the RGBToLabColorSpace component */
	inline void Set(InternalType& output, const ExternalType& input) const
	{
		// Normalize RGB values.
		double r = (double)input[0] / (double)NumericTraits<TInput>::max();
		double g = (double)input[1] / (double)NumericTraits<TInput>::max();
		double b = (double)input[2] / (double)NumericTraits<TInput>::max();

		if (r > 0.04045)
			r = vcl_pow(((r + 0.055) / 1.055), 2.4);
		else
			r = r / 12.92;

		if (g > 0.04045)
			g = vcl_pow(((g + 0.055) / 1.055), 2.4);
		else
			g = g / 12.92;

		if (b > 0.04045)
			b = vcl_pow(((b + 0.055) / 1.055), 2.4);
		else
			b = b / 12.92;

		r = r * 100.0;
		g = g * 100.0;
		b = b * 100.0;

		double X = r * 0.4124 + g * 0.3576 + b * 0.1805;
		double Y = r * 0.2126 + g * 0.7152 + b * 0.0722;
		double Z = r * 0.0193 + g * 0.1192 + b * 0.9505;

		double X2, Y2, Z2;
		X = X / 95.047;
		Y = Y / 100.0;
		Z = Z / 108.883;

		if (X > 0.008856)
			X2 = vcl_exp(vcl_log(X) / 3.0);
		else
			X2 = (7.787 * X) + (16.0 / 116.0);
		if (Y > 0.008856)
			Y2 = vcl_exp(vcl_log(Y) / 3.0);
		else
			Y2 = (7.787 * Y) + (16.0 / 116.0);
		if (Z > 0.008856)
			Z2 = vcl_exp(vcl_log(Z) / 3.0);
		else
			Z2 = (7.787 * Z) + (16.0 / 116.0);

		output[0] = static_cast<TInput>((116.0 * Y2) - 16.0); // L
		output[1] = static_cast<TInput>(500.0 * (X2 - Y2));		// a
		output[2] = static_cast<TInput>(200.0 * (Y2 - Z2));		// b
	}

	/** Read access to the RGBToLabColorSpace component */
	inline ExternalType Get(const InternalType& input) const
	{
		// Normalize RGB values.
		double r = (double)input[0] / (double)NumericTraits<TInput>::max();
		double g = (double)input[1] / (double)NumericTraits<TInput>::max();
		double b = (double)input[2] / (double)NumericTraits<TInput>::max();

		if (r > 0.04045)
			r = vcl_pow(((r + 0.055) / 1.055), 2.4);
		else
			r = r / 12.92;

		if (g > 0.04045)
			g = vcl_pow(((g + 0.055) / 1.055), 2.4);
		else
			g = g / 12.92;

		if (b > 0.04045)
			b = vcl_pow(((b + 0.055) / 1.055), 2.4);
		else
			b = b / 12.92;

		r = r * 100.0;
		g = g * 100.0;
		b = b * 100.0;

		double X = r * 0.4124 + g * 0.3576 + b * 0.1805;
		double Y = r * 0.2126 + g * 0.7152 + b * 0.0722;
		double Z = r * 0.0193 + g * 0.1192 + b * 0.9505;

		X = X / 95.047;
		Y = Y / 100.0;
		Z = Z / 108.883;

		double X2, Y2, Z2;
		if (X > 0.008856)
			X2 = vcl_exp(vcl_log(X) / 3.0);
		else
			X2 = (7.787 * X) + (16.0 / 116.0);
		if (Y > 0.008856)
			Y2 = vcl_exp(vcl_log(Y) / 3.0);
		else
			Y2 = (7.787 * Y) + (16.0 / 116.0);
		if (Z > 0.008856)
			Z2 = vcl_exp(vcl_log(Z) / 3.0);
		else
			Z2 = (7.787 * Z) + (16.0 / 116.0);

		ExternalType output;
		output[0] = static_cast<TOutput>((116.0 * Y2) - 16.0); // L
		output[1] = static_cast<TOutput>(500.0 * (X2 - Y2));	 // a
		output[2] = static_cast<TOutput>(200.0 * (Y2 - Z2));	 // b

		return output;
	}

private:
};

}
} // namespace itk::Accessor

#endif