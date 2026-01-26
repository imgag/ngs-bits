#ifndef GENEWIDGET_H
#define GENEWIDGET_H

#include <QWidget>
#include "ui_GeneWidget.h"
#include "NGSD.h"
#include "DelayedInitializationTimer.h"
#include "TabBaseClass.h"

class GeneWidget
	: public TabBaseClass
{
	Q_OBJECT

public:
    GeneWidget(QWidget* parent, QByteArray symbol);

public slots:
    ///Loads information needed for the GUI
    void delayedInitialization();

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
    DelayedInitializationTimer init_timer_;
    QByteArray symbol_;
	QStringList omim_lines;
	QStringList orpha_lines;
	QStringList hpo_lines;

	void updateTranscriptsTable(NGSD& db);
};

#endif // GENEWIDGET_H
