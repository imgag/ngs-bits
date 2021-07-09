#ifndef CNVSEARCHWIDGET_H
#define CNVSEARCHWIDGET_H

#include <QWidget>
#include "NGSD.h"
#include "DelayedInitializationTimer.h"
#include "ui_CnvSearchWidget.h"

class CnvSearchWidget
	: public QWidget
{
	Q_OBJECT

public:
	CnvSearchWidget(QWidget* parent = 0);
	void setCoordinates(Chromosome chr, int start, int end);

protected slots:
	void search();
	void delayedInitialization();
	void copyCoodinatesToClipboard();
	void changeSearchType();
	void openSelectedSampleTabs();

private:
	Ui::CnvSearchWidget ui_;
	DelayedInitializationTimer init_timer_;
	NGSD db_;
};

#endif // CNVSEARCHWIDGET_H
