#include "ProgressDialog.h"

#include "../Data/Logger.h"

#include <qapplication.h>
#include <qprogressdialog.h>
#include <qpushbutton.h>

namespace iseg {

ProgressDialog::ProgressDialog(const char* msg, QWidget* parent /*= 0*/)
		: QObject(parent)
{
	progress = new QProgressDialog(msg, "Cancel", 0, 100, parent);
	progress->setWindowModality(Qt::WindowModal);
	progress->setMinimumDuration(0);

	auto cancel_button = new QPushButton(QString("Cancel"), progress);
	progress->setCancelButton(cancel_button);

	QObject::connect(cancel_button, SIGNAL(clicked()), this, SLOT(cancel()));
}

bool ProgressDialog::wasCanceled() const
{
	return canceled;
}

void ProgressDialog::setValue(int percent)
{
	if (progress->value() != percent)
	{
		progress->setValue(percent);

		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
}

void ProgressDialog::cancel()
{
	canceled = true;
}

} // namespace iseg
