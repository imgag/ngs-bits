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
	void storeSettings();

private slots:
	void loadSettings();
	void changePage();

private:
	Ui::SettingsDialog ui_;
};

#endif // SETTINGSDIALOG_H
