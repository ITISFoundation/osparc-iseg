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

#include "iSegData.h"

#include "DataSelection.h"
#include "Point.h"
#include "SlicesHandlerInterface.h"

#include <boost/function.hpp>

namespace iseg {

	class ISEG_DATA_API BrushInteraction
	{
	public:
		BrushInteraction(SliceHandlerInterface* handler,
			const boost::function<void(DataSelection)>& begin,
			const boost::function<void(EndUndoAction)>& end) 
			: begin_datachange(begin), end_datachange(end)
		{
			init(handler);
		}

		void init(SliceHandlerInterface* handler);

		void set_radius(float radius) { _radius = radius; }
		void set_brush_target(bool on) { _brush_target = on; }
		void set_old_value(float v) { _old_value = v; }
		void set_new_value(float v) { _new_value = v; }

		void on_mouse_clicked(Point p);

		void on_mouse_moved(Point p);

		void on_mouse_released(Point p);

	private:
		float* slice()
		{
			return _brush_target
				? _slice_handler->target_slices().at(_slice_handler->active_slice())
				: _slice_handler->target_slices().at(_slice_handler->active_slice());
		}

		boost::function<void(DataSelection)> begin_datachange;
		boost::function<void(EndUndoAction)> end_datachange;

		SliceHandlerInterface* _slice_handler;
		unsigned _width;
		unsigned _height;
		float _dx;
		float _dy;
		float _radius = 1.0f;
		bool _brush_target = true;
		float _old_value = 0.f;
		float _new_value = 255.f;
		Point _last_pt;
	};

} // namespace iseg
