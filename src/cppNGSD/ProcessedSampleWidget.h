#ifndef PROCESSEDSAMPLEWIDGET_H
#define PROCESSEDSAMPLEWIDGET_H

#include "cppNGSD_global.h"
#include "NGSD.h"

#include <QWidget>

namespace Ui {
class ProcessedSampleWidget;
}

class CPPNGSDSHARED_EXPORT ProcessedSampleWidget
	: public QWidget
{
	Q_OBJECT


public:
	ProcessedSampleWidget(QWidget* parent, QString ps_id);
	~ProcessedSampleWidget();

protected:
	void updateGUI();

private:
	Ui::ProcessedSampleWidget* ui_;
	QString id_;
	NGSD db_;
};

#endif // PROCESSEDSAMPLEWIDGET_H
