#include "LoginDialog.h"
#include "GlobalServiceProvider.h"
#include "LoginManager.h"
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.ok_btn, SIGNAL(clicked(bool)), this, SLOT(checkPassword()));

	//init
	if (ui_.user_name->text().isEmpty()) ui_.user_name->setText(Helper::userName().toLower());
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
		accept();

	}  catch (Exception& e)
	{
		showMessage(e.message());
	}
}
