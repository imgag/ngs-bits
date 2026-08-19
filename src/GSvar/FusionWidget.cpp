#include "FusionWidget.h"
#include "ui_FusionWidget.h"

#include <GUIHelper.h>
#include <Helper.h>
#include <TsvFile.h>
#include <VersatileFile.h>
#include <GlobalServiceProvider.h>
#include "QImageReader"
#include "QMessageBox"
#include <HttpHandler.h>
#include "RnaReportFusionDialog.h"


FusionWidget::FusionWidget(const QString& filename, const QString& rna_ps_name, QSharedPointer<RnaReportConfiguration> rna_rep_conf,  QWidget *parent) :
	QWidget(parent),
	rna_report_config_(rna_rep_conf),
	data_controller_(AnalysisDataController::instance()),
	state_(data_controller_.getFusionFilterState()),
    filename_(filename),
    fusions_(),
    rna_ps_name_(rna_ps_name),

	ui_(new Ui::FusionWidget)
{
	ui_->setupUi(this);
	fusions_.load(filename_);

	QStringList messages;

	if(!messages.isEmpty())
	{
		QMessageBox::warning(this, "RNA report configuration", "The following problems were encountered while loading the rna report configuration:\n" + messages.join("\n"));
	}

	GUIHelper::styleSplitter(ui_->splitter);

	connect(&state_, SIGNAL(targetRegionChanged()), this, SLOT(clearTooltips()));
	connect(&data_controller_, SIGNAL(FusionFilterResultChanged()), this, SLOT(updateGUI()));

	connect(ui_->fusions, SIGNAL(itemSelectionChanged()), this, SLOT(displayFusionImage()));
	//TODO
	connect(ui_->fusions, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui_->filter_widget, SIGNAL(filtersChanged()), this, SLOT(applyFilters()));
	connect(ui_->filter_widget, SIGNAL(targetRegionChanged()), this, SLOT(clearTooltips()));
	connect(ui_->filter_widget, SIGNAL(calculateGeneTargetRegionOverlap()), this, SLOT(annotateTargetRegionGeneOverlap()));
	connect(ui_->filter_widget, SIGNAL(phenotypeImportNGSDRequested()), this, SLOT(importPhenotypesFromNGSD()));

	updateGUI();

	QStringList images = GlobalServiceProvider::database().getRnaFusionPics(rna_ps_name_);
	images_ = imagesFromFiles(images);
}

FusionWidget::~FusionWidget()
{
	delete ui_;
}

void FusionWidget::updateGUI()
{
	//get report variant indices
    QList<int> var_idices_list = rna_report_config_->fusionIndices(false);
    QSet<int> report_variant_indices(var_idices_list.begin(), var_idices_list.end());

	QStringList column_names = fusions_.headers();
	QStringList numeric_columns;
	numeric_columns << "split_reads1" << "split_reads2" << "discordant_mates" << "coverage1" << "coverage2";

	//create header
	ui_->fusions->setColumnCount(column_names.size());
	for (int col_idx = 0; col_idx < column_names.size(); ++col_idx)
	{
		ui_->fusions->setHorizontalHeaderItem(col_idx, new QTableWidgetItem(column_names.at(col_idx)));
	}

	//fill table widget with expression data
	ui_->fusions->setRowCount(fusions_.count());
	for(int row_idx=0; row_idx<fusions_.count(); ++row_idx)
	{
		const QStringList& row = fusions_[row_idx];
		for (int col_idx = 0; col_idx < column_names.size(); ++col_idx)

		{
			if(numeric_columns.contains(column_names[col_idx]))
			{
				// add numeric QTableWidgetItem
				QString value = row.at(col_idx);
				if (value != "n/a" && !value.isEmpty())
				{
					ui_->fusions->setItem(row_idx, col_idx, GUIHelper::createTableItem(Helper::toDouble(value, "TSV column " + QString::number(col_idx), QString::number(row_idx)), 0));
				}
				else
				{
					ui_->fusions->setItem(row_idx, col_idx, GUIHelper::createTableItem(""));
				}
			}
			else
			{
				// add standard QTableWidgetItem
				ui_->fusions->setItem(row_idx, col_idx, GUIHelper::createTableItem(row.at(col_idx)));
			}
		}

		//vertical headers (report config icons)
		QTableWidgetItem* header_item = GUIHelper::createTableItem(QByteArray::number(row_idx+1));
		if (report_variant_indices.contains(row_idx))
		{
            const RnaReportFusionConfiguration& rc = rna_report_config_->get(row_idx);
			header_item->setIcon(VariantTable::reportIcon(rc.showInReport(), true));
		}
		ui_->fusions->setVerticalHeaderItem(row_idx, header_item);
	}

	//enable sorting
	ui_->fusions->setSortingEnabled(true);

	//update GUI by filter
	for(int r=0; r<fusions_.count(); ++r)
	{
		ui_->fusions->setRowHidden(r, !data_controller_.getFusionFilterResult().flags()[r]);
	}
	updateStatus(data_controller_.getFusionFilterResult().countPassing());

	//optimize table view
	GUIHelper::resizeTableCellWidths(ui_->fusions, 200);
	ui_->fusions->resizeRowsToContents();
}

void FusionWidget::showContextMenu(QPoint p)
{
    qDebug() << "FusionWidget::showContextMenu()";

	QMenu menu;

	QList<int> selected_rows = GUIHelper::selectedTableRows(ui_->fusions);

	//create menu

    //report config:
    QAction* a_rep_edit;
    QAction* a_rep_del;
    if (rna_report_config_)
    {
        a_rep_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
        a_rep_del = menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
        bool any_exists = false;
        foreach(int idx, selected_rows)
        {
            if (rna_report_config_ && rna_report_config_->exists(idx))
            {
                any_exists = true;
                break;
            }
        }
        a_rep_del->setEnabled(any_exists);
    }

    //exec menu
    QAction* action = menu.exec(ui_->fusions->viewport()->mapToGlobal(p));
    if (action==nullptr) return;

    //react on selection
    if (action==a_rep_edit)
    {
        editRnaReportConfiguration(selected_rows);
    }
    else if (action==a_rep_del)
    {
        //Delete som variant configuration for more than one variant
        foreach(const auto& selected_row, selected_rows)
        {
            if (rna_report_config_->exists(selected_row))
            {
                rna_report_config_->remove(selected_row);
                updateReportConfigHeaderIcon(selected_row);
            }
        }
        emit storeRnaReportConfiguration();
    }
}

void FusionWidget::editRnaReportConfiguration(int row)
{
	RnaReportFusionConfiguration fusion_config;
    bool settings_exist = rna_report_config_->exists(row);
	if(settings_exist)
	{
        fusion_config = rna_report_config_->get(row);
	}
	else
	{
		fusion_config.variant_index = row;
	}

    RnaReportFusionDialog dlg = RnaReportFusionDialog(fusions_.getFusion(row).toString(), fusion_config, this);
    if(dlg.exec()!=QDialog::Accepted) return;

    rna_report_config_->addRnaFusionConfiguration(fusion_config);
	updateReportConfigHeaderIcon(row);
    emit storeRnaReportConfiguration();
}

void FusionWidget::editRnaReportConfiguration(const QList<int> &rows)
{
    qDebug() << "FusionWidget::editReportConfiguration()";
    //TODO Seg fault somewhere?
	RnaReportFusionConfiguration fusion_config;

    RnaReportFusionDialog dlg = RnaReportFusionDialog(QString::number(rows.count()) +" selected fusions", fusion_config, this);
    if(dlg.exec() != QDialog::Accepted) return;

	//Accepted was pressed -> see slot writeBackSettings()
	for(int row : rows)
	{
		RnaReportFusionConfiguration temp_var_config = fusion_config;
		temp_var_config.variant_index = row;

        rna_report_config_->addRnaFusionConfiguration(temp_var_config);

		updateReportConfigHeaderIcon(row);
	}
	storeRnaReportConfiguration();
}

void FusionWidget::updateReportConfigHeaderIcon(int row)
{
    qDebug() << "FusionWidget::updateReportConfigHeaderIcon()";
	//report config-based filter is on => update whole variant list
	if (state_.getReportConfigFilter()!=ReportConfigFilter::NONE)
	{
		updateGUI();
	}
	else //no filter => refresh icon only
	{
		QIcon report_icon;
		if (rna_report_config_->exists(row))
		{
			const RnaReportFusionConfiguration& rc = rna_report_config_->get(row);
			report_icon = VariantTable::reportIcon(rc.showInReport(), true);
		}

		ui_->fusions->verticalHeaderItem(row)->setIcon(report_icon);
	}
}

void FusionWidget::updateStatus(int shown)
{
	QString text = QString::number(shown) + "/" + QString::number(fusions_.count()) + " passing filter(s)";
	ui_->status->setText(text);
}

void FusionWidget::displayFusionImage()
{
	if (ui_->fusions->selectedItems().count() != 1)
	{
		ui_->fusion_image->clearMask();
		qDebug() << "Multiple items selected.";
		return;
	}

	const QTableWidgetItem* selected = ui_->fusions->selectedItems()[0];
	int row = selected->row();

	qDebug() << "item row: " << row;

	if (row >= images_.count())
	{
        ui_->fusion_image->clearImage();
		qDebug() <<"No image found for this fusion. Row > image count";
		return;
	}

	ui_->fusion_image->setImage(images_[row]);

}

QList<QImage> FusionWidget::imagesFromFiles(const QStringList& files)
{
	QList<QImage> pic_list;
	foreach(const QString& path, files)
	{
		QImage pic;
		if (path.startsWith("http", Qt::CaseInsensitive))
		{
			QByteArray response = HttpHandler(true).get(path);
			if (!response.isEmpty()) pic.loadFromData(response);
		}
		else
		{
			pic = QImage(path);
		}
		if(pic.isNull()) continue;

		pic_list << pic;
	}

	return pic_list;
}
