#ifndef USERACTIONPERMISSIONSEDITOR_H
#define USERACTIONPERMISSIONSEDITOR_H

#include <QWidget>
#include "NGSD.h"
#include "ui_UserActionPermissionsEditor.h"

class UserActionPermissionsEditor
	: public QWidget
{
	Q_OBJECT

public:
	UserActionPermissionsEditor(QString table, QString user_id, QWidget* parent = 0);

protected slots:
	void updateTable();

private:
	void clearServerCache();

	NGSD db_;
	Ui::DBTableActionPermissions ui_;
	QString table_;
	QString user_id_;
	QString table_display_name_;
};

#endif // USERACTIONPERMISSIONSEDITOR_H
