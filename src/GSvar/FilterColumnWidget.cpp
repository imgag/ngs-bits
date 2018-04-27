#include "FilterColumnWidget.h"
#include "ui_FilterColumnWidget.h"

FilterColumnWidget::FilterColumnWidget(QString name, QString tooltip) :
	QWidget(),
	ui(new Ui::FilterColumnWidget)
{
	ui->setupUi(this);
	setObjectName(name);
	ui->name->setText(name);
	ui->name->setToolTip(tooltip);
	connect(ui->keep, SIGNAL(toggled(bool)), this, SLOT(keepClicked(bool)));
    connect(ui->remove, SIGNAL(toggled(bool)), this, SLOT(removeClicked(bool)));
    connect(ui->filter, SIGNAL(toggled(bool)), this, SIGNAL(stateChanged()));

	ui->remove->setStyleSheet("QCheckBox::indicator{width: 18px; height: 18px;} QCheckBox::indicator:checked{image: url(:/Icons/Remove.png);} QCheckBox::indicator:unchecked{image: url(:/Icons/Remove_gray.png);}");
	ui->keep->setStyleSheet("QCheckBox::indicator{width: 18px; height: 18px;} QCheckBox::indicator:checked{image: url(:/Icons/Keep.png);} QCheckBox::indicator:unchecked{image: url(:/Icons/Keep_gray.png);}");
    ui->filter->setStyleSheet("QCheckBox::indicator{width: 18px; height: 18px;} QCheckBox::indicator:checked{image: url(:/Icons/Filter.png);} QCheckBox::indicator:unchecked{image: url(:/Icons/Filter_gray.png);}");
}

FilterColumnWidget::~FilterColumnWidget()
{
	delete ui;
}

FilterColumnWidget::State FilterColumnWidget::state() const
{
	if (ui->remove->isChecked()) return REMOVE;
	if (ui->keep->isChecked()) return KEEP;
	return NONE;
}

void FilterColumnWidget::setState(FilterColumnWidget::State state)
{
	ui->remove->setChecked(state==REMOVE);
    ui->keep->setChecked(state==KEEP);
}

bool FilterColumnWidget::filter() const
{
    return ui->filter->isChecked();
}

void FilterColumnWidget::setFilter(bool checked)
{
    ui->filter->setChecked(checked);
}

void FilterColumnWidget::keepClicked(bool checked)
{
	if (checked)
	{
		ui->remove->setChecked(false);
	}

	emit stateChanged();
}

void FilterColumnWidget::removeClicked(bool checked)
{
	if (checked)
	{
		ui->keep->setChecked(false);
	}

    emit stateChanged();
}
