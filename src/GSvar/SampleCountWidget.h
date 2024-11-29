#ifndef SAMPLECOUNTWIDGET_H
#define SAMPLECOUNTWIDGET_H

#include <QWidget>
#include "ui_SampleCountWidget.h"
#include "DelayedInitializationTimer.h"

class SampleCountWidget
	: public QWidget
{
	Q_OBJECT

public:
	SampleCountWidget(QWidget *parent = nullptr);

protected slots:
	void delayedInitialization();

private:
	Ui::SampleCountWidget ui_;
	DelayedInitializationTimer init_timer_;
};

#endif // SAMPLECOUNTWIDGET_H
