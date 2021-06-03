#include "BrushInteraction.h"

#include "Brush.h"
#include "addLine.h"

namespace iseg {

void BrushInteraction::Init(iseg::SlicesHandlerInterface* handler)
{
	if (handler == nullptr)
		throw std::runtime_error("null slice handler");

	m_SliceHandler = handler;
	m_Width = m_SliceHandler->Width();
	m_Height = m_SliceHandler->Height();
	auto spacing = m_SliceHandler->Spacing();
	m_Dx = spacing[0];
	m_Dy = spacing[1];
	m_CachedTissueLocks = m_SliceHandler->TissueLocks();
}

void BrushInteraction::OnMouseClicked(Point p)
{
	DataSelection data_selection;
	data_selection.sliceNr = m_SliceHandler->ActiveSlice();
	data_selection.work = m_BrushTarget;
	data_selection.tissues = !m_BrushTarget;
	m_BeginDatachange(data_selection);

	m_LastPt = p;

	if (m_BrushTarget)
	{
		DrawCircle(p);

		float* target = m_SliceHandler->TargetSlices().at(m_SliceHandler->ActiveSlice());
		brush(target, m_Width, m_Height, m_Dx, m_Dy, p, m_Radius, true, m_TargetValue, 0.f, [](float v) { return false; });
	}
	else
	{
		tissues_size_t* tissue = m_SliceHandler->TissueSlices(0).at(m_SliceHandler->ActiveSlice());
		brush(tissue, m_Width, m_Height, m_Dx, m_Dy, p, m_Radius, true, m_TissueValue, tissues_size_t(0), [this](tissues_size_t v) {
			return v < m_CachedTissueLocks.size() && m_CachedTissueLocks[v];
		});
	}

	m_EndDatachange(iseg::NoUndo);
}

void BrushInteraction::OnMouseMoved(Point p)
{
	std::vector<Point> vps;
	addLine(&vps, m_LastPt, p);
	m_LastPt = p;

	if (m_BrushTarget)
	{
		DrawCircle(p);

		float* target = m_SliceHandler->TargetSlices().at(m_SliceHandler->ActiveSlice());
		for (auto pi : vps)
		{
			brush(target, m_Width, m_Height, m_Dx, m_Dy, pi, m_Radius, true, m_TargetValue, 0.f, [](float v) { return false; });
		}
	}
	else
	{
		tissues_size_t* tissue = m_SliceHandler->TissueSlices(0).at(m_SliceHandler->ActiveSlice());
		for (auto pi : vps)
		{
			brush(tissue, m_Width, m_Height, m_Dx, m_Dy, pi, m_Radius, true, m_TissueValue, tissues_size_t(0), [this](tissues_size_t v) {
				return v < m_CachedTissueLocks.size() && m_CachedTissueLocks[v];
			});
		}
	}

	m_EndDatachange(iseg::NoUndo);
}

void BrushInteraction::OnMouseReleased(Point p)
{
	std::vector<Point> vps;
	addLine(&vps, m_LastPt, p);

	if (m_BrushTarget)
	{
		float* target = m_SliceHandler->TargetSlices().at(m_SliceHandler->ActiveSlice());
		for (auto pi : vps)
		{
			brush(target, m_Width, m_Height, m_Dx, m_Dy, pi, m_Radius, true, m_TargetValue, 0.f, [](float v) { return false; });
		}
	}
	else
	{
		tissues_size_t* tissue = m_SliceHandler->TissueSlices(0).at(m_SliceHandler->ActiveSlice());
		for (auto pi : vps)
		{
			brush(tissue, m_Width, m_Height, m_Dx, m_Dy, pi, m_Radius, true, m_TissueValue, tissues_size_t(0), [this](tissues_size_t v) {
				return v < m_CachedTissueLocks.size() && m_CachedTissueLocks[v];
			});
		}
	}

	std::vector<Point> vpdyn;
	m_VpdynChanged(&vpdyn);

	m_EndDatachange(iseg::EndUndo);
}

void BrushInteraction::DrawCircle(Point p)
{
	Point p1;
	std::vector<Point> vpdyn;

	float radius_corrected = (m_Dx > m_Dy) ? std::floor(m_Radius / m_Dx + 0.5f) * m_Dx : std::floor(m_Radius / m_Dy + 0.5f) * m_Dy;
	float const radius_corrected2 = radius_corrected * radius_corrected;
	int const xradius = static_cast<int>(std::ceil(radius_corrected / m_Dx));
	int const yradius = static_cast<int>(std::ceil(radius_corrected / m_Dy));
	for (p1.px = std::max(0, p.px - xradius); p1.px <= std::min((int)m_Width - 1, p.px + xradius); p1.px++)
	{
		for (p1.py = std::max(0, p.py - yradius); p1.py <= std::min((int)m_Height - 1, p.py + yradius); p1.py++)
		{
			if (std::pow(m_Dx * (p.px - p1.px), 2.f) + std::pow(m_Dy * (p.py - p1.py), 2.f) <= radius_corrected2)
			{
				vpdyn.push_back(p1);
			}
		}
	}

	m_VpdynChanged(&vpdyn);
	vpdyn.clear();
}

} // namespace iseg
