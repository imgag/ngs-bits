#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "ui_SettingsDialog.h"

class SettingsDialog
	: public QDialog
{
	Q_OBJECT

public:
	SettingsDialog(QWidget* parent = nullptr);
	//Opens the spcified page. Does nothing if the page does not exist.
	void gotoPage(QString page_name);
	void storeSettings();

private slots:
	void loadSettings();
	void changePage();

private:
	Ui::SettingsDialog ui_;

	virtual void closeEvent(QCloseEvent *e) override;
};

#endif // SETTINGSDIALOG_H
