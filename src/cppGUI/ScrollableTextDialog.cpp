#include "ScrollableTextDialog.h"
#include "ui_ScrollableTextDialog.h"

ScrollableTextDialog::ScrollableTextDialog(QWidget *parent)
	: QDialog(parent)
	, ui_(new Ui::ScrollableTextDialog)
{
	ui_->setupUi(this);
}

ScrollableTextDialog::~ScrollableTextDialog()
{
	delete ui_;
}

void ScrollableTextDialog::setText(QString lines)
{
	ui_->text_browser->setText(lines);
}


