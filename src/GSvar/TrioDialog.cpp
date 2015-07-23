#include "TrioDialog.h"
#include "Settings.h"

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
	QString sys = db_.getProcessingSystem(value.trimmed(), NGSD::LONG);
	if (sys=="") sys = "unknown";
	ui_.f_sys->setText(sys);

	updateOkButton();
}

void TrioDialog::mother_changed(QString value)
{
	QString sys = db_.getProcessingSystem(value.trimmed(), NGSD::LONG);
	if (sys=="") sys = "unknown";
	ui_.m_sys->setText(sys);

	updateOkButton();
}

void TrioDialog::child_changed(QString value)
{
	QString sys = db_.getProcessingSystem(value.trimmed(), NGSD::LONG);
	if (sys=="") sys = "unknown";
	ui_.c_sys->setText(sys);

	updateOkButton();
}

void TrioDialog::updateOkButton()
{
	QString f = ui_.f_sys->text();
	QString m = ui_.m_sys->text();
	QString c = ui_.c_sys->text();
	ui_.ok_button->setEnabled(f!="" && m!="" && c!="" && f==m && f==c);
}
