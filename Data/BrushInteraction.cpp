#include "BrushInteraction.h"

#include "addLine.h"
#include "Brush.h"

namespace iseg {

void BrushInteraction::init(iseg::SliceHandlerInterface* handler)
{
	if (_slice_handler = handler)
	{
		_width = _slice_handler->width();
		_height = _slice_handler->height();
		auto spacing = _slice_handler->spacing();
		_dx = spacing[0];
		_dy = spacing[1];
	}
}

void BrushInteraction::on_mouse_clicked(Point p)
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = _slice_handler->active_slice();
	dataSelection.work = true;
	begin_datachange(dataSelection);

	_last_pt = p;
	float* target = slice();
	brush(target, _width, _height, _dx, _dy, p, _radius, true, _new_value, _old_value, [](float v) { return false; });

	end_datachange(iseg::NoUndo);
}

void BrushInteraction::on_mouse_moved(Point p)
{
	std::vector<Point> vps;
	addLine(&vps, _last_pt, p);
	_last_pt = p;

	float* target = slice();
	for (auto pi : vps)
	{
		brush(target, _width, _height, _dx, _dy, pi, _radius, true, _new_value, _old_value, [](float v) { return false; });
	}

	end_datachange(iseg::NoUndo);
}

void BrushInteraction::on_mouse_released(Point p)
{
	std::vector<Point> vps;
	addLine(&vps, _last_pt, p);

	float* target = slice();
	for (auto pi : vps)
	{
		brush(target, _width, _height, _dx, _dy, pi, _radius, true, _new_value, _old_value, [](float v) { return false; });
	}

	end_datachange(iseg::EndUndo);
}

}