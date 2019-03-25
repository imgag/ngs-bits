#ifndef RUNPLANNER_H
#define RUNPLANNER_H

#include <QWidget>
#include <QScopedPointer>
#include "NGSD.h"

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
	void runChanged(int index);
	void laneChanged(int index);
	void addItem();
	void pasteItems();
	void removeSelectedItems();
	void clearVisualOutput();
	void checkForMidCollisions();

private:
	QString midSequenceFromItem(int row, int col);
	QString midNameFromItem(int row, int col);
	void highlightItem(int row, int col, QString tooltip);
	void updateRunData();
	QString midToString(const QString& mid_id);
	static QList<int> setToSortedList(const QSet<int>& set);
	void loadRunsFromNGSD();

	Ui::RunPlanner* ui_;
	NGSD db_;
};

#endif // RUNPLANNER_H
