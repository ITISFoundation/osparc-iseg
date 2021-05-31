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

#include <vtkOutputWindow.h>

// \brief Redirect VTK errors/warnings to file
class vtkCustomOutputWindow : public vtkOutputWindow
{
public:
	static vtkCustomOutputWindow* New();
	vtkTypeMacro(vtkCustomOutputWindow, vtkOutputWindow);
	void PrintSelf(ostream& os, vtkIndent indent) override {}

	// Put the text into the output stream.
	void DisplayText(const char* msg) override;

protected:
	vtkCustomOutputWindow() = default;

private:
	vtkCustomOutputWindow(const vtkCustomOutputWindow&) = delete;
	void operator=(const vtkCustomOutputWindow&) = delete;
};
