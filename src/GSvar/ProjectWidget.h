#ifndef PROJECTWIDGET_H
#define PROJECTWIDGET_H

#include "DelayedInitializationTimer.h"
#include <QWidget>

#include "ui_ProjectWidget.h"

class ProjectWidget
	: public QWidget
{
	Q_OBJECT

public:
	ProjectWidget(QWidget* parent, QString name);

signals:
	void openProcessedSampleTab(QString ps_name);

private slots:
	void delayedInitialization();
	void updateGUI();
	void showDiagnosticStatusDialog();

private:
	Ui::ProjectWidget ui_;
	DelayedInitializationTimer init_timer_;
	QString name_;
};

#endif // PROJECTWIDGET_H
