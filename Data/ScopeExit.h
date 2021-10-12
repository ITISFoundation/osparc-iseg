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

namespace iseg {

/** This simple template class allows to execute arbitrary code when leaving a scope.

		\note Instances can only be moved, not copied.
	*/
template <typename F>
struct ScopeExit
{
    explicit ScopeExit(F f)
        : f(std::move(f))
        , m_Empty(false)
    {}
    ~ScopeExit()
    {
        if (!m_Empty)
            f();
    }

    F f;

    ScopeExit(ScopeExit && rhs) noexcept
        : f(std::move(rhs.f))
    {
        rhs.m_Empty = true;
        m_Empty = false;
    }

  private:
    ScopeExit(const ScopeExit & rhs) = delete;
    void operator=(const ScopeExit & rhs) = delete;
    bool m_Empty;
};

/// Creates
template <typename F>
ScopeExit<F> MakeScopeExit(F f)
{
    return std::move(ScopeExit<F>(std::move(f)));
};

} // namespace iseg