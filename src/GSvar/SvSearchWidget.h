#ifndef SVSEARCHWIDGET_H
#define SVSEARCHWIDGET_H

#include <QWidget>
#include "NGSD.h"
#include "DelayedInitializationTimer.h"
#include "ui_SvSearchWidget.h"

class SvSearchWidget
	: public QWidget
{
	Q_OBJECT

public:
	SvSearchWidget(QWidget* parent = 0);
	void setCoordinates(const BedpeLine& sv_coordinates);
	void setProcessedSampleId(QString ps_id);

protected slots:
	void search();
	void delayedInitialization();
	void changeSearchType();

private:
	Ui::SvSearchWidget ui_;
	DelayedInitializationTimer init_timer_;
	NGSD db_;
	QString ps_id_;
};

#endif // SVSEARCHWIDGET_H
