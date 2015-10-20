#ifndef RUNPLANNER_H
#define RUNPLANNER_H

#include <QWidget>
#include <QScopedPointer>
#include "GDBO.h"

class QTableWidgetItem;

namespace Ui {
class RunPlanner;
}

class RunPlanner
	: public QWidget
{
	Q_OBJECT

public:
	explicit RunPlanner(QWidget *parent = 0);
	~RunPlanner();

private slots:
	void debug();
	void runChanged(int index);
	void laneChanged(int index);
	void addItem();
	void removeSelectedItems();
	void clearVisualOutput();
	void checkForMidCollisions();
	void importNewSamplesToNGSD();

private:
	QString midSequenceFromItem(int row, int col);
	QString midNameFromItem(int row, int col);
	void highlightItem(int row, int col, QString tooltip);
	void updateRunData();
	QString midToString(const GDBO& mid);
	static QList<int> setToSortedList(const QSet<int>& set);
	static QTableWidgetItem* readOnlyItem(QString text);
	static QTableWidgetItem* readWriteItem(QString text);
	void loadRunsFromNGSD();

	Ui::RunPlanner* ui;
};

#endif // RUNPLANNER_H
