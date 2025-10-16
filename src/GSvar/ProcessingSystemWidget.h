#ifndef PROCESSINGSYSTEMWIDGET_H
#define PROCESSINGSYSTEMWIDGET_H

#include <QWidget>
#include "ui_ProcessingSystemWidget.h"
#include "DelayedInitializationTimer.h"
#include "TabBaseClass.h"

class ProcessingSystemWidget
	: public TabBaseClass
{
	Q_OBJECT

public:
	ProcessingSystemWidget(QWidget* parent, int sys_id);

private slots:
	void updateGUI();
	void delayedInitialization();
	void edit();

	void openRoiInIGV();

private:
	Ui::ProcessingSystemWidget ui_;
	DelayedInitializationTimer init_timer_;
	int sys_id_;

	void addItem(int r, int c, QString text);
	QList<int> selectedRows() const;
};

#endif // PROCESSINGSYSTEMWIDGET_H
