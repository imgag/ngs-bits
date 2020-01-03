#ifndef VARIANTSAMPLEOVERVIEWWIDGET_H
#define VARIANTSAMPLEOVERVIEWWIDGET_H

#include <QWidget>
#include "ui_VariantSampleOverviewWidget.h"
#include "VariantList.h"

class VariantSampleOverviewWidget
	: public QWidget
{
	Q_OBJECT

public:
	VariantSampleOverviewWidget(const Variant& variant, QWidget* parent = 0);

signals:
	void openProcessedSampleTab(QString);
	void openProcessedSampleFromNGSD(QString);

private slots:
	void copyToClipboard();
	void calculateSimilarity();
	void openProcessedSample();
	void openProcessedSampleTab();

private:
	Ui::VariantSampleOverviewWidget ui_;

	void addItem(int r, int c, QString text);
	QList<int> selectedRows() const;
};

#endif // VARIANTSAMPLEOVERVIEWWIDGET_H
