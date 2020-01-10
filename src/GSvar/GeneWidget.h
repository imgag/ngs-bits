#ifndef GENEWIDGET_H
#define GENEWIDGET_H

#include <QWidget>
#include "ui_GeneWidget.h"


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

private:
    Ui::GeneWidget ui_;
    QByteArray symbol_;
};

#endif // GENEWIDGET_H
