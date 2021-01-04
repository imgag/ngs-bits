#ifndef VARIANTWIDGET_H
#define VARIANTWIDGET_H

#include <QWidget>
#include "ui_VariantWidget.h"
#include "VariantList.h"
#include "DelayedInitializationTimer.h"

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
	void delayedInitialization();
	void copyToClipboard();
	void calculateSimilarity();
	void openProcessedSampleTab();
	void openGSvarFile();
	void editClassification();
	void gnomadClicked(QString var_id);

private:
	Ui::VariantWidget ui_;
	DelayedInitializationTimer init_timer_;
	Variant variant_;

	QTableWidgetItem* addItem(int r, int c, QString text);
	QList<int> selectedRows() const;
};

#endif // VARIANTWIDGET_H
