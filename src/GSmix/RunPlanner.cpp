#include "RunPlanner.h"
#include "ui_RunPlanner.h"
#include "NGSD.h"
#include "MidCache.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QTimer>
#include <QDebug>
#include <QSqlError>
#include <QPair>
#include <QMessageBox>

RunPlanner::RunPlanner(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::RunPlanner)
{
	ui->setupUi(this);
	ui->samples->setColumnWidth(0, 150);
	ui->samples->setColumnWidth(1, 250);
	ui->samples->setColumnWidth(2, 250);

	connect(ui->run, SIGNAL(currentIndexChanged(int)), this, SLOT(runChanged(int)));
	connect(ui->lane, SIGNAL(valueChanged(int)), this, SLOT(laneChanged(int)));
	connect(ui->addButton, SIGNAL(clicked(bool)), this, SLOT(addItem()));
	connect(ui->removeButton, SIGNAL(clicked(bool)), this, SLOT(removeSelectedItems()));
	connect(ui->checkButton, SIGNAL(clicked(bool)), this, SLOT(checkForMidCollisions()));

	//init delayed initialization
	QTimer* timer = new QTimer(this);
	timer->setSingleShot(true);
	timer->start(50);
	connect(timer, SIGNAL(timeout()), this, SLOT(delayedInizialization()));
}

RunPlanner::~RunPlanner()
{
	delete ui;
}

void RunPlanner::delayedInizialization()
{
	//init NGSD instance
	db.reset(new NGSD());

	//load run list
	QSqlQuery q = db->getQuery();
	if (q.exec("SELECT id, name, fcid FROM sequencing_run ORDER BY name DESC"))
	{
		while(q.next())
		{
			QString name = q.value(1).toString();
			QString fcid = q.value(2).toString();
			ui->run->addItem(name + " (" + fcid + ")", q.value(0));
		}
	}
	else
	{
		THROW(DatabaseException, q.lastError().text());
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
		ui->samples->item(r, 0)->setBackground(brush);
		ui->samples->item(r, 0)->setToolTip("");
		ui->samples->item(r, 1)->setBackground(brush);
		ui->samples->item(r, 1)->setToolTip("");
		ui->samples->item(r, 2)->setBackground(brush);
		ui->samples->item(r, 2)->setToolTip("");
	}
}

void RunPlanner::checkForMidCollisions()
{
	if (ui->samples->rowCount()==0) return;

	clearVisualOutput();

	QStringList output;

	//get sample list with MIDs
	struct SampleMIDs
	{
		QString name;
		QString mid1;
		QString mid2;
	};
	QList<SampleMIDs> samples;
	for(int r=0; r<ui->samples->rowCount(); ++r)
	{
		QString name = ui->samples->item(r, 0)->text();
		QString mid1 = itemMid(r,1);
		QString mid2 = itemMid(r,2);
		samples.append(SampleMIDs{name, mid1, mid2});
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
		output << "Additional MIDs lengths for demultiplexing: " + QString::number(mid1_len) + "+" + QString::number(mid2_len1);
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

QString RunPlanner::itemMid(int row, int col)
{
	QString text = ui->samples->item(row, col)->text();
	QString mid = text.mid(text.lastIndexOf(' '));
	return mid.replace('(', ' ').replace(')',' ').trimmed();
}

void RunPlanner::highlightItem(int row, int col, QString tooltip)
{
	QTableWidgetItem* item = ui->samples->item(row, col);
	item->setBackgroundColor(Qt::yellow);
	item->setToolTip(item->toolTip() + tooltip + "\n");
}

void RunPlanner::updateRunData()
{
	ui->samples->clearContents();
	ui->samples->setRowCount(0);

	if (ui->run->currentIndex()==0) return; //[none]

	//load samples for run
	QString run_id = ui->run->currentData(Qt::UserRole).toString();
	QString lane = QString::number(ui->lane->value());
	QSqlQuery q = db->getQuery();

	int row = 0;
	const MidCache& mid_cache = MidCache::inst();
	if (q.exec("SELECT ps.id, s.name, ps.process_id, ps.mid1_i7, ps.mid2_i5 FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.sequencing_run_id='" + run_id + "' AND lane='" + lane + "'"))
	{
		ui->samples->setRowCount(q.numRowsAffected());

		while(q.next())
		{
			QString sample_name = q.value(1).toString() + "_" + q.value(2).toString().rightJustified(2, '0');
			ui->samples->setItem(row, 0, readOnlyItem(sample_name));

			QVariant value = q.value(3);
			QString mid1 = value.isNull() ? "" : mid_cache.midById(value.toInt()).toString();
			ui->samples->setItem(row, 1, readOnlyItem(mid1));

			value = q.value(4);
			QString mid2 = value.isNull() ? "" : mid_cache.midById(value.toInt()).toString();
			ui->samples->setItem(row, 2, readOnlyItem(mid2));

			++row;
		}
	}
	else
	{
		THROW(DatabaseException, q.lastError().text());
	}
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

QTableWidgetItem*RunPlanner::readWriteItem(QString text)
{
	QTableWidgetItem* item = new QTableWidgetItem(text);
	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled|Qt::ItemIsEnabled);
	return item;
}
