#ifndef DBTABLEADMINISTRATION_H
#define DBTABLEADMINISTRATION_H

#include <QWidget>

#include "ui_DBTableAdministration.h"

class DBTableAdministration
	: public QWidget
{
	Q_OBJECT

public:
	DBTableAdministration(QString table, QWidget* parent = 0);

protected slots:
	void updateTable();
	void add();
	void edit();
	void remove();

private:
	Ui::DBTableAdministration ui_;
	QString table_;
	QString table_display_name_;
};

#endif // DBTABLEADMINISTRATION_H
