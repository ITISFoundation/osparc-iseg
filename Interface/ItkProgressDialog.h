
#include <itkCommand.h>

#include <qprogressdialog.h>

namespace iseg {

// \todo change name
template<class TFilter>
class CommandIterationUpdate : public itk::Command
{
public:
	using Self = CommandIterationUpdate;
	using Superclass = itk::Command;
	using Pointer = itk::SmartPointer<Self>;
	itkNewMacro(Self);

protected:
	CommandIterationUpdate()
	{
		//QProgressDialog progress("Performing bias correction...", "Cancel", 0, 101, this);
		//progress.setWindowModality(Qt::WindowModal);
		//progress.setModal(true);
		//progress.show();
		//progress.setValue(1);
	}

public:
	void SetProgressObject(QProgressDialog* progress)
	{
		m_Progress = progress;
	}

	void Execute(itk::Object* caller, const itk::EventObject& event) ITK_OVERRIDE
	{
		Execute((const itk::Object*)caller, event);
	}

	void Execute(const itk::Object* object, const itk::EventObject& event) ITK_OVERRIDE
	{
		const TFilter* filter = dynamic_cast<const TFilter*>(object);

		// \todo is this correct?
		if (typeid(event) != typeid(itk::IterationEvent))
		{
			return;
		}

		int percent = 1; // \todo get percent from filter/event
		m_Progress->setValue(std::min(percent, 100));

		if (m_Progress->wasCanceled())
		{
			// hack to stop filter from executing
			throw itk::ProcessAborted();
		}
	}

private:
	QProgressDialog* m_Progress;
};

}
