/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Addon.h"

namespace iseg { namespace plugin {

namespace
{
	std::vector<CAddon*> _addons;
}

std::vector<CAddon*> CAddonRegistry::GetAllAddons()
{
	return _addons;
}


CAddon::CAddon()
{
	_addons.push_back(this);
}

CAddon::~CAddon()
{

}



}}