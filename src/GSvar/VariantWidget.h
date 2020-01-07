#ifndef VARIANTWIDGET_H
#define VARIANTWIDGET_H

#include <QWidget>
#include "ui_VariantWidget.h"
#include "VariantList.h"

class VariantWidget
	: public QWidget
{
	Q_OBJECT

public:
	VariantWidget(const Variant& variant, QWidget* parent = 0);

signals:
	void openProcessedSampleTab(QString);
	void openProcessedSampleFromNGSD(QString);
	void openGeneTab(QString);

private slots:
	void updateGUI();
	void copyToClipboard();
	void calculateSimilarity();
	void openProcessedSample();
	void openProcessedSampleTab();

private:
	Ui::VariantWidget ui_;
	Variant variant_;

	void addItem(int r, int c, QString text);
	QList<int> selectedRows() const;
};

#endif // VARIANTWIDGET_H
