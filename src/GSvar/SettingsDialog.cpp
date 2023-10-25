#include "SettingsDialog.h"
#include "Settings.h"
#include <QDebug>

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

void SettingsDialog::changePage()
{
	ui_.title->setText(qobject_cast<QPushButton*>(sender())->text());

	QString page_name = sender()->objectName().replace("btn_", "page_");
	for (int i=0; i<ui_.stack->count(); ++i)
	{
		if (ui_.stack->widget(i)->objectName()==page_name)
		{
			ui_.stack->setCurrentIndex(i);
		}
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
}
