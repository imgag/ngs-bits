#ifndef USERPERMISSIONSEDITOR_H
#define USERPERMISSIONSEDITOR_H

#include <QWidget>
#include "DelayedInitializationTimer.h"
#include "NGSD.h"
#include "ui_UserPermissionsEditor.h"

// This class implements a user interface for changing user permissions: i.e. access to a specific project, project type, study, or sample.
// It also includes action permissions like changes the database, searching variants, running burden test, starting analysis jobs.
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
	void updateAccessPermissions();
	void addProjectAccessPermission();
	void addProjectTypeAccessPermission();
	void addStudyAccessPermission();
	void addSampleAccessPermission();
	void removeAccessPermission();
	void updateActionPermissions();

private:
	void clearServerCache();
	void createAddAccessPermissionDialog(QString table_name);
	void addAccessPermissionToDatabase(QString permission, QString data, QString display_text);
	NGSD db_;
	Ui::DBTablePermissions ui_;
	QString table_;
	QString user_id_;
	QString table_display_name_;
	DelayedInitializationTimer init_timer_;
};

#endif // USERPERMISSIONSEDITOR_H
