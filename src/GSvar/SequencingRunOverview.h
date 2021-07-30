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

protected slots:
	void delayedInitialization();
	void updateTable();
	void openRunTab();
	void openRunTab(int row);
	void editRun();
	void moveSamples();
	void addRun();

private:
	Ui::SequencingRunOverview ui_;
	DelayedInitializationTimer init_timer_;
};

#endif // SEQUENCINGRUNOVERVIEW_H
