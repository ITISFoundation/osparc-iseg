#include "QSliderEditableRange.h"

#include "QtConnect.h"

#include <QBoxLayout>
#include <QIntValidator>
#include <QLineEdit>
#include <QSlider>

namespace iseg {

QSliderEditableRange::QSliderEditableRange(QWidget* parent, Qt::WindowFlags flags) : QWidget(parent, flags)
{
	m_Slider = new QSlider(Qt::Horizontal);
	m_MinEdit = new QLineEdit(QString::number(m_Slider->minimum()));
	m_MaxEdit = new QLineEdit(QString::number(m_Slider->maximum()));

	m_MinEdit->setValidator(new QIntValidator);
	m_MaxEdit->setValidator(new QIntValidator);

	m_MinEdit->setMinimumHeight(18);
	m_MaxEdit->setMinimumHeight(18);

	auto hbox = new QHBoxLayout;
	hbox->setContentsMargins(0, 0, 0, 0);
	hbox->setMargin(0);
	hbox->setSpacing(0);

	hbox->addWidget(m_MinEdit, 1);
	hbox->addWidget(m_Slider, 8);
	hbox->addWidget(m_MaxEdit, 1);
	setLayout(hbox);

	QObject_connect(m_Slider, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged(int)));

	QObject_connect(m_Slider, SIGNAL(sliderPressed()), this, SIGNAL(sliderPressed()));
	QObject_connect(m_Slider, SIGNAL(sliderMoved(int)), this, SIGNAL(sliderMoved(int)));
	QObject_connect(m_Slider, SIGNAL(sliderReleased()), this, SIGNAL(sliderReleased()));
	QObject_connect(m_Slider, SIGNAL(rangeChanged(int, int)), this, SIGNAL(rangeChanged(int, int)));
	QObject_connect(m_Slider, SIGNAL(actionTriggered(int)), this, SIGNAL(actionTriggered(int)));

	QObject_connect(m_MinEdit, SIGNAL(editingFinished()), this, SLOT(Edited()));
	QObject_connect(m_MaxEdit, SIGNAL(editingFinished()), this, SLOT(Edited()));
}

void QSliderEditableRange::Edited()
{
	if (auto edit = dynamic_cast<QLineEdit*>(QObject::sender()))
	{
		if (edit == m_MinEdit)
		{
			m_Slider->setMinimum(edit->text().toInt());
		}
		if (edit == m_MaxEdit)
		{
			m_Slider->setMaximum(edit->text().toInt());
		}
	}
}

void QSliderEditableRange::setValue(int v)
{
	m_Slider->setValue(v);
}

int QSliderEditableRange::value() const
{
	return m_Slider->value();
}

void QSliderEditableRange::setRange(int vmin, int vmax)
{
	setMinimum(vmin);
	setMaximum(vmax);
}

void QSliderEditableRange::setMinimum(int v)
{
	m_Slider->setMinimum(v);
}

int QSliderEditableRange::minimum() const
{
	return m_Slider->minimum();
}

void QSliderEditableRange::setMaximum(int v)
{
	m_Slider->setMaximum(v);
}

int QSliderEditableRange::maximum() const
{
	return m_Slider->maximum();
}
void QSliderEditableRange::setMinimumVisible(bool v)
{
	m_MinEdit->setVisible(v);
}

void QSliderEditableRange::setMaximumVisible(bool v)
{
	m_MaxEdit->setVisible(v);
}

} // namespace iseg