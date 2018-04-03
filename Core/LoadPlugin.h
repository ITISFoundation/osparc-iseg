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

#include "iSegCore.h"

#include <string>

namespace iseg { namespace plugin {

iSegCore_API bool LoadPlugin(const std::string& plugin_file_path);

iSegCore_API bool LoadPlugins(const std::string& directory_path);
}} // namespace iseg::plugin
