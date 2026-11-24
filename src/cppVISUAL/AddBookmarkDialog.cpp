#include "AddBookmarkDialog.h"
#include "ui_AddBookmarkDialog.h"

AddBookmarkDialog::AddBookmarkDialog(const QString& name, QWidget* parent)
	: QDialog(parent)
	, ui_(new Ui::AddBookmarkDialog)
{
	ui_->setupUi(this);
	ui_->name->setText(name);
	ui_->name->selectAll();
}

AddBookmarkDialog::~AddBookmarkDialog()
{
	delete ui_;
}

QString AddBookmarkDialog::name() const
{
	return ui_->name->text().trimmed();
}

bool AddBookmarkDialog::showInChromosomeTrack() const
{
	return ui_->show_in_chromosome_track->isChecked();
}
