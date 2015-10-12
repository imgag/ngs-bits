#ifndef RUNPLANNER_H
#define RUNPLANNER_H

#include <QWidget>
#include <QScopedPointer>

class NGSD;
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
	void delayedInizialization();
	void runChanged(int index);
	void laneChanged(int index);
	void addItem();
	void removeSelectedItems();
	void clearVisualOutput();
	void checkForMidCollisions();

private:
	QString itemMid(int row, int col);
	void highlightItem(int row, int col, QString tooltip);
	void updateRunData();
	static QList<int> setToSortedList(const QSet<int>& set);
	static QTableWidgetItem* readOnlyItem(QString text);
	static QTableWidgetItem* readWriteItem(QString text);

	Ui::RunPlanner* ui;
	QScopedPointer<NGSD> db;
};

#endif // RUNPLANNER_H
