#ifndef IGVLOGWIDGET_H
#define IGVLOGWIDGET_H

#include <QWidget>
#include "ui_IgvLogWidget.h"


class IgvLogWidget: public QWidget
{
    Q_OBJECT

public:
    IgvLogWidget(QWidget *parent = 0, int index = 0);

public slots:
    void updateTable(QStringList updated_history);

private:
    Ui::IgvLogWidget ui_;
};

#endif // IGVLOGWIDGET_H
