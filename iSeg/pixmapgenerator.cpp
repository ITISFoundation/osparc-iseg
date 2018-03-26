/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"
#include "pixmapgenerator.h"
#include "tissueinfos.h"
#include <qpainter.h>

QPixmap generatePixmap(tissues_size_t tissuenr)
{
	QPixmap abc(10,10);
	if(tissuenr>TissueInfos::GetTissueCount()) {
		abc.fill(QColor(0,0,0));
		return abc;
	}
	unsigned char r, g, b;
	TissueInfoStruct *tissueInfo = TissueInfos::GetTissueInfo(tissuenr);
	tissueInfo->GetColorRGB(r, g, b);
	abc.fill(QColor(r, g, b));
	if(tissueInfo->locked) {
		QPainter painter(&abc);
//		painter.drawText(3,7,QString("L"));
		float r,g,b;
		r=(tissueInfo->color[0]+0.5f);
		if(r>=1.0f) r=r-1.0f;
		g=(tissueInfo->color[1]+0.5f);
		if(g>=1.0f) g=g-1.0f;
		b=(tissueInfo->color[2]+0.5f);
		if(b>=1.0f) b=b-1.0f;
		painter.setPen(QColor(int(r*255),int(g*255),int(b*255)));
		painter.drawLine(0,0,9,9);
		painter.drawLine(0,9,9,0);
		painter.drawRect(0,0,9,9);
	}

	return abc;
}