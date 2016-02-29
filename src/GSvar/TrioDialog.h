#ifndef TRIODIALOG_H
#define TRIODIALOG_H

#include <QDialog>
#include "ui_TrioDialog.h"
#include "NGSD.h"

///Dialog for selecting a trio.
class TrioDialog
		: public QDialog
{
	Q_OBJECT

public:
	///Constructor
	explicit TrioDialog(QWidget* parent = 0);

	///Return the processed sample name of the father
	QString father();
	///Return the processed sample name of the mother
	QString mother();
	///Return the processed sample name of the child
	QString child();

private slots:
	void father_changed(QString value);
	void mother_changed(QString value);
	void child_changed(QString value);

private:
	QString name2sys(QString name);
	Ui::TrioDialog ui_;
	NGSD db_;

	void updateOkButton();
};

#endif // TRIODIALOG_H
