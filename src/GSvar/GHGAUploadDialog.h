#ifndef GHGAUPLOADDIALOG_H
#define GHGAUPLOADDIALOG_H

#include <QDialog>
#include "ui_GHGAUploadDialog.h"

class GHGAUploadDialog
	: public QDialog
{
	Q_OBJECT

public:
	GHGAUploadDialog(QWidget *parent = nullptr);

private slots:
	void test();

private:
	Ui::GHGAUploadDialog ui_;
};

#endif // GHGAUPLOADDIALOG_H
