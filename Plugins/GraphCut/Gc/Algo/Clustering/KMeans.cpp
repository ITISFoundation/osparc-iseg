/*
    This file is part of Graph Cut (Gc) combinatorial optimization library.
    Copyright (C) 2008-2010 Centre for Biomedical Image Analysis (CBIA)
    Copyright (C) 2008-2010 Ondrej Danek

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published 
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Gc is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Graph Cut library. If not, see <http://www.gnu.org/licenses/>.
*/

/**
    @file
    KMeans clustering algorithm.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#include "../../Core.h"
#include "../../System/Collection/Array.h"
#include "../../System/Time/StopWatch.h"
#include "../../System/ArgumentException.h"
#include "../../System/InvalidOperationException.h"
#include "../../Math/ConvergenceException.h"
#include "KMeans.h"

namespace Gc
{
	namespace Algo
	{
        namespace Clustering
        {
            namespace KMeans
            {
                /******************************************************************************/

                template <Size N, class T>
                Size Lloyd(const System::Collection::Array<N,T> &im, Size k, 
                    const System::Collection::Array<1,T> &lambda, T conv_crit, 
                    Size max_iter, System::Collection::Array<1,T> &mean)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__);

                    if (im.IsEmpty())
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__, "Empty array.");
                    }

                    if (k < 2)
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__, "At least two means "
                            "required.");
                    }

                    if (k != lambda.Elements())
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__, "Invalid number of "
                            "weights supplied.");
                    }

                    mean.Resize(k);

                    // Compute min, max and avg intensity values
                    T dmin = im[0], dmax = dmin, davg = 0;

                    for (const T *pi = im.Begin(); pi != im.End(); ++pi)
                    {
                        davg += *pi;

                        if (*pi > dmax)
                        {
                            dmax = *pi;
                        }
                        else if (*pi < dmin)
                        {
                            dmin = *pi;
                        }
                    }

                    // Initial mean estimates
                    if (k == 2)
                    {
                        // A slightly more precise estimate in case of 2 means
                        davg /= T(im.Elements());
                        mean[0] = dmin + (davg - dmin) / T(2);
                        mean[1] = davg + (dmax - davg) / T(2);
                    }
                    else
                    {
                        for (Size i = 0; i < k; i++)
                        {
                            mean[i] = dmin + T(i + 1) * ((dmax - dmin) / T(k + 1));
                        }
                    }

                    // Minimization using Lloyd's algorithm
                    System::Collection::Array<1,T> oc(k);
                    System::Collection::Array<1,Size> cn(k);
                    Size iter = 0;

                    while (iter < max_iter)
                    {
                        iter++;

                        for (Size i = 0; i < k; i++)
                        {
                            oc[i] = mean[i];
                            mean[i] = 0;
                            cn[i] = 0;
                        }
                        
                        for (const T *pi = im.Begin(); pi != im.End(); ++pi)
                        {
                            Size bi = 0;
                            T br = lambda[0] * Math::Sqr(*pi - oc[0]);

                            for (Size j = 1; j < k; j++)
                            {
                                T nbr = lambda[j] * Math::Sqr(*pi - oc[j]);

                                if (nbr < br)
                                {
                                    br = nbr;
                                    bi = j;
                                }
                            }

                            cn[bi]++;
                            mean[bi] += *pi;
                        }

                        T cdelta = 0;
                        for (Size i = 0; i < k; i++)
                        {
                            if (!cn[i])
                            {
                                throw Math::ConvergenceException(__FUNCTION__, __LINE__, "Empty partition.");
                            }

                            mean[i] /= T(cn[i]);
                            cdelta += Math::Abs(mean[i] - oc[i]);
                        }

                        if (cdelta <= conv_crit)
                        {
                            break;
                        }
                    }

                    return iter;
                }

                /******************************************************************************/

                // Explicit instantiations
                /** @cond */
                template GC_DLL_EXPORT Size Lloyd(const System::Collection::Array<2,Float32> &im, 
                    Size k, const System::Collection::Array<1,Float32> &lambda, Float32 conv_crit, 
                    Size max_iter, System::Collection::Array<1,Float32> &mean);
                template GC_DLL_EXPORT Size Lloyd(const System::Collection::Array<2,Float64> &im, 
                    Size k, const System::Collection::Array<1,Float64> &lambda, Float64 conv_crit, 
                    Size max_iter, System::Collection::Array<1,Float64> &mean);
                template GC_DLL_EXPORT Size Lloyd(const System::Collection::Array<3,Float32> &im, 
                    Size k, const System::Collection::Array<1,Float32> &lambda, Float32 conv_crit, 
                    Size max_iter, System::Collection::Array<1,Float32> &mean);
                template GC_DLL_EXPORT Size Lloyd(const System::Collection::Array<3,Float64> &im, 
                    Size k, const System::Collection::Array<1,Float64> &lambda, Float64 conv_crit, 
                    Size max_iter, System::Collection::Array<1,Float64> &mean);
                /** @endcond */

                /******************************************************************************/

                template <Size N, class T>
                Size LloydMasked(const System::Collection::Array<N,T> &im, 
                    const System::Collection::IArrayMask<N> &mask, Size k, 
                    const System::Collection::Array<1,T> &lambda, T conv_crit, 
                    Size max_iter, System::Collection::Array<1,T> &mean)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__);

                    if (im.IsEmpty())
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__, "Empty array.");
                    }

                    if (k < 2)
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__, "At least two means "
                            "required.");
                    }

                    if (k != lambda.Elements())
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__, "Invalid number of "
                            "weights supplied.");
                    }

                    mean.Resize(k);

                    // Compute min, max and avg intensity values
                    const T *pi = im.Begin();
                    T dmin = *pi, dmax = *pi, davg = 0;
                    Size nvox = 0;

                    for (Size i = 0; i < im.Elements(); i++, pi++)
                    {
                        if (!mask.IsMasked(i))
                        {
                            nvox++;
                            davg += *pi;

                            if (*pi > dmax)
                            {
                                dmax = *pi;
                            }
                            else if (*pi < dmin)
                            {
                                dmin = *pi;
                            }
                        }
                    }

                    if (!nvox)
                    {
                        throw System::InvalidOperationException(__FUNCTION__, __LINE__, "All elements masked out.");
                    }

                    // Initial mean estimates
                    if (k == 2)
                    {
                        // A slightly more precise estimate in case of 2 means
                        davg /= T(nvox);
                        mean[0] = dmin + (davg - dmin) / T(2);
                        mean[1] = davg + (dmax - davg) / T(2);
                    }
                    else
                    {
                        for (Size i = 0; i < k; i++)
                        {
                            mean[i] = dmin + T(i + 1) * ((dmax - dmin) / T(k + 1));
                        }
                    }

                    // Minimization using Lloyd's algorithm
                    System::Collection::Array<1,T> oc(k);
                    System::Collection::Array<1,Size> cn(k);
                    Size iter = 0;

                    while (iter < max_iter)
                    {
                        iter++;

                        for (Size i = 0; i < k; i++)
                        {
                            oc[i] = mean[i];
                            mean[i] = 0;
                            cn[i] = 0;
                        }
                        
                        pi = im.Begin();         

                        for (Size i = 0; i < im.Elements(); i++, pi++)
                        {
                            if (!mask.IsMasked(i))
                            {
                                Size bi = 0;
                                T br = lambda[0] * Math::Sqr(*pi - oc[0]);

                                for (Size j = 1; j < k; j++)
                                {
                                    T nbr = lambda[j] * Math::Sqr(*pi - oc[j]);

                                    if (nbr < br)
                                    {
                                        br = nbr;
                                        bi = j;
                                    }
                                }

                                cn[bi]++;
                                mean[bi] += *pi;
                            }
                        }

                        T cdelta = 0;
                        for (Size i = 0; i < k; i++)
                        {
                            if (!cn[i])
                            {
                                throw Math::ConvergenceException(__FUNCTION__, __LINE__, "Empty partition.");
                            }

                            mean[i] /= T(cn[i]);
                            cdelta += Math::Abs(mean[i] - oc[i]);
                        }

                        if (cdelta <= conv_crit)
                        {
                            break;
                        }
                    }

                    return iter;
                }

                /******************************************************************************/

                // Explicit instantiations
                /** @cond */
                template GC_DLL_EXPORT Size LloydMasked(const System::Collection::Array<2,Float32> &im, 
                    const System::Collection::IArrayMask<2> &mask, Size k, 
                    const System::Collection::Array<1,Float32> &lambda, Float32 conv_crit, 
                    Size max_iter, System::Collection::Array<1,Float32> &mean);
                template GC_DLL_EXPORT Size LloydMasked(const System::Collection::Array<2,Float64> &im, 
                    const System::Collection::IArrayMask<2> &mask, Size k, 
                    const System::Collection::Array<1,Float64> &lambda, Float64 conv_crit, 
                    Size max_iter, System::Collection::Array<1,Float64> &mean);
                template GC_DLL_EXPORT Size LloydMasked(const System::Collection::Array<3,Float32> &im, 
                    const System::Collection::IArrayMask<3> &mask, Size k, 
                    const System::Collection::Array<1,Float32> &lambda, Float32 conv_crit, 
                    Size max_iter, System::Collection::Array<1,Float32> &mean);
                template GC_DLL_EXPORT Size LloydMasked(const System::Collection::Array<3,Float64> &im, 
                    const System::Collection::IArrayMask<3> &mask, Size k, 
                    const System::Collection::Array<1,Float64> &lambda, Float64 conv_crit, 
                    Size max_iter, System::Collection::Array<1,Float64> &mean);
                /** @endcond */

                /******************************************************************************/
            }
        }
	}
}
