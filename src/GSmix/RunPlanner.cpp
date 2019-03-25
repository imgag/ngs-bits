#include "RunPlanner.h"
#include "ui_RunPlanner.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QTimer>
#include <QSqlError>
#include <QPair>
#include <QMessageBox>
#include <QClipboard>

//get sample list with MIDs
struct SampleMIDs
{
	QString name;
	QString mid1;
	QString mid2;
};

RunPlanner::RunPlanner(QWidget *parent) :
	QWidget(parent),
	ui_(new Ui::RunPlanner)
{
	ui_->setupUi(this);

	connect(ui_->run, SIGNAL(currentIndexChanged(int)), this, SLOT(runChanged(int)));
	connect(ui_->lane, SIGNAL(valueChanged(int)), this, SLOT(laneChanged(int)));
	connect(ui_->addButton, SIGNAL(clicked(bool)), this, SLOT(addItem()));
	connect(ui_->pasteButton, SIGNAL(clicked(bool)), this, SLOT(pasteItems()));
	connect(ui_->removeButton, SIGNAL(clicked(bool)), this, SLOT(removeSelectedItems()));
	connect(ui_->checkButton, SIGNAL(clicked(bool)), this, SLOT(checkForMidCollisions()));

	loadRunsFromNGSD();
}

RunPlanner::~RunPlanner()
{
	delete ui_;
}

void RunPlanner::loadRunsFromNGSD()
{
	SqlQuery q = db_.getQuery();
	q.exec("SELECT id, name, fcid FROM sequencing_run ORDER BY name DESC");
	while(q.next())
	{
		QString name = q.value(1).toString();
		QString fcid = q.value(2).toString();
		ui_->run->addItem(name + " (" + fcid + ")", q.value(0));
	}
}

void RunPlanner::runChanged(int)
{
	if(ui_->run->currentIndex()==0) //[none]
	{
		ui_->lane->setEnabled(false);
	}
	else
	{
		ui_->lane->setValue(1);
		ui_->lane->setEnabled(true);
	}

	updateRunData();
}

void RunPlanner::laneChanged(int)
{
	updateRunData();
}

void RunPlanner::addItem()
{
	static int sample_num = 0;
	++sample_num;
	QString name = "sample" + QString::number(sample_num).rightJustified(3, '0');

	ui_->samples->appendSampleRW(name);
}

void RunPlanner::pasteItems()
{
	ui_->samples->appendSamplesFromText(QApplication::clipboard()->text());
}

void RunPlanner::removeSelectedItems()
{
	//get list of rows
	QList<int> rows;
	QList<QTableWidgetSelectionRange> ranges = ui_->samples->selectedRanges();
	foreach(QTableWidgetSelectionRange range, ranges)
	{
		for (int i=range.topRow(); i<=range.bottomRow(); ++i)
		{
			rows.append(i);
		}
	}

	//delete them starting from the last index
	std::sort(rows.begin(), rows.end());

	for(int i=rows.count()-1; i>=0; --i)
	{
		ui_->samples->removeRow(rows[i]);
	}
}

void RunPlanner::clearVisualOutput()
{
	QBrush brush;
	for(int r=0; r<ui_->samples->rowCount(); ++r)
	{
		for (int c=0; c<3; ++c)
		{
			QTableWidgetItem* item = ui_->samples->item(r, c);
			if(item==nullptr)
			{
				item = new QTableWidgetItem();
				ui_->samples->setItem(r, c, item);
			}
			item->setBackground(brush);
			item->setToolTip("");
		}
	}
}

void RunPlanner::checkForMidCollisions()
{
	if (ui_->samples->rowCount()==0) return;

	clearVisualOutput();

	QStringList output;

	QList<SampleMIDs> samples;
	for(int r=0; r<ui_->samples->rowCount(); ++r)
	{
		QString name = ui_->samples->item(r, 0)->text();
		QString mid1 = midSequenceFromItem(r,1);
		QString mid2 = midSequenceFromItem(r,2);
		samples.append(SampleMIDs{name, mid1, mid2});
		if (name=="")
		{
			QMessageBox::critical(this, "Invalid sample row!", "No name given for sample in row " + QString::number(r) + "!");
			return;
		}
		if (mid1=="")
		{
			QMessageBox::critical(this, "Invalid sample row!", "No i7 MID given for sample " + name + "!");
			return;
		}
	}

	//determine MID lengths to create sample sheets for (smallest for each i7/i5)
	QSet<int> mid1_lengths;
	QSet<int> mid2_lengths;
	foreach(const SampleMIDs& s, samples)
	{
		mid1_lengths.insert(s.mid1.length());
		mid2_lengths.insert(s.mid2.length());
	}
	QList<int> mid1_sorted = setToSortedList(mid1_lengths);
	QList<int> mid2_sorted = setToSortedList(mid2_lengths);

	int mid1_len = mid1_sorted[0];
	int mid2_len1 = mid2_sorted[0];
	output << "MIDs lengths for demultiplexing: " + QString::number(mid1_len) + "+" + QString::number(mid2_len1);
	int mid2_len2 = -1;
	if (mid2_len1==0 && mid2_sorted.count()>1)
	{
		mid2_len2 = mid2_sorted[1];
		output << "Notice: Runs contains samples with different MID lengths!";
		output << "Additional MIDs lengths for demultiplexing: " + QString::number(mid1_len) + "+" + QString::number(mid2_len2);
	}
	output << "";

	//trim MIDs to maximum usable lengths
	int mid2_max = std::max(mid2_len1, mid2_len2);
	for(int i=0; i<samples.count(); ++i)
	{
		SampleMIDs& s = samples[i];
		s.mid1 = s.mid1.left(mid1_len);
		s.mid2 = s.mid2.left(mid2_max);
	}

	//check for MID clashes
	bool collisions_found = false;
	for(int i=0; i<samples.count(); ++i)
	{
		for(int j=i+1; j<samples.count(); ++j)
		{
			int dist1 = Helper::levenshtein(samples[i].mid1, samples[j].mid1);
			int dist2 = -1;
			if (samples[i].mid2!="" && samples[j].mid2!="")
			{
				dist2 = Helper::levenshtein(samples[i].mid2, samples[j].mid2);
			}

			if (dist1==0 && dist2<1)
			{
				collisions_found = true;

				//text output
				QString mid = samples[i].mid1;
				if (dist2>=0) mid += "+" + samples[i].mid2;
				QString n1 = samples[i].name;
				QString n2 = samples[j].name;
				output << "clash " + n1 + " <=> " + n2 + " (" + mid + ")";

				//gui output
				highlightItem(i, 0, n2 + " (" + mid + ")");
				highlightItem(j, 0, n1 + " (" + mid + ")");
			}
		}
	}

	if (collisions_found)
	{
		QMessageBox::critical(this, "MID collision check", "MID clashes detected!\n\n" +output.join("\n"));
	}
	else
	{
		QMessageBox::information(this, "MID collision check", "No MID clashes detected!\n\n" + output.join("\n"));
	}
}

QString RunPlanner::midSequenceFromItem(int row, int col)
{
	QString text = ui_->samples->item(row, col)->text();
	QString mid = text.mid(text.lastIndexOf(' '));
	return mid.replace('(', ' ').replace(')',' ').trimmed();
}

QString RunPlanner::midNameFromItem(int row, int col)
{
	QString text = ui_->samples->item(row, col)->text();
	QString mid = text.left(text.lastIndexOf(' '));
	return mid.trimmed();
}

void RunPlanner::highlightItem(int row, int col, QString tooltip)
{
	QTableWidgetItem* item = ui_->samples->item(row, col);
	item->setBackgroundColor(Qt::yellow);

	QString old = item->toolTip().trimmed();
	if (!old.isEmpty()) old += "\n";
	item->setToolTip(old + tooltip);
}

void RunPlanner::updateRunData()
{
	ui_->samples->clearContents();
	ui_->samples->setRowCount(0);

	if (ui_->run->currentIndex()==0) return; //[none]

	//load samples for run
	QString run_id = ui_->run->currentData(Qt::UserRole).toString();
	QString lane = QString::number(ui_->lane->value());

	SqlQuery query = db_.getQuery();
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), ps.mid1_i7, ps.mid2_i5 FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.sequencing_run_id='" + run_id + "' AND ps.lane LIKE '%" + lane + "%'");
	while(query.next())
	{
		QString name = query.value(0).toString();
		QString mid1 = query.value(1).isNull() ? "" : midToString(query.value(1).toString());
		QString mid2 = query.value(2).isNull() ? "" : midToString(query.value(2).toString());
		ui_->samples->appendSampleRO(name, mid1, mid2);
	}
}

QString RunPlanner::midToString(const QString& mid_id)
{
	QString name = db_.getValue("SELECT name FROM mid WHERE id='" + mid_id + "'").toString();
	QString sequence = db_.getValue("SELECT sequence FROM mid WHERE id='" + mid_id + "'").toString();
	return name + " (" + sequence + ")";
}

QList<int> RunPlanner::setToSortedList(const QSet<int>& set)
{
	QList<int> list = set.toList();
	std::sort(list.begin(), list.end());
	return list;
}
