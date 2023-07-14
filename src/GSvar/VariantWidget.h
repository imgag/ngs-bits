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

private slots:
	void updateGUI();
	void delayedInitialization();
	void copyToClipboard();
	void calculateSimilarity();
	void openProcessedSampleTabs();
	void openGeneTab(QString symbol);
	void openGSvarFile();
	void editComment();
	void editClassification();
	void gnomadClicked(QString var_id);
	void pubmedClicked(QString link);
	void tableCellDoubleClicked(int row, int column);

private:
	Ui::VariantWidget ui_;
	DelayedInitializationTimer init_timer_;
	Variant variant_;

	QTableWidgetItem* addItem(int r, int c, QString text, bool also_as_tooltip=false);
	QList<int> selectedRows() const;
};

#endif // VARIANTWIDGET_H
