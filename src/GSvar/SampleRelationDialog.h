#ifndef SAMPLERELATIONWIDGET_H
#define SAMPLERELATIONWIDGET_H

#include <QDialog>

#include "ui_SampleRelationDialog.h"
#include "NGSD.h"

class SampleRelationDialog
	: public QDialog
{
	Q_OBJECT

public:
	SampleRelationDialog(QWidget* parent);
	void setSample1(QString sample_name, bool enabled=true);
	void setSample2(QString sample_name, bool enabled=true);

	SampleRelation sampleRelation() const;

private slots:
	void swapSamples();
	void updateOkButton();

private:
	Ui::SampleRelationDialog ui_;
};

#endif // SAMPLERELATIONWIDGET_H
