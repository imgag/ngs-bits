#ifndef SEQUENCINGRUNOVERVIEW_H
#define SEQUENCINGRUNOVERVIEW_H

#include <QWidget>
#include "ui_SequencingRunOverview.h"
#include "DelayedInitializationTimer.h"

class SequencingRunOverview
	: public QWidget
{
	Q_OBJECT

public:
	SequencingRunOverview(QWidget* parent = 0);

signals:
	void openRun(QString run_name);

protected slots:
	void delayedInitialization();
	void updateTable();
	void openRunTab();
	void editRun();
	void addRun();

private:
	Ui::SequencingRunOverview ui_;
	DelayedInitializationTimer init_timer_;
};

#endif // SEQUENCINGRUNOVERVIEW_H
