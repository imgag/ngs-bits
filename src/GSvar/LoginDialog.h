#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "ui_LoginDialog.h"

//NGSD login dialog
class LoginDialog
	: public QDialog
{
	Q_OBJECT

public:
	LoginDialog(QWidget *parent = 0);

	//Returns the user name that successfully logged in.
	QString userName() const;

	//Returns the user secure token needed to communicate with the server
	QString userToken() const;

protected slots:
	void clear();
	void checkPassword();

private:
	Ui::LoginDialog ui_;
	QString user_name_;
	QString user_token_;
};

#endif // LOGINDIALOG_H
