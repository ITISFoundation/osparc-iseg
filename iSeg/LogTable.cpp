#include "LogTable.h"

#include <QFont>
#include <QtGui>

#define PART_OF_CONTENT_TO_KEEP 0.75

/*!
    \class QLogTable
    \brief The QLogTable class is a very basic widget
    that is used to display text in a table (like QTableWidget).
    
    - The widget can only use a proportionnal font
    - Scrollbars are automatically updated when text is added
    - Copy/paste/cur and selection are not implemented
    - The number of columns can't be changed after creation
*/

/*!
    Constructs an empty QLogTable with parent \a
    parent and \a numCols columns.
    
    The default font will be a 10pt Courier.
*/
QLogTable::QLogTable(int numCols, QWidget* parent) : QAbstractScrollArea(parent), m_NumberOfColumns(numCols), m_Limit(NoLimit)
{
	m_CurFont = QFont("Courier", 10, QFont::Normal);
	m_RowHeight = QFontMetrics(m_CurFont).lineSpacing() + 3;

	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	m_Content = new QList<QList<QString>>;
	m_ColWidth = new int[m_NumberOfColumns];
	for (int i = 0; i < m_NumberOfColumns; i++)
	{
		m_ColWidth[i] = 0;
	}

	verticalScrollBar()->setMaximum(m_Content->size() * m_RowHeight);
	verticalScrollBar()->setSingleStep(m_RowHeight);
	verticalScrollBar()->setPageStep((height() * 8) / 10);
	horizontalScrollBar()->setSingleStep(QFontMetrics(m_CurFont).maxWidth());
	horizontalScrollBar()->setPageStep(10 * QFontMetrics(m_CurFont).maxWidth());
}

/*!
    Destructor.
*/
QLogTable::~QLogTable()
{
	delete[] m_ColWidth;
}

/*!
    Remove all text in the table and clear the widget
*/
void QLogTable::ClearBuffer()
{
	m_Content->clear();
	viewport()->update();
}

/*!
    Set the font used by the widget
    This font must be a proportionnal font (like Courier)
*/
void QLogTable::setFont(QFont& f)
{
	if (f.styleHint() == QFont::TypeWriter || f.styleHint() == QFont::Courier)
	{
		m_CurFont = f;
		m_RowHeight = QFontMetrics(m_CurFont).lineSpacing() + 3;
		verticalScrollBar()->setSingleStep(m_RowHeight);
	}
}

/*!
    Set the content limit
*/
void QLogTable::SetLimit(int l)
{
	m_Limit = l;
}

/*!
    Set the number of columns of the widget
    Remove some columns or add some new empty columns
*/
void QLogTable::SetColumnsNumber(int n)
{
	if ((n > 0) && (n != m_NumberOfColumns))
	{
		int* colw = new int[n];
		for (int i = 0; i < (n < m_NumberOfColumns ? n : m_NumberOfColumns); i++)
		{
			colw[i] = m_ColWidth[i];
		}
		if (n > m_NumberOfColumns)
		{
			for (int i = m_NumberOfColumns; i < n; i++)
			{
				colw[i] = 0;
			}
		}

		int* oldcolw = m_ColWidth;

		m_ColWidth = colw;
		m_NumberOfColumns = n;

		delete[] oldcolw;
	}
}

/*!
    Append a row with \a str data
    \a str is a QStringList where each item is a cell of the row.
    
    So number of columns should equal to str.size()
*/
void QLogTable::AppendRow(const QStringList& str)
{
	if (!str.isEmpty())
	{

		ManageLimit();

		(*m_Content) << str;

		for (int i = 0; (i < m_NumberOfColumns) && (i < str.size()); i++)
		{
			if (str[i].size() > m_ColWidth[i])
				m_ColWidth[i] = str[i].size();
		}

		UpdateScrollbar();
		// Set the scrollbar position at the bottom of the widget
		if (!verticalScrollBar()->isSliderDown() && !horizontalScrollBar()->isSliderDown())
		{
			verticalScrollBar()->setValue(verticalScrollBar()->maximum());
			viewport()->update();
		}
	}
}

void QLogTable::ManageLimit()
{
	if ((m_Limit != NoLimit) && (m_Content->size() >= m_Limit))
	{
		// Remove a part of the content
		QList<QList<QString>>::iterator e = m_Content->begin();
		e += (int)(m_Limit * (1 - PART_OF_CONTENT_TO_KEEP));
		m_Content->erase(m_Content->begin(), e);
	}
}

/*! \internal
    Update scrollbar values (current and maximum)
*/
void QLogTable::UpdateScrollbar()
{
	// Update the scrollbar
	int max = (m_Content->size() * m_RowHeight) - viewport()->height();
	verticalScrollBar()->setMaximum(max < 0 ? 0 : max);

	int maxlinelength = 0;

	for (int i = 0; i < m_NumberOfColumns; i++)
	{
		maxlinelength += m_ColWidth[i] * QFontMetrics(m_CurFont).maxWidth() + 5;
	}

	max = (maxlinelength)-viewport()->width();
	horizontalScrollBar()->setMaximum(max < 0 ? 0 : max);
}

/*! \internal
    All input event are ignored
*/
void QLogTable::keyPressEvent(QKeyEvent* e)
{
	switch (e->key())
	{
	case Qt::Key_PageUp:
		verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
		break;
	case Qt::Key_PageDown:
		verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
		break;
	default:
		e->ignore();
	}
}

/*! \internal
*/
void QLogTable::resizeEvent(QResizeEvent* event)
{
	QPainter p(viewport());
	verticalScrollBar()->setPageStep((height() * 8) / 10);
	UpdateScrollbar();
}

/*! \internal
*/
void QLogTable::paintEvent(QPaintEvent* event)
{
	QPainter p(viewport());
	Paint(&p, event);
}

/*! \internal
    To paint the widget.
    Only lines displayed by the widget are painted. The very low QTextLayout API is used
    to optimize the display.
*/
void QLogTable::Paint(QPainter* p, QPaintEvent* e)
{
	bool line_drawn = false;
	const int x_offset = horizontalScrollBar()->value();
	const int y_offset = verticalScrollBar()->value();

	QPen text_pen(Qt::black);
	QPen lines_pen(Qt::gray);
	lines_pen.setStyle(Qt::DotLine);

	QRect r = e->rect();
	p->translate(-x_offset, -y_offset);
	r.translate(x_offset, y_offset);
	p->setClipRect(r);

	for (int i = (r.top() / m_RowHeight); i < (r.bottom() / m_RowHeight) + 1; i++)
	{
		if (i < m_Content->size())
		{
			int x = 0;
			for (int j = 0; j < (m_NumberOfColumns < m_Content->at(i).size() ? m_NumberOfColumns : m_Content->at(i).size()); j++)
			{

				QTextLayout text_layout(m_Content->at(i).at(j), m_CurFont);
				text_layout.beginLayout();
				text_layout.createLine();
				text_layout.endLayout();

				p->setPen(text_pen);
				text_layout.draw(p, QPointF(x, i * m_RowHeight));

				x += m_ColWidth[j] * QFontMetrics(m_CurFont).maxWidth() + 5;

				if (!line_drawn)
				{ // Draw the vertical line but only once
					p->setPen(lines_pen);
					p->drawLine(x - 3, r.top(), x - 3, (m_Content->size() * m_RowHeight < r.bottom() ? m_Content->size() * m_RowHeight - 2 : r.bottom()));
				}
			}
			line_drawn = true;

			// Draw horizontal lines (normally x = right limit of the table)
			p->setPen(lines_pen);
			p->drawLine(r.left(), (i + 1) * m_RowHeight - 2, ((x - 3) < r.right() ? (x - 3) : r.right()), (i + 1) * m_RowHeight - 2);
		}
	}
}

QTextStream& operator<<(QTextStream& out, const QLogTable& table)
{
	for (int i = 0; i < table.m_Content->size(); ++i)
	{
		for (int j = 0; j < (table.m_NumberOfColumns < table.m_Content->at(i).size() ? table.m_NumberOfColumns : table.m_Content->at(i).size()); j++)
		{
			QString str = table.m_Content->at(i).at(j);
			str = str.leftJustified(1 + table.m_ColWidth[j], ' ');
			out << str;
		}
		out << endl;
	}
	return out;
}
