#include "MidCheckWidget.h"
#include "GUIHelper.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Settings.h"
#include <QDebug>
#include <QDialog>
#include <QMessageBox>

MidCheckWidget::MidCheckWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.check_btn, SIGNAL(clicked(bool)), this, SLOT(checkMids()));
	connect(ui_.add_btn, SIGNAL(clicked(bool)), this, SLOT(add()));
	connect(ui_.add_batch_btn, SIGNAL(clicked(bool)), this, SLOT(addBatch()));
	setMinimumSize(600, 800);

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

void MidCheckWidget::checkMids()
{
	//check
	QStringList messages;
	QList<MidClash> clashes = MidCheck::check(mids_, ui_.length1->value(), ui_.length2->value(), messages);

	//show output
	ui_.output->setText(messages.join("\n"));

	//mark clashed samples
	foreach(MidClash clash, clashes)
	{
		for (int c=0; c<ui_.samples->columnCount(); ++c)
		{
			ui_.samples->item(clash.s1_index, c)->setBackgroundColor(Qt::yellow);
			ui_.samples->item(clash.s2_index, c)->setBackgroundColor(Qt::yellow);
		}
	}
}

void MidCheckWidget::add()
{
	//TODO

/*
s1	1	ACGTACGT
s2	1	ACGTACGG
s3	2	ACGTACGT
s4	2	ACGTACGT	CTGACTGA
s4	3	ACGTACGG	CTGACTGA
s5	3	ACGTACGG	CTGACTGG
s6	3	ACGTACGA	CTGACTGA
s7	4	ACGTACGA	CTGACTGA
s8	4	ACGTACGA	CTGACTGA
*/
}

void MidCheckWidget::addBatch()
{
	//get user input
	QTextEdit* edit = new QTextEdit(this);
	edit->setAcceptRichText(false);
	edit->setMinimumSize(600, 400);
	auto dlg = GUIHelper::createDialog(edit, "Batch MID import", "Enter sample MID data. Paste four tab-separated columnns:\nsample name, lane(s), mid1, mid2", true);
	if (dlg->exec()!=QDialog::Accepted) return;

	//parse input
	QList<SampleMids> sample_mids;
	int line_nr = 0;
	try
	{
		QStringList lines = edit->toPlainText().split("\n");
		foreach(QString line, lines)
		{
			++line_nr;

			if (line.trimmed().isEmpty() || line[0]=='#') continue;

			QStringList parts = line.split("\t");

			//check tab-separated parts count
			if (parts.count()!=4) THROW(ArgumentException, "Error: line " + QString::number(line_nr) + " does not contain 4 tab-separated parts.\n\nLine:\n" + line);

			SampleMids mid;
			mid.name = parts[0].trimmed();

			QStringList lanes = parts[1].split(',');
			foreach(QString lane, lanes)
			{
				lane = lane.trimmed();
				if (lane.isEmpty()) continue;
				mid.lanes << Helper::toInt(lane, "Lane in line" + QString::number(line_nr));
			}

			QString mid1 = parts[2].trimmed();
			if (mid1.contains(QRegularExpression("^[ACGT]+$")))
			{
				mid.mid1_name = "no name";
				mid.mid1_seq = mid1;
			}
			else //MID is a name > import from NGSD
			{
				if (!Settings::boolean("NGSD_enabled", true)) THROW(DatabaseException, "NGSD is not enabled. MIDs can only be given by sequence");
				//TODO
			}

			QString mid2 = parts[3].trimmed();
			if (mid2.contains(QRegularExpression("^[ACGT]+$")))
			{
				mid.mid2_name = "no name";
				mid.mid2_seq = mid2;
			}
			else //MID is a name > import from NGSD
			{
				if (!Settings::boolean("NGSD_enabled", true)) THROW(DatabaseException, "NGSD is not enabled. MIDs can only be given by sequence");
				//TODO
			}

			sample_mids << mid;
		}
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Pasing MID data failed", "Message:\n" + e.message());
	}

	//add data
	mids_ << sample_mids;
	updateSampleTable();
}

QTableWidgetItem*MidCheckWidget::createItem(const QString& text, int alignment)
{
	auto item = new QTableWidgetItem();
	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

	item->setText(text);

	item->setTextAlignment(alignment);

	return item;
}
