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

protected:
	void showMessage(QString message, bool reset_password = false);

protected slots:
	void clear();
	void checkPassword();

private:
	Ui::LoginDialog ui_;
};

#endif // LOGINDIALOG_H
