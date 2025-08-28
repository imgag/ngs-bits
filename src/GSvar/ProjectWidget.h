#ifndef PROJECTWIDGET_H
#define PROJECTWIDGET_H

#include "DelayedInitializationTimer.h"
#include <QWidget>
#include "TabBaseClass.h"
#include "ui_ProjectWidget.h"

class ProjectWidget
	: public TabBaseClass
{
	Q_OBJECT

public:
	ProjectWidget(QWidget* parent, QString name);

private slots:
	void delayedInitialization();
	void updateGUI();
	void edit();
	void openProcessedSampleTab(QString ps);

private:
	Ui::ProjectWidget ui_;
	DelayedInitializationTimer init_timer_;
	QString name_;
};

#endif // PROJECTWIDGET_H
