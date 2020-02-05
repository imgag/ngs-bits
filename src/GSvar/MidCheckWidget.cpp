#include "MidCheckWidget.h"
#include "GUIHelper.h"

MidCheckWidget::MidCheckWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);

	updateSampleTable();
}

void MidCheckWidget::setParameters(QPair<int, int> lengths)
{
	ui_.length1->setValue(lengths.first);
	ui_.length2->setValue(lengths.second);
}

void MidCheckWidget::setMids(const QList<SampleMids>& mids)
{
	mids_ = mids;
	updateSampleTable();
}

void MidCheckWidget::updateSampleTable()
{
	//clear
	ui_.samples->setRowCount(0);

	//set
	ui_.samples->setRowCount(mids_.count());
	for(int i=0; i<mids_.count(); ++i)
	{
		const SampleMids& mid = mids_[i];
		ui_.samples->setItem(i, 0, createItem(mid.name));
		ui_.samples->setItem(i, 1, createItem(mid.lanesAsString()));
		ui_.samples->setItem(i, 2, createItem(mid.mid1_name + " (" + mid.mid1_seq + ")"));
		ui_.samples->setItem(i, 3, createItem(mid.mid2_seq.isEmpty() ? "" : mid.mid2_name + " (" + mid.mid2_seq + ")"));
	}
	GUIHelper::resizeTableCells(ui_.samples);
}

QTableWidgetItem*MidCheckWidget::createItem(const QString& text, int alignment)
{
	auto item = new QTableWidgetItem();
	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

	item->setText(text);

	item->setTextAlignment(alignment);

	return item;
}
