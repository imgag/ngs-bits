#ifndef DBTABLEADMINISTRATION_H
#define DBTABLEADMINISTRATION_H

#include <QWidget>
#include "DelayedInitializationTimer.h"
#include "ui_DBTableAdministration.h"

class DBTableAdministration
	: public QWidget
{
	Q_OBJECT

public:
	DBTableAdministration(QString table, QWidget* parent = 0);

signals:
	void openProjectTab(QString name);
	void openProcessingSystemTab(QString name_short);

protected slots:
	void delayedInitialization();
	void updateTable();
	void add();
	void edit();
	void edit(int row);
	void remove();
	void openTabs();

private:
	Ui::DBTableAdministration ui_;
	QString table_;
	QString table_display_name_;
	DelayedInitializationTimer init_timer_;
};

#endif // DBTABLEADMINISTRATION_H
