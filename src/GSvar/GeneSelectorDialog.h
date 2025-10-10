#ifndef GENESELECTORDIALOG_H
#define GENESELECTORDIALOG_H

#include <QDialog>
#include <QTableWidgetItem>
#include "GeneSet.h"

namespace Ui {
class GeneSelectorDialog;
}

///Dialog to select genes based on gaps and
class GeneSelectorDialog
	: public QDialog
{
	Q_OBJECT

public:
	GeneSelectorDialog(QString sample_name, QWidget *parent = 0);
	~GeneSelectorDialog();
	///Generates and returns the text report
	QString report();
	///Returns the selected gene list (for variants)
	GeneSet genesForVariants();

private slots:
	void updateGeneTable();
	void geneTableItemChanged(int row, int col);
	void geneDoubleClicked(QTableWidgetItem* item);

private:
	Ui::GeneSelectorDialog *ui;	
	QString sample_name_;

    #if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    void setGeneTableItem(int row, int col, QString text, Qt::Alignment alignment = Qt::AlignLeft, Qt::ItemFlags flags = Qt::ItemIsEnabled);
    #else
    void setGeneTableItem(int row, int col, QString text, int alignment = Qt::AlignLeft, Qt::ItemFlags flags = Qt::ItemIsEnabled);
    #endif

	void updateSelectedGenesStatistics();
	void updateError(QString title, QString text);
};

#endif // GENESELECTORDIALOG_H
