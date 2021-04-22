#ifndef PROCESSINGSYSTEMWIDGET_H
#define PROCESSINGSYSTEMWIDGET_H

#include <QWidget>
#include "ui_ProcessingSystemWidget.h"
#include "VariantList.h"
#include "DelayedInitializationTimer.h"

class ProcessingSystemWidget
	: public QWidget
{
	Q_OBJECT

public:
	ProcessingSystemWidget(QWidget* parent, int sys_id);

signals:
	void openProcessedSampleTab(QString);
	void openProcessedSampleFromNGSD(QString);
	void openGeneTab(QString);
	void executeIGVCommands(QStringList commands);

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
