#include "ItkProgressObserver.h"

#include "Logger.h"

#include <itkProcessObject.h>

namespace iseg {

void ItkProgressObserver::Execute(itk::Object* object, const itk::EventObject& event)
{
	Execute((const itk::Object*)object, event);
}

void ItkProgressObserver::Execute(const itk::Object* object, const itk::EventObject& event)
{
	if (!itk::ProgressEvent().CheckEvent(&event))
	{
		return;
	}
	const auto* filter = dynamic_cast<const itk::ProcessObject*>(object);
	if (!filter)
	{
		return;
	}

	// check if abort was requested
	if (m_ProgressInfo->WasCanceled())
	{
		ISEG_WARNING("Process aborted");

		auto filter_non_const = const_cast<itk::ProcessObject*>(filter);

		filter_non_const->AbortGenerateDataOn();
	}
	else
	{
		// update progress
		int percent = static_cast<int>(100.f * filter->GetProgress());
		m_ProgressInfo->SetValue(percent);
	}
}

} // namespace iseg
