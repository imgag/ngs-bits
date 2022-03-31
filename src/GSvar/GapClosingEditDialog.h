#ifndef GAPCLOSINGEDITDIALOG_H
#define GAPCLOSINGEDITDIALOG_H

#include <QDialog>
#include "ui_GapClosingEditDialog.h"
#include "NGSD.h"

//Edit dialog for a single gap
class GapClosingEditDialog
	: public QDialog
{
	Q_OBJECT

public:
	GapClosingEditDialog(QWidget* parent, int id);
	void store();

protected slots:
	void updateGUI();

private:
	Ui::GapClosingEditDialog ui_;
	int id_;
	NGSD db_;
};

#endif // GAPCLOSINGEDITDIALOG_H
