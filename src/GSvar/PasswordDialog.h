#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QDialog>
#include "ui_PasswordDialog.h"
#include "NGSD.h"

///Dialog for changing the NGSD password
class PasswordDialog
	: public QDialog
{
	Q_OBJECT

public:
	PasswordDialog(QWidget* parent = 0);
	QString password() const;

protected slots:
	void checkValid();

private:
	Ui::PasswordDialog ui_;
	NGSD db_;
};

#endif // PASSWORDDIALOG_H
