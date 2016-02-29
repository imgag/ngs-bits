#include "TrioDialog.h"
#include "Settings.h"
#include "Exceptions.h"

TrioDialog::TrioDialog(QWidget* parent)
	: QDialog(parent)
	, ui_()
	, db_()
{
	ui_.setupUi(this);\
	ui_.status_page_link->setText("<a href=\"" + Settings::string("SampleStatus") + "\"><span style=\"text-decoration: underline; color:#0000ff;\">[open status page]</span></a>");

	connect(ui_.f_ps, SIGNAL(textChanged(QString)), this, SLOT(father_changed(QString)));
	connect(ui_.m_ps, SIGNAL(textChanged(QString)), this, SLOT(mother_changed(QString)));
	connect(ui_.c_ps, SIGNAL(textChanged(QString)), this, SLOT(child_changed(QString)));

	updateOkButton();
}

QString TrioDialog::father()
{
	return ui_.f_ps->text().trimmed();
}

QString TrioDialog::mother()
{
	return ui_.m_ps->text().trimmed();
}

QString TrioDialog::child()
{
	return ui_.c_ps->text().trimmed();
}

void TrioDialog::father_changed(QString value)
{
	ui_.f_sys->setText(name2sys(value));
	updateOkButton();
}

void TrioDialog::mother_changed(QString value)
{
	ui_.m_sys->setText(name2sys(value));
	updateOkButton();
}

void TrioDialog::child_changed(QString value)
{
	ui_.c_sys->setText(name2sys(value));
	updateOkButton();
}

QString TrioDialog::name2sys(QString name)
{
	QString sys;
	try
	{
		sys = db_.getProcessingSystem(name.trimmed(), NGSD::LONG);
	}
	catch (Exception&)
	{
		sys = "unknown";
	}

	return sys;
}

void TrioDialog::updateOkButton()
{
	QString f = ui_.f_sys->text();
	QString m = ui_.m_sys->text();
	QString c = ui_.c_sys->text();
	ui_.ok_button->setEnabled(f!="" && m!="" && c!="" && f!="unknown" && m!="unknown" && c!="unknown" && f==m && f==c);
}
