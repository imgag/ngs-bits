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

signals:
    void openGeneTab(QString);

private slots:
    void updateGUI();
    void editInheritance();
    void editComment();
	void showGeneVariationDialog();
	void openGeneDatabase();
    void parseLink(QString link);

private:
    Ui::GeneWidget ui_;
    QByteArray symbol_;

	void updateTranscriptsTable(NGSD& db);
};

#endif // GENEWIDGET_H
