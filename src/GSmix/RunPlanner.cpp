#include "RunPlanner.h"
#include "ui_RunPlanner.h"
#include "NGSD.h"
#include "GDBO.h"
#include "Exceptions.h"
#include "Helper.h"
#include "GDBODialog.h"
#include <QTimer>
#include <QDebug>
#include <QSqlError>
#include <QPair>
#include <QMessageBox>


//get sample list with MIDs
struct SampleMIDs
{
	QString name;
	QString mid1;
	QString mid2;
};

RunPlanner::RunPlanner(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::RunPlanner)
{
	ui->setupUi(this);
	ui->samples->setColumnWidth(0, 150);
	ui->samples->setColumnWidth(1, 250);
	ui->samples->setColumnWidth(2, 250);

	connect(ui->debugButton, SIGNAL(clicked(bool)), this, SLOT(debug()));
	ui->debugButton->setVisible(false);
	connect(ui->run, SIGNAL(currentIndexChanged(int)), this, SLOT(runChanged(int)));
	connect(ui->lane, SIGNAL(valueChanged(int)), this, SLOT(laneChanged(int)));
	connect(ui->addButton, SIGNAL(clicked(bool)), this, SLOT(addItem()));
	connect(ui->removeButton, SIGNAL(clicked(bool)), this, SLOT(removeSelectedItems()));
	connect(ui->checkButton, SIGNAL(clicked(bool)), this, SLOT(checkForMidCollisions()));
	connect(ui->importButton, SIGNAL(clicked(bool)), this, SLOT(importNewSamplesToNGSD()));

	loadRunsFromNGSD();
}

RunPlanner::~RunPlanner()
{
	delete ui;
}

void RunPlanner::debug()
{
	ui->run->setCurrentIndex(5);

	int idx = ui->samples->rowCount();
	ui->samples->setRowCount(idx+2);

	ui->samples->setItem(idx, 0, readWriteItem("NA12878"));
	ui->samples->setItem(idx, 1, readWriteItem("Illumina 10 (TAGCTT)"));
	ui->samples->setItem(idx, 2, readWriteItem(""));
	++idx;

	ui->samples->setItem(idx, 0, readWriteItem("NA12878"));
	ui->samples->setItem(idx, 1, readWriteItem("Illumina 11 (GGCTAC)"));
	ui->samples->setItem(idx, 2, readWriteItem(""));

	importNewSamplesToNGSD();
}

void RunPlanner::loadRunsFromNGSD()
{
	SqlQuery q = DatabaseCache::inst().ngsd().getQuery();
	q.exec("SELECT id, name, fcid FROM sequencing_run ORDER BY name DESC");
	while(q.next())
	{
		QString name = q.value(1).toString();
		QString fcid = q.value(2).toString();
		ui->run->addItem(name + " (" + fcid + ")", q.value(0));
	}
}

void RunPlanner::runChanged(int)
{
	if(ui->run->currentIndex()==0) //[none]
	{
		ui->lane->setEnabled(false);
	}
	else
	{
		ui->lane->setValue(1);
		ui->lane->setEnabled(true);
	}

	updateRunData();
}

void RunPlanner::laneChanged(int)
{
	updateRunData();
}

void RunPlanner::addItem()
{
	//
	int idx = ui->samples->rowCount();
	ui->samples->setRowCount(idx+1);

	static int sample_num = 0;
	++sample_num;
	ui->samples->setItem(idx, 0, readWriteItem("sample" + QString::number(sample_num).rightJustified(3, '0')));
	ui->samples->setItem(idx, 1, readWriteItem(""));
	ui->samples->setItem(idx, 2, readWriteItem(""));
}

void RunPlanner::removeSelectedItems()
{
	//get list of rows
	QList<int> rows;
	QList<QTableWidgetSelectionRange> ranges = ui->samples->selectedRanges();
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
		ui->samples->removeRow(rows[i]);
	}
}

void RunPlanner::clearVisualOutput()
{
	QBrush brush;
	for(int r=0; r<ui->samples->rowCount(); ++r)
	{
		for (int c=0; c<3; ++c)
		{
			QTableWidgetItem* item = ui->samples->item(r, c);
			if(item==nullptr)
			{
				item = new QTableWidgetItem();
				ui->samples->setItem(r, c, item);
			}
			item->setBackground(brush);
			item->setToolTip("");
		}
	}
}

void RunPlanner::checkForMidCollisions()
{
	if (ui->samples->rowCount()==0) return;

	clearVisualOutput();

	QStringList output;

	QList<SampleMIDs> samples;
	for(int r=0; r<ui->samples->rowCount(); ++r)
	{
		QString name = ui->samples->item(r, 0)->text();
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
		QMessageBox::critical(this, "MID clashes detected!", output.join("\n"));
	}
	else
	{
		QMessageBox::information(this, "No MID clashes detected!", output.join("\n"));
	}
}

void RunPlanner::importNewSamplesToNGSD()
{
	if (ui->samples->rowCount()==0) return;

	//check that run [none] is not selected
	if (ui->run->currentIndex()==0)
	{
		QMessageBox::critical(this, "NGSD import", "No run selected for import!");
		return;
	}
	int run_id = ui->run->currentData(Qt::UserRole).toInt();
	int lane = ui->lane->value();

	//add samples to the run
	for(int r=0; r<ui->samples->rowCount(); ++r)
	{
		QTableWidgetItem* item = ui->samples->item(r, 0);
		if (item->flags() & Qt::ItemIsEditable)
		{
			try
			{
				GDBO ps("processed_sample");
				ps.setFK("sample_id", item->text());
				ps.set("sequencing_run_id", QString::number(run_id));
				ps.set("lane", QString::number(lane));
				ps.setFK("mid1_i7", midNameFromItem(r, 1));
				QString mid2 = midNameFromItem(r, 2);
				if (mid2!="")
				{
					ps.setFK("mid2_i5", mid2);
				}
				ps.set("operator_id", DatabaseCache::inst().ngsd().userId());

				GDBODialog dlg(this, ps, QStringList() << "process_id" << "last_analysis");
				dlg.setWindowTitle("Add processed sample to NGSD");
				if (dlg.exec())
				{
					ps.set("process_id", DatabaseCache::inst().ngsd().nextProcessingId(ps.get("sample_id")));
					ps.store();

					ui->samples->item(r, 0)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsDragEnabled);
					ui->samples->item(r, 1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsDragEnabled);
					ui->samples->item(r, 2)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsDragEnabled);
				}
				else
				{
					return;
				}
			}
			catch(Exception& e)
			{
				QMessageBox::critical(this, "NGSD import error", e.message());
				return;
			}
		}
	}

	//to update sample names (missing processing id)
	updateRunData();
}

QString RunPlanner::midSequenceFromItem(int row, int col)
{
	QString text = ui->samples->item(row, col)->text();
	QString mid = text.mid(text.lastIndexOf(' '));
	return mid.replace('(', ' ').replace(')',' ').trimmed();
}

QString RunPlanner::midNameFromItem(int row, int col)
{
	QString text = ui->samples->item(row, col)->text();
	QString mid = text.left(text.lastIndexOf(' '));
	return mid.trimmed();
}

void RunPlanner::highlightItem(int row, int col, QString tooltip)
{
	QTableWidgetItem* item = ui->samples->item(row, col);
	item->setBackgroundColor(Qt::yellow);

	QString old = item->toolTip().trimmed();
	if (!old.isEmpty()) old += "\n";
	item->setToolTip(old + tooltip);
}

void RunPlanner::updateRunData()
{
	ui->samples->clearContents();
	ui->samples->setRowCount(0);

	if (ui->run->currentIndex()==0) return; //[none]

	//load samples for run
	QString run_id = ui->run->currentData(Qt::UserRole).toString();
	QString lane = QString::number(ui->lane->value());

	QList<GDBO> psamples = GDBO::all("processed_sample", QStringList() << "sequencing_run_id='" + run_id + "'" << "lane='" + lane + "'");
	ui->samples->setRowCount(psamples.count());

	int row = 0;
	foreach(const GDBO& psample, psamples)
	{
		QString name = psample.getFkObject("sample_id").get("name") + "_" + psample.get("process_id").rightJustified(2, '0');
		ui->samples->setItem(row, 0, readOnlyItem(name));
		QString mid1 = psample.get("mid1_i7")=="" ? "" : midToString(psample.getFkObject("mid1_i7"));
		ui->samples->setItem(row, 1, readOnlyItem(mid1));
		QString mid2 = psample.get("mid2_i5")=="" ? "" : midToString(psample.getFkObject("mid2_i5"));
		ui->samples->setItem(row, 2, readOnlyItem(mid2));

		++row;
	}
}

QString RunPlanner::midToString(const GDBO& mid)
{
	return mid.get("name") + " (" + mid.get("sequence") + ")";
}

QList<int> RunPlanner::setToSortedList(const QSet<int>& set)
{
	QList<int> list = set.toList();
	std::sort(list.begin(), list.end());
	return list;
}

QTableWidgetItem* RunPlanner::readOnlyItem(QString text)
{
	QTableWidgetItem* item = new QTableWidgetItem(text);
	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsDragEnabled);
	return item;
}

QTableWidgetItem* RunPlanner::readWriteItem(QString text)
{
	QTableWidgetItem* item = new QTableWidgetItem(text);
	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled|Qt::ItemIsEnabled);
	return item;
}
