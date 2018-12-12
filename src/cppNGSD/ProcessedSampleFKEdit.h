#ifndef PROCESSEDSAMPLEFKEDIT_H
#define PROCESSEDSAMPLEFKEDIT_H

#include "GBDOFKEdit.h"
#include "NGSD.h"

///Database-aware Foreign-Key edit for NGSD processed samples
class ProcessedSampleFKEdit
	: public GBDOFKEdit
{
	Q_OBJECT

public:
	ProcessedSampleFKEdit(QWidget* parent=0);

protected slots:
	void search(QString text);

protected:
	NGSD db_;
};

#endif
