#include "BrushInteraction.h"

#include "Brush.h"
#include "addLine.h"

namespace iseg {

void BrushInteraction::init(iseg::SliceHandlerInterface* handler)
{
	if (handler == nullptr)
		throw std::runtime_error("null slice handler");

	_slice_handler = handler;
	_width = _slice_handler->width();
	_height = _slice_handler->height();
	auto spacing = _slice_handler->spacing();
	_dx = spacing[0];
	_dy = spacing[1];
	_cached_tissue_locks = _slice_handler->tissue_locks();
}

void BrushInteraction::on_mouse_clicked(Point p)
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = _slice_handler->active_slice();
	dataSelection.work = _brush_target;
	dataSelection.tissues = !_brush_target;
	begin_datachange(dataSelection);

	_last_pt = p;

	if (_brush_target)
	{
		draw_circle(p);

		float* target = _slice_handler->target_slices().at(_slice_handler->active_slice());
		brush(target, _width, _height, _dx, _dy, p, _radius, true, _target_value, 0.f, [](float v) { return false; });
	}
	else
	{
		tissues_size_t* tissue = _slice_handler->tissue_slices(0).at(_slice_handler->active_slice());
		brush(tissue, _width, _height, _dx, _dy, p, _radius, true, _tissue_value, tissues_size_t(0),
				[this](tissues_size_t v) {
					return v < _cached_tissue_locks.size() && _cached_tissue_locks[v];
				});
	}

	end_datachange(iseg::NoUndo);
}

void BrushInteraction::on_mouse_moved(Point p)
{
	std::vector<Point> vps;
	addLine(&vps, _last_pt, p);
	_last_pt = p;

	if (_brush_target)
	{
		draw_circle(p);

		float* target = _slice_handler->target_slices().at(_slice_handler->active_slice());
		for (auto pi : vps)
		{
			brush(target, _width, _height, _dx, _dy, pi, _radius, true, _target_value, 0.f, [](float v) { return false; });
		}
	}
	else
	{
		tissues_size_t* tissue = _slice_handler->tissue_slices(0).at(_slice_handler->active_slice());
		for (auto pi : vps)
		{
			brush(tissue, _width, _height, _dx, _dy, pi, _radius, true, _tissue_value, tissues_size_t(0),
					[this](tissues_size_t v) {
						return v < _cached_tissue_locks.size() && _cached_tissue_locks[v];
					});
		}
	}

	end_datachange(iseg::NoUndo);
}

void BrushInteraction::on_mouse_released(Point p)
{
	std::vector<Point> vps;
	addLine(&vps, _last_pt, p);

	if (_brush_target)
	{
		float* target = _slice_handler->target_slices().at(_slice_handler->active_slice());
		for (auto pi : vps)
		{
			brush(target, _width, _height, _dx, _dy, pi, _radius, true, _target_value, 0.f, [](float v) { return false; });
		}
	}
	else
	{
		tissues_size_t* tissue = _slice_handler->tissue_slices(0).at(_slice_handler->active_slice());
		for (auto pi : vps)
		{
			brush(tissue, _width, _height, _dx, _dy, pi, _radius, true, _tissue_value, tissues_size_t(0),
					[this](tissues_size_t v) {
						return v < _cached_tissue_locks.size() && _cached_tissue_locks[v];
					});
		}
	}

	std::vector<Point> vpdyn;
	vpdyn_changed(&vpdyn);

	end_datachange(iseg::EndUndo);
}

void BrushInteraction::draw_circle(Point p)
{
	Point p1;
	std::vector<Point> vpdyn;

	float radius_corrected = (_dx > _dy) ? std::floor(_radius / _dx + 0.5f) * _dx : std::floor(_radius / _dy + 0.5f) * _dy;
	float const radius_corrected2 = radius_corrected * radius_corrected;
	int const xradius = static_cast<int>(std::ceil(radius_corrected / _dx));
	int const yradius = static_cast<int>(std::ceil(radius_corrected / _dy));
	for (p1.px = std::max(0, p.px - xradius);
			 p1.px <= std::min((int)_width - 1, p.px + xradius);
			 p1.px++)
	{
		for (p1.py = std::max(0, p.py - yradius);
				 p1.py <= std::min((int)_height - 1, p.py + yradius);
				 p1.py++)
		{
			if (std::pow(_dx * (p.px - p1.px), 2.f) + std::pow(_dy * (p.py - p1.py), 2.f) <= radius_corrected2)
			{
				vpdyn.push_back(p1);
			}
		}
	}

	vpdyn_changed(&vpdyn);
	vpdyn.clear();
}

} // namespace iseg
