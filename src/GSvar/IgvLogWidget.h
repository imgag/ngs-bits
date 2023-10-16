#ifndef IGVLOGWIDGET_H
#define IGVLOGWIDGET_H

#include <QWidget>
#include "ui_IgvLogWidget.h"
#include "IGVSession.h"

class IgvLogWidget
	: public QWidget
{
    Q_OBJECT

public:
    IgvLogWidget(QWidget *parent = 0);

public slots:
    void switchCurrentSession(int index);
	void updateTable(const QList<IGVCommand>& updated_history);

private:
    Ui::IgvLogWidget ui_;
};

#endif // IGVLOGWIDGET_H
