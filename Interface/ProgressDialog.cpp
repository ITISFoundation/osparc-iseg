#include "ProgressDialog.h"

#include "../Data/Logger.h"

#include <qapplication.h>
#include <qprogressdialog.h>
#include <qpushbutton.h>

namespace iseg {

ProgressDialog::ProgressDialog(const char* msg, QWidget* parent /*= 0*/)
		: QObject(parent)
{
	m_Progress = new QProgressDialog(msg, "Cancel", 0, 100, parent);
	m_Progress->setWindowModality(Qt::WindowModal);
	m_Progress->setMinimumDuration(0);

	auto cancel_button = new QPushButton(QString("Cancel"), m_Progress);
	m_Progress->setCancelButton(cancel_button);

	m_Count.store(0);

	QObject_connect(cancel_button, SIGNAL(clicked()), this, SLOT(Cancel()));
}

void ProgressDialog::SetNumberOfSteps(int N)
{
	m_Progress->setMaximum(N);
	m_Progress->setValue(0);
}

void ProgressDialog::Increment()
{
	m_Count++;
	m_Progress->setValue(m_Count);
}

bool ProgressDialog::WasCanceled() const
{
	return m_Canceled;
}

void ProgressDialog::SetValue(int percent)
{
	if (m_Progress->value() != percent)
	{
		m_Progress->setValue(percent);

		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
}

void ProgressDialog::Cancel()
{
	m_Canceled = true;
}

} // namespace iseg
