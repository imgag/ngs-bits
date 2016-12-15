#include "Exceptions.h"
#include "ReportDialog.h"
#include "NGSD.h"
#include "Log.h"
#include <QTableWidgetItem>
#include <QBitArray>
#include <QPushButton>
#include <QMenu>


ReportDialog::ReportDialog(QString filename, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, filename_(filename)
	, labels_()
{
	ui_.setupUi(this);

	//variant header
	labels_ << "" << "chr" << "start" << "end" << "ref" << "obs" << "ihdb_allsys_hom" << "ihdb_allsys_het" << "genotype" << "gene" << "variant_type" << "coding_and_splicing";
	ui_.vars->setColumnCount(labels_.count());
	ui_.vars->setHorizontalHeaderLabels(labels_);

	//contect menu
	ui_.vars->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_.vars, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

	//init outcome
	ui_.outcome->addItems(NGSD().getEnum("diag_status", "outcome"));
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	connect(ui_.outcome, SIGNAL(currentTextChanged(QString)), this, SLOT(outcomeChanged(QString)));

	//set outcome from NGSD
	QStringList diag_status = NGSD().getDiagnosticStatus(filename_);
	if (diag_status.count()<4)
	{
		Log::warn("Could not determine diagnostic status in NGSD for file '" + filename + "'!");
	}
	else
	{
		int index = ui_.outcome->findText(diag_status.at(3));
		if (index==-1)
		{
			Log::warn("Could not determine outcome index of outcome '" + diag_status.at(3) + "'!");
		}
		else
		{
			ui_.outcome->setCurrentIndex(index);
		}
	}
}

void ReportDialog::addVariants(const VariantList& variants, const QBitArray& visible)
{
	int class_idx = variants.annotationIndexByName("classification", true, false);

	ui_.vars->setRowCount(visible.count(true));
	int row = 0;
	for (int i=0; i<variants.count(); ++i)
	{
		if (!visible[i]) continue;

		const Variant& variant = variants[i];
		ui_.vars->setItem(row, 0, new QTableWidgetItem(""));
		ui_.vars->item(row, 0)->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		ui_.vars->item(row, 0)->setCheckState(Qt::Checked);
		if (class_idx!=-1 && variant.annotations()[class_idx]=="1")
		{
			ui_.vars->item(row, 0)->setCheckState(Qt::Unchecked);
			ui_.vars->item(row, 0)->setToolTip("Unchecked because of classification '1'.");
		}
		ui_.vars->item(row, 0)->setData(Qt::UserRole, i);
		ui_.vars->setItem(row, 1, new QTableWidgetItem(QString(variant.chr().str())));
		ui_.vars->setItem(row, 2, new QTableWidgetItem(QString::number(variant.start())));
		ui_.vars->setItem(row, 3, new QTableWidgetItem(QString::number(variant.end())));
		ui_.vars->setItem(row, 4, new QTableWidgetItem(variant.ref(), 0));
		ui_.vars->setItem(row, 5, new QTableWidgetItem(variant.obs(), 0));
		for (int j=6; j<labels_.count(); ++j)
		{
			int index = variants.annotationIndexByName(labels_[j], true, false);
			if (index==-1 && labels_[j]!="genotype")
			{
				THROW(ArgumentException, "Report dialog: Could not find variant annotation '" + labels_[j] + "'!");
			}
			else if (index==-1 && labels_[j]=="genotype")
			{
				ui_.vars->setItem(row, j, new QTableWidgetItem("n/a"));
			}
			else
			{
				ui_.vars->setItem(row, j, new QTableWidgetItem(variant.annotations().at(index), 0));
			}
		}

		++row;
	}

	ui_.vars->resizeColumnsToContents();
	ui_.vars->resizeRowsToContents();
}

void ReportDialog::setTargetRegionSelected(bool is_selected)
{
	if (!is_selected)
	{
		ui_.details_cov->setChecked(false);
		ui_.details_cov->setEnabled(false);
		ui_.min_cov->setEnabled(false);
	}
}

QList<int> ReportDialog::selectedIndices() const
{
	QList<int> output;

	for (int i=0; i<ui_.vars->rowCount(); ++i)
	{
		QTableWidgetItem* item = ui_.vars->item(i, 0);
		if (item->checkState()==Qt::Checked)
		{
			output.append(item->data(Qt::UserRole).toInt());
		}
	}

	return output;
}

bool ReportDialog::detailsCoverage() const
{
	return ui_.details_cov->isChecked();
}

int ReportDialog::minCoverage() const
{
	return ui_.min_cov->value();
}

QString ReportDialog::outcome() const
{
	return ui_.outcome->currentText();
}

void ReportDialog::outcomeChanged(QString text)
{
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(text!="n/a");
}

void ReportDialog::on_outcome_submit_clicked(bool)
{
	NGSD().setReportOutcome(filename_, outcome());
}

void ReportDialog::showContextMenu(QPoint pos)
{
	QMenu menu(ui_.vars);
	menu.addAction(QIcon(":/Icons/box_checked.png"), "select all");
	menu.addAction(QIcon(":/Icons/box_unchecked.png"), "unselect all");

	QAction* action = menu.exec(ui_.vars->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	QByteArray text = action->text().toLatin1();
	if (text=="select all")
	{
		for (int i=0; i<ui_.vars->rowCount(); ++i)
		{
			ui_.vars->item(i, 0)->setCheckState(Qt::Checked);
		}
	}
	if (text=="unselect all")
	{
		for (int i=0; i<ui_.vars->rowCount(); ++i)
		{
			ui_.vars->item(i, 0)->setCheckState(Qt::Unchecked);
		}
	}
}
