#include "SettingsDialog.h"
#include "Settings.h"
#include "Log.h"
#include <QDebug>
#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget *parent) :
	QDialog(parent),
	ui_()
{
	ui_.setupUi(this);
	ui_.stack->setCurrentIndex(0);

	//connect page buttons
	foreach(QPushButton* button, findChildren<QPushButton*>())
	{
		if (button->isFlat())
		{
			connect(button, SIGNAL(clicked(bool)), this, SLOT(changePage()));
		}
	}

	loadSettings();
}

void SettingsDialog::gotoPage(QString page_name)
{
	//empty page name > nothing to do
	page_name = page_name.remove("page_");
	page_name=page_name.trimmed();
	if (page_name.isEmpty()) return;

	//update title
	QPushButton* button = findChild<QPushButton*>("btn_"+page_name);
	ui_.title->setText(button->text());

	//update page
	for (int i=0; i<ui_.stack->count(); ++i)
	{
		if (ui_.stack->widget(i)->objectName()=="page_"+page_name)
		{
			ui_.stack->setCurrentIndex(i);
			return;
		}
	}
	Log::info("Settings page '" + page_name + "' not found!");
}

void SettingsDialog::changePage()
{
	QString page_name = sender()->objectName().replace("btn_", "");
	gotoPage(page_name);
}

void SettingsDialog::closeEvent(QCloseEvent* e)
{
	e->ignore();
	if (QMessageBox::Yes == QMessageBox::question(this, windowTitle(), "Do you want to exit without storing the changes?", QMessageBox::Yes | QMessageBox::No))
	{
		e->accept();
	}
}

void SettingsDialog::loadSettings()
{
	//general
	ui_.proxy_host->setText(Settings::string("proxy_host", true));
	ui_.proxy_port->setText(Settings::string("proxy_port", true));
	ui_.proxy_user->setText(Settings::string("proxy_user", true));
	ui_.proxy_password->setText(Settings::string("proxy_password", true));

	//IGV
	ui_.igv_default_small->setChecked(Settings::boolean("igv_default_small", true));
	ui_.igv_default_sv->setChecked(Settings::boolean("igv_default_sv", true));
	ui_.igv_default_lowcov->setChecked(Settings::boolean("igv_default_lowcov", true));

	//View
	if (Settings::string("view_adjust_large_numbers", true) == "raw_counts") ui_.view_adjust_large_numbers->setCurrentText( "raw counts");
	if (Settings::string("view_adjust_large_numbers", true) == "modifier") ui_.view_adjust_large_numbers->setCurrentText("T, G, M, k modifier");
	if (Settings::string("view_adjust_large_numbers", true) == "thousands_separator") ui_.view_adjust_large_numbers->setCurrentText( "use ',' as thousands separator");

	//Columns

}

void SettingsDialog::storeSettings()
{
	//general
	Settings::setString("proxy_host", ui_.proxy_host->text());
	Settings::setString("proxy_port", ui_.proxy_port->text());
	Settings::setString("proxy_user", ui_.proxy_user->text());
	Settings::setString("proxy_password", ui_.proxy_password->text());

	//IGV
	Settings::setBoolean("igv_default_small", ui_.igv_default_small->isChecked());
	Settings::setBoolean("igv_default_sv", ui_.igv_default_sv->isChecked());
	Settings::setBoolean("igv_default_lowcov", ui_.igv_default_lowcov->isChecked());

	//View
	if (ui_.view_adjust_large_numbers->currentText() == "raw counts") Settings::setString("view_adjust_large_numbers", "raw_counts");
	if (ui_.view_adjust_large_numbers->currentText() == "T, G, M, k modifier") Settings::setString("view_adjust_large_numbers", "modifier");
	if (ui_.view_adjust_large_numbers->currentText() == "use ',' as thousands separator") Settings::setString("view_adjust_large_numbers", "thousands_separator");

	//columns
	ui_.column_config->store();
}
