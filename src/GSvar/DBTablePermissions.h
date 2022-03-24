#ifndef DBTABLEPERMISSIONS_H
#define DBTABLEPERMISSIONS_H

#include <QWidget>
#include "DelayedInitializationTimer.h"
#include "NGSD.h"
#include "ui_DBTablePermissions.h"


class DBTablePermissions : public QWidget
{
	Q_OBJECT

public:
	DBTablePermissions(QString table, QString user_id, QWidget* parent = 0);

protected slots:
	void delayedInitialization();
	void updateTable();
	void add();
	void remove();

private:
	Ui::DBTablePermissions ui_;
	QString table_;
	QString user_id_;
	QString table_display_name_;
	DelayedInitializationTimer init_timer_;
};

#endif // DBTABLEPERMISSIONS_H
