#include "LoginDialog.h"
#include "GlobalServiceProvider.h"
#include "LoginManager.h"
#include "PasswordDialog.h"
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.ok_btn, SIGNAL(clicked(bool)), this, SLOT(checkPassword()));

	//init
	if (ui_.user_name->text().isEmpty()) ui_.user_name->setText(Helper::userName());
	ui_.password->setFocus();
}

void LoginDialog::showMessage(QString message, bool reset_password)
{
	ui_.message->setText("<font color='red'>" + message + "</font>");
	if (reset_password)
	{
		ui_.password->clear();
		ui_.password->setFocus();
	}
}

void LoginDialog::clear()
{
	ui_.password->clear();
}

void LoginDialog::checkPassword()
{
	QString user_name = ui_.user_name->text().trimmed();
	QString password = ui_.password->text().trimmed();
	QString message;

	try
	{		
		message = GlobalServiceProvider::database().checkPassword(user_name, password);
		if (!message.isEmpty())
		{			
			showMessage(message, true);
			return;
		}
	}
	catch(DatabaseException& e)
	{		
		showMessage(e.message());
		return;
	}

	try
	{
		LoginManager::login(user_name, password);

		// if the user has a single use initial password, a new password has to be set
		NGSD db;
		QDateTime last_login = db.userLastLogin(user_name);
		if (last_login.isNull())
		{
			QMessageBox::information(this, "Password reset", "You are using a temporary password. To be able to proceed, a new password (known only to you) has to be set.");
			PasswordDialog dlg(this);
			if(dlg.exec()==QDialog::Accepted)
			{
				password = dlg.password();
				db.setPassword(LoginManager::userId(), dlg.password());
				LoginManager::updateLastLogin(LoginManager::userId());
				// after setting the "last_login" field the user will not be asked to change the password
			}
			// the user refused to set a passoword -> not allowed to continue
			else LoginManager::logout();
		}
		accept();
    }
	catch (DatabaseException& e)
	{
		showMessage(e.message());
	}
    catch (Exception& e)
	{
		showMessage(e.message());
	}
}
