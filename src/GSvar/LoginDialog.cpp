#include "LoginDialog.h"
#include "GlobalServiceProvider.h"

LoginDialog::LoginDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
	, user_name_()
{
	ui_.setupUi(this);
	connect(ui_.ok_btn, SIGNAL(clicked(bool)), this, SLOT(checkPassword()));

	//init
	if (ui_.user_name->text().isEmpty()) ui_.user_name->setText(Helper::userName().toLower());
	ui_.password->setFocus();
}

QString LoginDialog::userName() const
{
	return user_name_;
}

QString LoginDialog::password() const
{
	return password_;
}

void LoginDialog::clear()
{
	ui_.password->clear();
}

void LoginDialog::checkPassword()
{
	QString user_name = ui_.user_name->text().trimmed();
	QString password = ui_.password->text().trimmed();

	try
	{		
		QString message = GlobalServiceProvider::database().checkPassword(user_name, password);
		if (message.isEmpty())
		{
			user_name_ = user_name;
			password_ = password;
			accept();
		}
		else
		{
			ui_.message->setText("<font color='red'>" + message + "</font>");
			ui_.password->clear();
			ui_.password->setFocus();
		}
	}
	catch(DatabaseException& e)
	{
		ui_.message->setText("<font color='red'>" + e.message() + "</font>");
	}
}
