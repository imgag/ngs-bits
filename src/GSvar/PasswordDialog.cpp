#include "PasswordDialog.h"
#include "LoginManager.h"

PasswordDialog::PasswordDialog(QWidget* parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);

	connect(ui_.old, SIGNAL(textChanged(QString)), this, SLOT(checkValid()));
	connect(ui_.new1, SIGNAL(textChanged(QString)), this, SLOT(checkValid()));
	connect(ui_.new2, SIGNAL(textChanged(QString)), this, SLOT(checkValid()));
}

QString PasswordDialog::password() const
{
	if (result()==QDialog::Rejected)
	{
		THROW(ProgrammingException, "Cannot call PasswordDialog::password() when the dialog was rejected!");
	}

	return ui_.new1->text().trimmed();
}

void PasswordDialog::checkValid()
{
	ui_.ok_btn->setEnabled(false);

	bool ok = true;

	//check old password
	QString msg_old = db_.checkPassword(LoginManager::userLogin(), ui_.old->text().trimmed());
	if (!msg_old.isEmpty())
	{
		ui_.old->setToolTip("The given password is not correct!");
		ui_.old->setStyleSheet("QLineEdit {border: 2px solid red;}");
		ok = false;
	}
	else
	{
		ui_.old->setStyleSheet("");
	}

	//check new password
	QStringList msg_new = db_.checkValue("user", "password", ui_.new1->text().trimmed(), false);
	if (!msg_new.isEmpty())
	{
		ui_.new1->setToolTip(msg_new.join("\n"));
		ui_.new1->setStyleSheet("QLineEdit {border: 2px solid red;}");
		ok = false;
	}
	else
	{
		ui_.new1->setStyleSheet("");
	}

	//check repeat password
	if (ui_.new1->text().trimmed()!=ui_.new2->text().trimmed())
	{
		ui_.new2->setToolTip("Repeated password does not match the original password!");
		ui_.new2->setStyleSheet("QLineEdit {border: 2px solid red;}");
		ok = false;
	}
	else
	{
		ui_.new2->setStyleSheet("");
	}

	//activate 'ok' button
	if (ok) ui_.ok_btn->setEnabled(true);
}
