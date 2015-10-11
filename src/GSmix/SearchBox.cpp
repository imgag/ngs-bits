#include "SearchBox.h"
#include "ui_SearchBox.h"

SearchBox::SearchBox(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SearchBox)
{
	ui->setupUi(this);

	connect(ui->button, SIGNAL(clicked(bool)), this, SLOT(clearText()));
	connect(ui->edit, SIGNAL(editingFinished()), this, SLOT(onTextChanged()));

	setFocusProxy(ui->edit);
}

SearchBox::~SearchBox()
{
	delete ui;
}

void SearchBox::clearText()
{
	ui->edit->clear();
	emit textEdited(ui->edit->text());
}

void SearchBox::onTextChanged()
{
	emit textEdited(ui->edit->text());
}
