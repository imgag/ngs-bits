#include "GapValidationLabel.h"
#include "ui_GapValidationLabel.h"
#include <QMenu>

GapValidationLabel::GapValidationLabel(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::GapValidationLabel)
{
	ui->setupUi(this);
	setState(CHECK);

	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));
}

GapValidationLabel::~GapValidationLabel()
{
	delete ui;
}

GapValidationLabel::State GapValidationLabel::state() const
{
	return state_;
}

void GapValidationLabel::setState(GapValidationLabel::State state)
{
	state_ = state;
	switch(state)
	{
		case CHECK:
			ui->icon->setPixmap(QPixmap(":/Icons/Question.png"));
			ui->text->setText("check in IGV");
			break;
		case VALIDATION:
			ui->icon->setPixmap(QPixmap(":/Icons/Ok.png"));
			ui->text->setText("validation");
			break;
		case NO_VALIDATION:
			ui->icon->setPixmap(QPixmap(":/Icons/Remove.png"));
			ui->text->setText("no validation");
			break;
	}
}

void GapValidationLabel::contextMenu(QPoint p)
{
	//set up
	QMenu menu;
	QAction* a_val = menu.addAction(QIcon(":/Icons/Ok.png"), "validation");
	QAction* a_chk = menu.addAction(QIcon(":/Icons/Question.png"), "check in IGV");
	QAction* a_rem = menu.addAction(QIcon(":/Icons/Remove.png"), "no validation");

	//exec
	QAction* action = menu.exec(mapToGlobal(p));
	if (action==nullptr) return;
	if (action==a_val) setState(VALIDATION);
	if (action==a_chk) setState(CHECK);
	if (action==a_rem) setState(NO_VALIDATION);
}
