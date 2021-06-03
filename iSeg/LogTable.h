#ifndef QLOGTABLE_H
#define QLOGTABLE_H

#include <QAbstractScrollArea>
#include <QTextStream>

class QLogTable : public QAbstractScrollArea
{
	Q_OBJECT

public:
	enum eLimit {
		NoLimit = -1
	};

	QLogTable(int numCols, QWidget* parent = nullptr);
	~QLogTable() override;

	void SetLimit(int l);
	void setFont(QFont&);
	void SetColumnsNumber(int);

	friend QTextStream& operator<<(QTextStream&, const QLogTable&);

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void keyPressEvent(QKeyEvent* e) override;
	virtual void Paint(QPainter* p, QPaintEvent* e);

	void UpdateScrollbar();

public slots:
	void ClearBuffer();
	void AppendRow(const QStringList&);

protected:
	QList<QList<QString>>* m_Content; // widget data
	int* m_ColWidth;									// columns width
	QFont m_CurFont;									// current font
	int m_RowHeight;									// column height get from curFont (internally used)
	int m_NumberOfLines;							// number of lines/rows
	int m_NumberOfColumns;						// number of columns
	int m_Limit;											// limit for number of lines (not implemented)

private:
	void ManageLimit();
};

#endif // QLOGTEXT_H
