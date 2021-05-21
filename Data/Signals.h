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

#ifndef Q_MOC_RUN
#	include <boost/signals2.hpp>
#endif

#include <memory>

namespace iseg {

template<class S, class C, typename F>
boost::signals2::connection Connect(S& signal, const boost::shared_ptr<C>& slot_owner, F func)
{
	return signal.connect(typename S::slot_type(std::move(func)).track(slot_owner));
}

template<class S, typename C, typename F>
boost::signals2::connection Connect(S& signal, const std::shared_ptr<C>& slot_owner, F func)
{
	return signal.connect(typename S::slot_type(std::move(func)).track_foreign(slot_owner));
}

template<class S, typename C, typename F>
boost::signals2::connection Connect(S& signal, const C& slot_owner, F func)
{
	return signal.connect(typename S::slot_type(std::move(func)).track_foreign(slot_owner));
}

} // namespace iseg
