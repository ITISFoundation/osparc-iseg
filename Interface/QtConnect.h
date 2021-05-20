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

#include <cassert>

#define QObject_connect(sender, sig, receiver, slot) \
{ \
    bool ok_ = QObject::connect(sender, sig, receiver, slot); \
    assert(ok_); \
}

#define QObject_disconnect(sender, sig, receiver, slot) \
{ \
    bool ok_ = QObject::disconnect(sender, sig, receiver, slot); \
    assert(ok_); \
}
