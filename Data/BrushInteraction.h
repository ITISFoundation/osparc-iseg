/*
* Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
*
* This file is part of iSEG
* (see https://github.com/ITISFoundation/osparc-iseg).
*
* This software is released under the MIT License.
*  https://opensource.org/licenses/MIT
*/
#pragma once

#include "iSegData.h"

#include "DataSelection.h"
#include "Point.h"
#include "SlicesHandlerInterface.h"

#include <boost/function.hpp>

namespace iseg {

class ISEG_DATA_API BrushInteraction
{
public:
	BrushInteraction(SlicesHandlerInterface* handler, const boost::function<void(DataSelection)>& begin, const boost::function<void(eEndUndoAction)>& end, const boost::function<void(std::vector<Point>*)>& vpdynchanged)
			: m_BeginDatachange(begin), m_EndDatachange(end), m_VpdynChanged(vpdynchanged)
	{
		Init(handler);
	}

	/// throws std::runtime_error if handler is invalid
	void Init(SlicesHandlerInterface* handler);

	void SetRadius(float radius) { m_Radius = radius; }
	void SetBrushTarget(bool on) { m_BrushTarget = on; }
	void SetTissueValue(tissues_size_t v) { m_TissueValue = v; }
	void SetTargetValue(float v) { m_TargetValue = v; }

	void OnMouseClicked(Point p);

	void OnMouseMoved(Point p);

	void OnMouseReleased(Point p);

	void DrawCircle(Point p);

private:
	boost::function<void(DataSelection)> m_BeginDatachange;
	boost::function<void(eEndUndoAction)> m_EndDatachange;
	boost::function<void(std::vector<Point>*)> m_VpdynChanged;

	SlicesHandlerInterface* m_SliceHandler;
	std::vector<bool> m_CachedTissueLocks;
	unsigned m_Width;
	unsigned m_Height;
	float m_Dx;
	float m_Dy;
	float m_Radius = 1.0f;
	bool m_BrushTarget = true;
	float m_TargetValue = 255.f;
	tissues_size_t m_TissueValue = 1;
	Point m_LastPt;
};

} // namespace iseg
