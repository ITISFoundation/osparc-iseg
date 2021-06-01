/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/WidgetInterface.h"

#include <QPushButton>
#include <QRadioButton>

namespace iseg {

class PickerWidget : public WidgetInterface
{
	Q_OBJECT
public:
	PickerWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~PickerWidget() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void Init() override;
	void Cleanup() override;
	void NewLoaded() override;
	std::string GetName() override { return std::string("Picker"); };
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath(QString("picker.png"))); }

private:
	void OnSlicenrChanged() override;

	void OnMouseClicked(Point p) override;

	void UpdateActive();
	void Showborder();

	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned int m_Width;
	unsigned int m_Height;
	bool m_Hasclipboard;
	bool m_Shiftpressed;
	bool m_Clipboardworkortissue;
	bool* m_Mask;
	bool* m_Currentselection;
	float* m_Valuedistrib;
	unsigned char m_Mode;

	QPushButton* m_PbCopy;
	QPushButton* m_PbPaste;
	QPushButton* m_PbCut;
	QPushButton* m_PbDelete;
	QRadioButton* m_RbWork;
	QRadioButton* m_RbTissue;
	QRadioButton* m_RbErase;
	QRadioButton* m_RbFill;

	std::vector<Point> m_Selection;

signals:
	void Vp1Changed(std::vector<Point>* vp1);

private slots:
	void CopyPressed();
	void CutPressed();
	void PastePressed();
	void DeletePressed();

	void WorktissueChanged(int);
	void BmphandChanged(Bmphandler* bmph);

protected:
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
};

} // namespace iseg
