#include "SplitterHandle.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QTreeView>

namespace iseg {

SplitterHandle::SplitterHandle(QTreeView* parent)
		: QWidget(parent), m_Parent(parent)
{
	setCursor(Qt::SplitHCursor);
	int w = 7;
	setGeometry(m_Parent->width() / 2 - w / 2, 0, w, m_Parent->height());
	m_Parent->installEventFilter(this);
}

bool SplitterHandle::eventFilter(QObject* event_target, QEvent* event)
{
	if (event_target == m_Parent && event->type() == QEvent::Resize)
	{
		UpdateGeometry();
	}
	return false; // Return false regardless so resize event also does its normal job.
}

void SplitterHandle::UpdateGeometry()
{
	int content_width = m_Parent->contentsRect().width();
	int new_column_width = int(m_FirstColumnSizeRatio * content_width);
	m_Parent->setColumnWidth(0, new_column_width);
	m_Parent->setColumnWidth(1, content_width - new_column_width);
	setGeometry(new_column_width - width() / 2, 0, width(), m_Parent->height());
}

void SplitterHandle::SetRatio(float ratio)
{
	m_FirstColumnSizeRatio = ratio;
	UpdateGeometry();
}

float SplitterHandle::Ratio()
{
	return m_FirstColumnSizeRatio;
}

void SplitterHandle::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_XPressOffset = event->x();
	}
}

void SplitterHandle::mouseMoveEvent(QMouseEvent* event)
{
	int padding = 55;

	if (event->buttons() & Qt::LeftButton)
	{
		QPoint pos_rel_parent = m_Parent->mapFromGlobal(event->globalPos());

		int new_x = pos_rel_parent.x() - m_XPressOffset;

		if (new_x > m_Parent->width() - width() - padding)
		{
			new_x = m_Parent->width() - width() - padding;
		}

		if (new_x < padding)
		{
			new_x = padding;
		}

		if (new_x != x())
		{
			int new_column_width = new_x + width() / 2 - m_Parent->contentsRect().x();
			m_FirstColumnSizeRatio = new_column_width / (float)m_Parent->contentsRect().width();
			UpdateGeometry();
		}
	}
}

} // end namespace iseg
