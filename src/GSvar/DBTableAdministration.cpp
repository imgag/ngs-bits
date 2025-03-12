#include "DBTableAdministration.h"
#include "NGSD.h"
#include "DBEditor.h"
#include "GUIHelper.h"
#include "LoginManager.h"
#include "EmailDialog.h"
#include "GlobalServiceProvider.h"
#include "UserPermissionsEditor.h"
#include "GenLabDB.h"
#include "ScrollableTextDialog.h"
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
	else if (table_=="study")
	{
		action = new QAction(QIcon(":/Icons/GenLab.png"), "Import samples from GenLab", this);
		ui_.table->addAction(action);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(importStudySamples()));
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


    edit(rows.values().first());
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
		int user_id = ui_.table->getId(rows.values()[0]).toInt();
		QString user_role = NGSD().getUserRole(user_id);
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
    int user_id = ui_.table->getId(rows.values().first()).toInt();
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
	body << "Your login is: " + db.userLogin(user_id);
	body << "";
	body << "Best regards, ";
	body << "  " + LoginManager::userName();

	//send
	EmailDialog dlg(this, QStringList() << to, subject, body);
	dlg.exec();
}

void DBTableAdministration::importStudySamples()
{
	//checks
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()!=1)
	{
		QMessageBox::information(this, "Selection error", "Please select exactly one study!");
		return;
	}

	QApplication::setOverrideCursor(Qt::BusyCursor);

	//get study ID and name
    int study_id = ui_.table->getId(rows.values().first()).toInt();
	NGSD db;
	QString study = db.getValue("SELECT name FROM study WHERE id='"+QString::number(study_id)+"'", false).toString();

	//import study samples from GenLab
	int c_imported = 0;
	QStringList errors;
	GenLabDB genlab;
	QList<int> ps_ids = genlab.studySamples(study, errors);
	foreach(int ps_id, ps_ids)
	{
		//check if persent
		int present = db.getValue("SELECT count(*) FROM study_sample WHERE study_id='"+QString::number(study_id)+"' AND processed_sample_id='"+QString::number(ps_id)+"'").toInt();

		if (present==0)
		{
			++c_imported;
			SqlQuery query = db.getQuery();
			query.exec("INSERT INTO study_sample (study_id, processed_sample_id, study_sample_idendifier) VALUES ("+QString::number(study_id)+", "+QString::number(ps_id)+", 'GenLab import by "+Helper::userName()+" on "+QDate::currentDate().toString()+"')");
		}
	}

	QApplication::restoreOverrideCursor();

	//output
	ScrollableTextDialog dlg(this, "Study import from GenLab");
	dlg.appendLine("Imported study samples: " + QString::number(c_imported));
	dlg.appendLine("");
	dlg.appendLine("Errors:");
	dlg.appendLines(errors);
	dlg.exec();
}

