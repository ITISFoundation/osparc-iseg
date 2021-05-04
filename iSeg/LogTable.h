#ifndef QLOGTABLE_H
#define QLOGTABLE_H

#include <QAbstractScrollArea>
#include <QTextStream>

class QLogTable : public QAbstractScrollArea
{
	Q_OBJECT

public:
	enum Limit {
		NoLimit = -1
	};

	QLogTable(int numCols, QWidget* parent = 0);
	~QLogTable();

	void setLimit(int l);
	void setFont(QFont&);
	void setColumnsNumber(int);

	friend QTextStream& operator<<(QTextStream&, const QLogTable&);

protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void keyPressEvent(QKeyEvent* e);
	virtual void paint(QPainter* p, QPaintEvent* e);

	void updateScrollbar();

public slots:
	void clearBuffer();
	void appendRow(const QStringList&);

protected:
	QList<QList<QString>>* content; // widget data
	int* colWidth;									// columns width
	QFont curFont;									// current font
	int rowHeight;									// column height get from curFont (internally used)
	int numberOfLines;							// number of lines/rows
	int numberOfColumns;						// number of columns
	int limit;											// limit for number of lines (not implemented)

private:
	void manageLimit();
};

#endif // QLOGTEXT_H
