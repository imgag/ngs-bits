#include "MidCheckWidget.h"
#include "GUIHelper.h"
#include "Exceptions.h"
#include "Helper.h"
#include "DBSelector.h"
#include <QDialog>
#include <QMessageBox>

MidCheckWidget::MidCheckWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.length1, SIGNAL(valueChanged(int)), this, SLOT(checkMids()));
	connect(ui_.length2, SIGNAL(valueChanged(int)), this, SLOT(checkMids()));
	connect(ui_.add_btn, SIGNAL(clicked(bool)), this, SLOT(add()));
	connect(ui_.batch_btn, SIGNAL(clicked(bool)), this, SLOT(addBatch()));
	connect(ui_.run_btn, SIGNAL(clicked(bool)), this, SLOT(addRun()));
	setMinimumSize(800, 800);

	//get MIDs from NGSD
	ui_.add_mid1->fill(db_.createTable("mid", "SELECT id, name FROM mid"));
	ui_.add_mid2->fill(db_.createTable("mid", "SELECT id, name FROM mid"));
}

const QList<SampleMids> MidCheckWidget::mids() const
{
	return mids_;
}

void MidCheckWidget::setParameters(QPair<int, int> lengths)
{
	ui_.length1->setValue(lengths.first);
	ui_.length2->setValue(lengths.second);

	checkMids();
}

void MidCheckWidget::addRun(QString run_name)
{
	try
	{
		//load MIDs
		QList<SampleMids> run_mids;
		QString run_id = db_.getValue("SELECT id FROM sequencing_run WHERE name=:0", false, run_name).toString();
		SqlQuery query = db_.getQuery();
		query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), ps.lane, ps.mid1_i7, ps.mid2_i5 FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.sequencing_run_id='" + run_id + "' ORDER BY ps.lane ASC, ps.id ASC");
		while(query.next())
		{
			SampleMids sample_mids;

			sample_mids.name = query.value(0).toString();

			QStringList lanes = query.value(1).toString().split(',');
			foreach(QString lane, lanes)
			{
				lane = lane.trimmed();
				if (lane.isEmpty()) continue;
				sample_mids.lanes << Helper::toInt(lane, "Lane");
			}

			if (query.value(2).isNull()) THROW(ArgumentException, "MID 1 unset for sample " + sample_mids.name);
			QString id = query.value(2).toString();
			sample_mids.mid1_name = db_.getValue("SELECT name FROM mid WHERE id=:0", false, id).toString();
			sample_mids.mid1_seq = db_.getValue("SELECT sequence FROM mid WHERE id=:0", false, id).toString();

			if (!query.value(3).isNull())
			{
				id = query.value(3).toString();
				sample_mids.mid2_name = db_.getValue("SELECT name FROM mid WHERE id=:0", false, id).toString();
				sample_mids.mid2_seq = db_.getValue("SELECT sequence FROM mid WHERE id=:0", false, id).toString();
			}

			run_mids << sample_mids;
		}
		mids_ = run_mids;
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Run import", "Error: MID run import could not be performed:\n" + e.message());
	}

	checkMids();
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
		ui_.samples->setItem(i, 0, GUIHelper::createTableItem(mid.name));
		ui_.samples->setItem(i, 1, GUIHelper::createTableItem(mid.lanesAsString()));
		ui_.samples->setItem(i, 2, GUIHelper::createTableItem(mid.mid1_name + " (" + mid.mid1_seq + ")"));
		ui_.samples->setItem(i, 3, GUIHelper::createTableItem(mid.mid2_seq.isEmpty() ? "" : mid.mid2_name + " (" + mid.mid2_seq + ")"));
	}
	GUIHelper::resizeTableCellWidths(ui_.samples);
	GUIHelper::resizeTableCellHeightsToFirst(ui_.samples);
}

void MidCheckWidget::checkMids()
{
	//clear
	updateSampleTable();

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
            ui_.samples->item(clash.s1_index, c)->setBackground(QBrush(QColor(Qt::yellow)));
            ui_.samples->item(clash.s2_index, c)->setBackground(QBrush(QColor(Qt::yellow)));
		}
	}
}

void MidCheckWidget::add()
{
	//parse input
	try
	{
		QString line = ui_.add_sample->text() + "\t" +  ui_.add_lanes->text() + "\t" +  ui_.add_mid1->text() + "\t" + ui_.add_mid2->text();
		mids_ << parseImportLine(line);
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Pasing MID data failed", "Message:\n" + e.message());
	}

	//update GUI
	checkMids();
}

void MidCheckWidget::addBatch()
{
	//get user input
	QTextEdit* edit = new QTextEdit(this);
	edit->setAcceptRichText(false);
	edit->setMinimumSize(600, 400);
	auto dlg = GUIHelper::createDialog(edit, "Batch MID import", "Enter sample MID data. Paste four tab-separated columnns, e.g. from Excel:<br>sample name, lane(s), mid1, mid2<br>Note: lanes must be comma-separated.<br>Note: MIDs can be given by sequence or by their name in the NGSD.", true);
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

			sample_mids << parseImportLine(line, line_nr);
		}
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Pasing MID data failed", "Message:\n" + e.message());
	}

	//add data
	mids_ << sample_mids;

	//update GUI
	checkMids();
}

void MidCheckWidget::addRun()
{
	//create
	DBSelector* selector = new DBSelector(this);
	selector->fill(db_.createTable("sequencing_run", "SELECT id, name FROM sequencing_run"));

	//show
	auto dlg = GUIHelper::createDialog(selector, "Select sequencing run", "run:", true);
	if (dlg->exec()==QDialog::Rejected) return ;

	//add and check
	addRun(selector->text());
}

SampleMids MidCheckWidget::parseImportLine(QString line, int line_nr)
{
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
		mid.lanes << Helper::toInt(lane, "Lane in line" + QString::number(line_nr) +". Note: lanes must be comma-separated!");
	}

	QString mid1 = parts[2].trimmed();
	if (mid1.isEmpty())
	{
		THROW(ArgumentException, "Error: MID 1 in line " + QString::number(line_nr) + " is empty.\n\nLine:\n" + line);
	}
	else if (mid1.contains(QRegularExpression("^[ACGTacgt]+$")))
	{
		mid.mid1_name = "no name";
		mid.mid1_seq = mid1.toUpper();
	}
	else //MID is a name > import from NGSD
	{
		mid.mid1_name = mid1;
		mid.mid1_seq = db_.getValue("SELECT sequence FROM mid WHERE name=:0", false, mid1).toString();
	}

	QString mid2 = parts[3].trimmed();
	if (mid2.contains(QRegularExpression("^[ACGTacgt]+$")))
	{
		mid.mid2_name = "no name";
		mid.mid2_seq = mid2.toUpper();
	}
	else if (!mid2.isEmpty())//MID is a name > import from NGSD
	{
		mid.mid2_name = mid2;
		mid.mid2_seq = db_.getValue("SELECT sequence FROM mid WHERE name=:0", false, mid2).toString();
	}

	return mid;
}
