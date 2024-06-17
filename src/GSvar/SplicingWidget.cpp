#include "SplicingWidget.h"
#include "ui_SplicingWidget.h"
#include "TSVFileStream.h"
#include "GUIHelper.h"
#include "Helper.h"
#include "GeneSet.h"
#include "GlobalServiceProvider.h"
#include "IgvSessionManager.h"

#include <FilterCascade.h>
#include <QCheckBox>

SplicingWidget::SplicingWidget(const QString& splicing_file, QWidget *parent) :
	QWidget(parent),
	ui_(new Ui::SplicingWidget)
{
	ui_->setupUi(this);

	connect(ui_->b_applyFilter, SIGNAL(clicked(bool)), this, SLOT(applyFilter()));
	connect(ui_->b_copyTable,SIGNAL(clicked()),this,SLOT(copyToClipboard()));
	connect(ui_->tw_splicing,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(OpenInIGV(QTableWidgetItem*)));

	loadSplicingFile(splicing_file);
	applyFilter();
}

SplicingWidget::~SplicingWidget()
{
	delete ui_;
}

void SplicingWidget::loadSplicingFile(const QString& file_path)
{
	TSVFileStream splicing_file(file_path);

	QByteArrayList header = splicing_file.header();
	QSet<int> numeric_columns;
	QSet<QByteArray> motifs;
	QSet<QByteArray> events;

	//init table
	ui_->tw_splicing->setColumnCount(header.size());

	//disable sorting
	ui_->tw_splicing->setSortingEnabled(false);

	//create header
	int motif_idx = -1;
	int event_idx = -1;
	for (int col_idx = 0; col_idx < header.size(); ++col_idx)
	{
		QByteArray col_name = header.at(col_idx);
		ui_->tw_splicing->setHorizontalHeaderItem(col_idx, new QTableWidgetItem(QString(col_name)));

		//save idx of numeric columns
		if ((col_name == "reads") || (col_name == "intron_start") || (col_name == "intron_end")) numeric_columns << col_idx;


		//store indices
		if (col_name == "motif") motif_idx = col_idx;
		if (col_name == "event") event_idx = col_idx;
		column_indices_.insert(col_name, col_idx);

	}

	//fill table
	int row_idx = 0;
	while(!splicing_file.atEnd())
	{
		QByteArrayList row = splicing_file.readLine();

		if(row.isEmpty()) continue;

		ui_->tw_splicing->setRowCount(row_idx+1);

		for (int col_idx = 0; col_idx < header.size(); ++col_idx)
		{
			if (numeric_columns.contains(col_idx))
			{
				ui_->tw_splicing->setItem(row_idx, col_idx, GUIHelper::createTableItem(row.at(col_idx).toInt()));
			}
			else
			{
				QTableWidgetItem* item = new QTableWidgetItem(QString(row.at(col_idx)));
				item->setFlags(item->flags() ^ Qt::ItemIsEditable);
				ui_->tw_splicing->setItem(row_idx, col_idx, item);
			}

			if (col_idx == motif_idx) motifs << row.at(col_idx);
			if (col_idx == event_idx) events << row.at(col_idx);
		}

		row_idx++;
	}

	//enable sorting
	ui_->tw_splicing->setSortingEnabled(true);

	//fill event scroll pane
	QVBoxLayout* vbox = new QVBoxLayout;
	foreach (const QByteArray& event, events)
	{
		QCheckBox* cb_event = new QCheckBox(event);
		cb_event->setChecked(event != "known");
		vbox->addWidget(cb_event);
	}
	ui_->gb_events->setLayout(vbox);

	//fill motif scroll pane
	vbox = new QVBoxLayout;
	foreach (const QByteArray& motif, motifs)
	{
		QCheckBox* cb_motif = new QCheckBox(motif);
		cb_motif->setChecked(true);
		vbox->addWidget(cb_motif);
	}
	ui_->gb_motif->setLayout(vbox);

	//optimize table view
	GUIHelper::resizeTableCellWidths(ui_->tw_splicing, 500);
	GUIHelper::resizeTableCellHeightsToFirst(ui_->tw_splicing);
}

void SplicingWidget::applyFilter()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);
		QTime timer;
		timer.start();
		qDebug() << "applying filter...";

		int row_count = ui_->tw_splicing->rowCount();
		FilterResult filter_result(row_count);

		//debug
		int filtered_lines = row_count;

		//determine indices
		int gene_idx = column_indices_.value("genes");

		//filter by gene name
		GeneSet gene_whitelist = GeneSet::createFromText(ui_->le_genes->text().toUtf8(), ',');
		if (!gene_whitelist.isEmpty())
		{
			qDebug() << "filter by gene filter";
			QByteArray genes_joined = gene_whitelist.join('|');

			// get column index of 'GENES' column
			if (gene_idx != -1)
			{
				if (genes_joined.contains("*")) //with wildcards
				{
					QRegExp reg(genes_joined.replace("-", "\\-").replace("*", "[A-Z0-9-]*"));
					for(int row_idx=0; row_idx<row_count; ++row_idx)
					{
						if (!filter_result.flags()[row_idx]) continue;

						// generate GeneSet from column text
						GeneSet genes = GeneSet::createFromText(ui_->tw_splicing->item(row_idx, gene_idx)->text().toUtf8(), ',');

						bool match_found = false;
						foreach(const QByteArray& gene, genes)
						{
							if (reg.exactMatch(gene))
							{
								match_found = true;
								break;
							}
						}
						filter_result.flags()[row_idx] = match_found;
					}
				}
				else //without wildcards
				{
					for(int row_idx=0; row_idx<row_count; ++row_idx)
					{
						if (!filter_result.flags()[row_idx]) continue;

						// generate GeneSet from column text
						GeneSet sv_genes = GeneSet::createFromText(ui_->tw_splicing->item(row_idx, gene_idx)->text().toUtf8(), ',');

						filter_result.flags()[row_idx] = sv_genes.intersectsWith(gene_whitelist);
					}
				}
			}

			//debug:
			qDebug() << "\t removed: " << (filtered_lines - filter_result.countPassing()) << Helper::elapsedTime(timer);
			filtered_lines = filter_result.countPassing();
		}


		//filter by min read count
		qDebug() << "filter by min read count";
		if (ui_->sb_min_reads->value() > 0)
		{
			int read_threshold = ui_->sb_min_reads->value();
			int read_idx = column_indices_.value("reads");
			for(int row_idx=0; row_idx<row_count; ++row_idx)
			{
				if (!filter_result.flags()[row_idx]) continue;

				int read_count = Helper::toInt(ui_->tw_splicing->item(row_idx, read_idx)->text(), "read count", QString::number(row_idx));

				filter_result.flags()[row_idx] = read_count >= read_threshold;
			}

			//debug:
			qDebug() << "\t removed: " << (filtered_lines - filter_result.countPassing()) << Helper::elapsedTime(timer);
			filtered_lines = filter_result.countPassing();
		}

		//filter by event
		qDebug() << "filter by event";
		QSet<QByteArray> selected_events;
		foreach (QCheckBox* cb_event, ui_->gb_events->findChildren<QCheckBox*>())
		{
			if (cb_event->isChecked())
			{
				selected_events.insert(cb_event->text().toUtf8().trimmed());
			}
		}
		int event_idx = column_indices_.value("event");
		for(int row_idx=0; row_idx<row_count; ++row_idx)
		{
			if (!filter_result.flags()[row_idx]) continue;
			QByteArray event = ui_->tw_splicing->item(row_idx, event_idx)->text().toUtf8().trimmed();
			filter_result.flags()[row_idx] = selected_events.contains(event);
		}
		//debug:
		qDebug() << "\t removed: " << (filtered_lines - filter_result.countPassing()) << Helper::elapsedTime(timer);
		filtered_lines = filter_result.countPassing();


		//filter by motif
		qDebug() << "filter by motif";
		QSet<QByteArray> selected_motifs;
		foreach (QCheckBox* cb_motif, ui_->gb_motif->findChildren<QCheckBox*>())
		{
			if (cb_motif->isChecked())
			{
				selected_motifs.insert(cb_motif->text().toUtf8().trimmed());
			}
		}
		int motif_idx = column_indices_.value("motif");
		for(int row_idx=0; row_idx<row_count; ++row_idx)
		{
			if (!filter_result.flags()[row_idx]) continue;
			QByteArray event = ui_->tw_splicing->item(row_idx, motif_idx)->text().toUtf8().trimmed();
			filter_result.flags()[row_idx] = selected_motifs.contains(event);
		}
		//debug:
		qDebug() << "\t removed: " << (filtered_lines - filter_result.countPassing()) << Helper::elapsedTime(timer);
		filtered_lines = filter_result.countPassing();

		//apply to table
		for(int row_idx=0; row_idx<row_count; ++row_idx)
		{
			ui_->tw_splicing->setRowHidden(row_idx, !filter_result.flags()[row_idx]);
		}

		//Set number of filtered / total SVs
		ui_->l_passFilter->setText(QByteArray::number(filter_result.countPassing()) + "/" + QByteArray::number(row_count));


		qDebug() << "done (runtime: " << Helper::elapsedTime(timer) << ")";
		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error filtering splicing prediction.");
	}
}

void SplicingWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_->tw_splicing);
}

void SplicingWidget::OpenInIGV(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	int row_idx = item->row();

	int chr_idx=-1, start_idx=-1, end_idx=-1;
	if (column_indices_.contains("chr")) chr_idx = column_indices_.value("chr");
	if (column_indices_.contains("intron_start")) start_idx = column_indices_.value("intron_start");
	if (column_indices_.contains("intron_end")) end_idx = column_indices_.value("intron_end");

	if ((chr_idx < 0) || (start_idx < 0) || (end_idx < 0))
	{
		THROW(FileParseException, "Missing coordinate column!");
	}

	QString coords = BedLine::fromString(ui_->tw_splicing->item(row_idx, chr_idx)->text() + ":"
										 + ui_->tw_splicing->item(row_idx, start_idx)->text() + "-"
										 + ui_->tw_splicing->item(row_idx, end_idx)->text()).toString(true);

    IgvSessionManager::get(0).gotoInIGV(coords, true);
}
