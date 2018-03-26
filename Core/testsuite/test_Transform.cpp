/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include <boost/test/unit_test.hpp>

#include "../vec3.h"
#include "../Transform.h"

namespace iseg {

namespace
{
	void makeTransform(Transform& tr)
	{
		tr.setIdentity();

		vec3 d0(0.1f, 0.9f, 0.1f);
		d0.normalize();
		vec3 d1(0.9f, -0.1f, 0.1f);
		d1.normalize();
		vec3 d2(d0 ^ d1);
		d2.normalize();
		tr.setRotation(d0, d1, d2);

		float offset[3] = { 3.4f, -10.f, 2.5f };
		tr.setOffset(offset);
	}
}

BOOST_AUTO_TEST_SUITE(iSeg_suite);
BOOST_AUTO_TEST_SUITE(Transform_suite);

BOOST_AUTO_TEST_CASE(vec3_api)
{
	vec3 x(1, 0, 0);
	vec3 y(0, 1, 0);
	vec3 z(x^y);
	BOOST_CHECK_CLOSE(z[2], 1.f, 1e-2f);
}

BOOST_AUTO_TEST_CASE(Transform_api)
{
	// from dc to transform
	Transform tr;
	float offset[3], dc[6];
	std::fill_n(offset, 3, 3.f);
	std::fill_n(dc, 6, 0.f);
	dc[0] = dc[4] = 1.f;

	tr.setTransform(offset, dc);
	for (int r=0; r<4; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			if (r == c)
			{
				BOOST_CHECK_CLOSE(tr[r][c], 1.f, 1e-2f);
			}
			else
			{
				BOOST_CHECK_CLOSE(tr[r][c], c==3 ? 3.f : 0.f, 1e-2f);
			}
		}
	}

	float disp[3];
	tr.getOffset(disp);
	for (int r = 0; r < 3; r++)
	{
		BOOST_CHECK_EQUAL(disp[r], offset[r]);
	}

	// padding
	{
		Transform tr;
		makeTransform(tr);
		Transform tr2(tr);

		// do padding
		float spacing[3] = { 1.f, 2.f, 3.f };
		{
			// negative padding
			int plo[3] = { -1,-2,-3 };
			tr2.paddingUpdateTransform(plo, spacing);

			// revert padding
			for (int k=0; k<3; k++)
			{
				plo[k] = -plo[k];
			}
			tr2.paddingUpdateTransform(plo, spacing);
		}

		// compare
		for (int r = 0; r < 4; r++)
		{
			for (int c = 0; c < 4; c++)
			{
				BOOST_CHECK_CLOSE(tr[r][c], tr2[r][c], 1e-2f);
			}
		}
	}

	// iSeg <-> S4L, iSeg <-> ITK ?

	// other operations...?
}

BOOST_AUTO_TEST_SUITE_END();
BOOST_AUTO_TEST_SUITE_END();

}
