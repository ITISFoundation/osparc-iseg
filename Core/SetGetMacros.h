/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef SETGETMACROS_H
#define SETGETMACROS_H

// These are ripped from vtkSetGet.h
#include <string.h>

#define SetMacro(name, type)                                                   \
	virtual void Set##name(type _arg)                                            \
	{                                                                            \
		if (this->name != _arg)                                                    \
		{                                                                          \
			this->name = _arg;                                                       \
		}                                                                          \
	}

//
// Get built-in type.  Creates member Get"name"() (e.g., GetVisibility());
//
#define GetMacro(name, type)                                                   \
	virtual type Get##name() { return this->name; }

//
// Set character string.  Creates member Set"name"()
// (e.g., SetFilename(char *));
//
#define SetStringMacro(name)                                                   \
	virtual void Set##name(const char *_arg)                                     \
	{                                                                            \
		if (this->name == NULL && _arg == NULL)                                    \
		{                                                                          \
			return;                                                                  \
		}                                                                          \
		if (this->name && _arg && (!strcmp(this->name, _arg)))                     \
		{                                                                          \
			return;                                                                  \
		}                                                                          \
		if (this->name)                                                            \
		{                                                                          \
			delete[] this->name;                                                     \
		}                                                                          \
		if (_arg)                                                                  \
		{                                                                          \
			size_t n = strlen(_arg) + 1;                                             \
			char *cp1 = new char[n];                                                 \
			const char *cp2 = (_arg);                                                \
			this->name = cp1;                                                        \
			do                                                                       \
			{                                                                        \
				*cp1++ = *cp2++;                                                       \
			} while (--n);                                                           \
		}                                                                          \
		else                                                                       \
		{                                                                          \
			this->name = NULL;                                                       \
		}                                                                          \
	}

//
// Get character string.  Creates member Get"name"()
// (e.g., char *GetFilename());
//
#define GetStringMacro(name)                                                   \
	virtual char *Get##name() { return this->name; }

#define SetStdStringMacro(name)                                                \
	virtual void Set##name(const std::string _arg) { this->name = _arg; }

#define GetStdStringMacro(name)                                                \
	virtual std::string Get##name() const { return this->name; }

#endif // SETGETMACROS_H
