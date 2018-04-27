#ifndef VARIANTSAMPLEOVERVIEWDIALOG_H
#define VARIANTSAMPLEOVERVIEWDIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include "ui_VariantSampleOverviewDialog.h"
#include "VariantList.h"


class VariantSampleOverviewDialog
	: public QDialog
{
	Q_OBJECT

public:
	VariantSampleOverviewDialog(const Variant& variant, QWidget *parent = 0);

private slots:
	void copyToClipboard();

private:
	Ui::VariantSampleOverviewDialog ui_;

	void addItem(int r, int c, QString text);
};

#endif // VARIANTSAMPLEOVERVIEWDIALOG_H
