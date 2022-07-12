#ifndef USERPERMISSIONSEDITOR_H
#define USERPERMISSIONSEDITOR_H

#include <QWidget>
#include "DelayedInitializationTimer.h"
#include "NGSD.h"
#include "ui_UserPermissionsEditor.h"

// This class implements a user interface for changing user permissions: i.e. access to a specific project, project type, study, or sample.
// Setting such permissions is only available for the users with the role 'user_restricted'.
// User permissions are necessary to restirct the access while working in a client-server mode.
class UserPermissionsEditor
	: public QWidget
{
	Q_OBJECT

public:
	UserPermissionsEditor(QString table, QString user_id, QWidget* parent = 0);

protected slots:
	void delayedInitialization();
	void updateTable();
	void addProjectPermission();
	void addProjectTypePermission();
	void addStudyPermission();
	void addSamplePermission();
	void remove();

private:
	void createAddPermissionDialog(QString table_name);
	void addPermissionToDatabase(QString permission, QString data, QString display_text);
	NGSD db_;
	Ui::DBTablePermissions ui_;
	QString table_;
	QString user_id_;
	QString table_display_name_;
	DelayedInitializationTimer init_timer_;
};

#endif // USERPERMISSIONSEDITOR_H
