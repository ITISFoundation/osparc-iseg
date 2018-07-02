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
			const boost::function<void(EndUndoAction)>& end,
			const boost::function<void(std::vector<Point>*)>& vpdynchanged) 
			: begin_datachange(begin)
			, end_datachange(end)
			, vpdyn_changed(vpdynchanged)
		{
			init(handler);
		}

		/// throws std::runtime_error if handler is invalid
		void init(SliceHandlerInterface* handler);

		void set_radius(float radius) { _radius = radius; }
		void set_brush_target(bool on) { _brush_target = on; }
		void set_tissue_value(tissues_size_t v) { _tissue_value = v; }
		void set_target_value(float v) { _target_value = v; }

		void on_mouse_clicked(Point p);

		void on_mouse_moved(Point p);

		void on_mouse_released(Point p);

		void draw_circle(Point p);

	private:
		boost::function<void(DataSelection)> begin_datachange;
		boost::function<void(EndUndoAction)> end_datachange;
		boost::function<void(std::vector<Point>*)> vpdyn_changed;

		SliceHandlerInterface* _slice_handler;
		std::vector<bool> _cached_tissue_locks;
		unsigned _width;
		unsigned _height;
		float _dx;
		float _dy;
		float _radius = 1.0f;
		bool _brush_target = true;
		float _target_value = 255.f;
		tissues_size_t _tissue_value = 1;
		Point _last_pt;
	};

} // namespace iseg
