#include "vtkCustomOutputWindow.h"

#include "Data/Logger.h"

#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkCustomOutputWindow);

void vtkCustomOutputWindow::DisplayText(const char* msg)
{
	ISEG_WARNING_MSG(msg);
}
