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

		void on_mouse_clicked(Point p);

		void on_mouse_moved(Point p);

		void on_mouse_released(Point p);

	private:
		boost::function<void(DataSelection)> begin_datachange;
		boost::function<void(EndUndoAction)> end_datachange;

		SliceHandlerInterface* _slice_handler;
		unsigned _width;
		unsigned _height;
		float _dx;
		float _dy;
		float _radius = 1.0f;
		Point _last_pt;
	};

} // namespace iseg
