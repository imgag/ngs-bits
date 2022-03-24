#ifndef DBPERMISSIONSEDITOR_H
#define DBPERMISSIONSEDITOR_H

#include <QWidget>
#include "ui_DBPermissionsEditor.h"
#include "NGSD.h"

class DBPermissionsEditor : public QWidget
{
	Q_OBJECT

public:
	DBPermissionsEditor(QString user_id, QWidget *parent = 0);
	void store();

protected slots:
	void permissionTextChanged(QString value);
	void permissionIndexChanged(int index);
	void dataChanged(QString value);
	void dataIndexChanged(int index);

protected:
	QStringList getEnumList(QString table_name, QString filed_name);
	void createDataList(bool is_enabled);

private:
	Ui::DBPermissionsEditor ui_;
	QString user_id_;
	NGSD db_;
	QWidget* data_widget_;
	QStringList permission_types_;
	QStringList data_values_;
	QString selected_permission_;
	QString selected_data_;
};

#endif // DBPERMISSIONSEDITOR_H
