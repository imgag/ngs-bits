#ifndef GENEWIDGET_H
#define GENEWIDGET_H

#include <QWidget>
#include "ui_GeneWidget.h"
#include "NGSD.h"

class GeneWidget
    : public QWidget
{
	Q_OBJECT

public:
    GeneWidget(QWidget* parent, QByteArray symbol);

private slots:
    void updateGUI();
    void editInheritance();
    void editComment();
	void showGeneVariationDialog();
	void openGeneDatabase();
    void parseLink(QString link);
	void openGeneTab(QString symbol);
	void updatePhenotypeSearch();

private:
    Ui::GeneWidget ui_;
    QByteArray symbol_;
	QStringList omim_lines;
	QStringList orpha_lines;
	QStringList hpo_lines;

	void updateTranscriptsTable(NGSD& db);
};

#endif // GENEWIDGET_H
