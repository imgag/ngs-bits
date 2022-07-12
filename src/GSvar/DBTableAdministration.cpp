#include "DBTableAdministration.h"
#include "NGSD.h"
#include "DBEditor.h"
#include "GUIHelper.h"
#include "LoginManager.h"
#include "EmailDialog.h"
#include "GlobalServiceProvider.h"
#include "UserPermissionsEditor.h"
#include <QMessageBox>
#include <QAction>

DBTableAdministration::DBTableAdministration(QString table, QWidget* parent)
	: QWidget(parent)
	, ui_()
	, table_(table)
	, table_display_name_(table.replace("_", " "))
	, init_timer_(this, true)
{
	ui_.setupUi(this);

	connect(ui_.add_btn, SIGNAL(clicked(bool)), this, SLOT(add()));
	connect(ui_.edit_btn, SIGNAL(clicked(bool)), this, SLOT(edit()));
	connect(ui_.delete_btn, SIGNAL(clicked(bool)), this, SLOT(remove()));
	connect(ui_.text_filter_btn, SIGNAL(clicked(bool)), this, SLOT(updateTable()));
	connect(ui_.table, SIGNAL(rowDoubleClicked(int)), this, SLOT(edit(int)));

	QAction* action = new QAction(QIcon(":/Icons/Edit.png"), "Edit", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(edit()));

	if (table_=="user")
	{
		action = new QAction(QIcon(":/Icons/Lock.png"), "Edit permissions", this);
		ui_.table->addAction(action);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(changeUserPermissions()));
	}

	action = new QAction(QIcon(":/Icons/Remove.png"), "Delete", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(remove()));

	if (table_=="project")
	{
		action = new QAction(QIcon(":/Icons/NGSD_project.png"), "Open project tab(s)", this);
		ui_.table->addAction(action);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(openTabs()));
	}
	else if (table_=="processing_system")
	{
		action = new QAction(QIcon(":/Icons/NGSD_processing_system.png"), "Open processing system tab(s)", this);
		ui_.table->addAction(action);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(openTabs()));
	}
	else if (table_=="user")
	{
		action = new QAction("Reset user password", this);
		ui_.table->addAction(action);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(resetUserPassword()));
	}
}

void DBTableAdministration::delayedInitialization()
{
	updateTable();
}


void DBTableAdministration::updateTable()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//create table
	NGSD db;
	DBTable db_table = db.createOverviewTable(table_, ui_.text_filter->text());

	//show table
	if (table_=="sample")
	{
		QStringList quality_values = db_table.takeColumn(db_table.columnIndex("quality"));
		ui_.table->setData(db_table);
		ui_.table->setQualityIcons("name", quality_values);
	}
	else
	{
		ui_.table->setData(db_table);
	}

	QApplication::restoreOverrideCursor();
}

void DBTableAdministration::add()
{
	//add
	DBEditor* editor = new DBEditor(this, table_);
	auto dlg = GUIHelper::createDialog(editor, "Add " + table_display_name_, "", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		try
		{
			editor->store();
			updateTable();
		}
		catch (DatabaseException e)
		{
			QMessageBox::warning(this, "Error storing item", "Could not store the item.\n\nDatabase error:\n" + e.message());
		}
	}
}

void DBTableAdministration::edit()
{
	//check
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()!=1)
	{
		QMessageBox::information(this, "Selection error", "Please select exactly one item!");
		return;
	}


	edit(rows.toList().first());
}

void DBTableAdministration::edit(int row)
{
	//edit
	int id = ui_.table->getId(row).toInt();
	DBEditor* editor = new DBEditor(this, table_, id);
	auto dlg = GUIHelper::createDialog(editor, "Edit " + table_display_name_, "", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		try
		{
			editor->store();
			updateTable();
		}
		catch (DatabaseException e)
		{
			QMessageBox::warning(this, "Error storing item", "Could not store the item.\n\nDatabase error:\n" + e.message());
		}
	}
}

void DBTableAdministration::changeUserPermissions()
{
	//check
	try
	{
		//check selection
		QSet<int> rows = ui_.table->selectedRows();
		if (rows.count()!=1)
		{
			INFO(ArgumentException, "Please select exactly one user!");
		}

		//check user role
		QString user_id = ui_.table->getId(rows.values()[0]);
		QString user_role = NGSD().getValue("SELECT user_role FROM user WHERE id=" + user_id).toString().toLower();
		if (user_role!="user_restricted")
		{
			INFO(ArgumentException, "Setting permissions is availabe for the users with role 'user_restricted' only!");
		}

		//show dialog
		UserPermissionsEditor* widget = new UserPermissionsEditor("user_permissions", ui_.table->getId(rows.values()[0]), this);
		auto dlg = GUIHelper::createDialog(widget, "User permissions", "", false);
		dlg->exec();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Changing user permissions error");
	}
}

void DBTableAdministration::remove()
{
	//check
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()==0)
	{
		QMessageBox::information(this, "Selection error", "Please select at least one item!");
		return;
	}

	//confirm
	int btn = QMessageBox::information(this, "Confirm deleting", "You have selected " + QString::number(rows.count()) + " items.\nDo you really want to delete them?", QMessageBox::Yes, QMessageBox::Cancel);
	if (btn!=QMessageBox::Yes) return;

	//delete
	NGSD db;
	SqlQuery query = db.getQuery();
	try
	{
		foreach(int row, rows)
		{
			 query.exec("DELETE FROM " + table_ + " WHERE id=" + ui_.table->getId(row));
		}
	}
	catch (DatabaseException e)
	{
		QMessageBox::warning(this, "Error deleting item", "Could not delete an item!"
														  "\nThis is probably caused by the item being referenced from another table."
														  "\n\nDatabase error:\n" + e.message());
	}

	updateTable();
}

void DBTableAdministration::openTabs()
{
	QSet<int> rows = ui_.table->selectedRows();

	foreach(int row, rows)
	{
		QString id = ui_.table->getId(row);

		if (table_=="project")
		{
			QString name = NGSD().getValue("SELECT name FROM project WHERE id=" + id).toString();
			GlobalServiceProvider::openProjectTab(name);
		}
		else if (table_=="processing_system")
		{
			QString name = NGSD().getValue("SELECT name_short FROM processing_system WHERE id=" + id).toString();
			GlobalServiceProvider::openProcessingSystemTab(name);
		}
	}
}

void DBTableAdministration::resetUserPassword()
{
	//checks
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()!=1)
	{
		QMessageBox::information(this, "Selection error", "Please select exactly one item!");
		return;
	}

	//set password
	int user_id = ui_.table->getId(rows.toList().first()).toInt();
	QString password = Helper::randomString(15) + Helper::randomString(1, "0123456789");
	NGSD db;
	db.setPassword(user_id, password);

	//create email
	QString to = db.userEmail(user_id);
	QString subject = "[NGSD] Password reset";
	QStringList body;

	body << "Hello " + db.userName(user_id) + ",";
	body << "";
	body << "your new password for NGSD/GSvar is: " + password;
	body << "You can change the password in the GSvar main menu (NGSD > Admin > Change password).";
	body << "";
	body << "Best regards, ";
	body << "  " + LoginManager::userName();

	//send
	EmailDialog dlg(this, QStringList() << to, subject, body);
	dlg.exec();
}

