#include <QFile>
#include <QMessageBox>
#include "RepeatExpansionWidget.h"
#include "ui_RepeatExpansionWidget.h"
#include "Helper.h"
#include "GUIHelper.h"
#include "TsvFile.h"
#include "VariantList.h"

NumericWidgetItem::NumericWidgetItem(QString text):
	QTableWidgetItem(text)
{
	this->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
}

bool NumericWidgetItem::operator<(const QTableWidgetItem& other) const
{
	//convert text to double
	double this_value = Helper::toDouble(this->text());
	double other_value = Helper::toDouble(other.text());
	return (this_value < other_value);
}

RepeatExpansionWidget::RepeatExpansionWidget(QString vcf_filename, QWidget* parent):
	QWidget(parent),
	vcf_filename_(vcf_filename),
	ui_(new Ui::RepeatExpansionWidget)
{
	ui_->setupUi(this);
	loadRepeatExpansionData();
}

RepeatExpansionWidget::~RepeatExpansionWidget()
{
	delete ui_;
}

void RepeatExpansionWidget::loadRepeatExpansionData()
{
	//load VCF file
	VariantList repeat_expansions;
	repeat_expansions.load(vcf_filename_);

	//load filter file
	QMap<QPair<QByteArray,QByteArray>, RepeatCutoff> filters;
	QStringList filter_content = Helper::loadTextFile(":/Resources/repeat_expansion_cutoffs.tsv", true);

	foreach (const QString& line, filter_content)
	{
		// skip empty lines and comments
		if((line=="") || (line.startsWith("#"))) continue;

		// parse line
		QStringList split_line = line.split('\t');
		if (split_line.size() < 5) THROW(FileParseException, "Error parsing repeat expansion file: file has to contain 4 tab-separated columns!");
		RepeatCutoff repeat_cutoff;

		repeat_cutoff.repeat_id = split_line.at(0).trimmed().toUtf8();
		repeat_cutoff.repeat_unit = split_line.at(1).trimmed().toUtf8();
		repeat_cutoff.max_normal = Helper::toInt(split_line.at(2));
		repeat_cutoff.min_pathogenic = Helper::toInt(split_line.at(3));
		repeat_cutoff.inheritance = split_line.at(4).trimmed().toUtf8();
		filters.insert(QPair<QByteArray,QByteArray>(repeat_cutoff.repeat_id, repeat_cutoff.repeat_unit), repeat_cutoff);
	}

	//create table

	//define columns
	QStringList column_names;
	QVector<bool> numeric_columns;
	QStringList description;
	column_names << "chr" << "start" << "end" << "repeat_id" << "repeat_unit"
				 << "repeats" << "wt_repeat" << "repeat_ci"<< "filter" << "locus_coverage"
				 << "reads_flanking" << "reads_in_repeat" << "reads_spanning";
	numeric_columns << false << true << true << false << false << false << true << false << true << false << false << false;
	description << ""
				<< ""
				<< ""
				<< "Repeat identifier as specified in the variant catalog"
				<< "Repeat unit in the reference orientation"
				<< "Number of repeat units spanned by the allele"
				<< "Reference repeat count"
				<< "Confidence interval for allele copy number"
				<< QString("Filter:\n")
				   + "  PASS:\t\tAll filters passed\n"
				   + "  LowDepth:\tThe overall locus depth is below 10x or number of reads spanning one or both breakends is below 5."
				<< "Locus coverage"
				<< "Number of flanking reads consistent with the allele"
				<< "Number of in-repeat reads consistent with the allele"
				<< "Number of spanning reads consistent with the allele";


	//create header
	ui_->repeat_expansions->setColumnCount(column_names.size());
	for (int col_idx = 0; col_idx < column_names.size(); ++col_idx)
	{
		ui_->repeat_expansions->setHorizontalHeaderItem(col_idx, new QTableWidgetItem(column_names.at(col_idx)));
		if (description.at(col_idx) != "")
		{
			ui_->repeat_expansions->horizontalHeaderItem(col_idx)->setToolTip(description.at(col_idx));
		}
	}

	// get indices for required columns in the VCF file
	int end_idx = repeat_expansions.annotationIndexByName("END");
	int repeat_id_idx = repeat_expansions.annotationIndexByName("REPID");
	int repeat_unit_idx = repeat_expansions.annotationIndexByName("RU");
	int allele_repeats_idx = repeat_expansions.annotationIndexByName("REPCN");
	int ref_repeats_idx = repeat_expansions.annotationIndexByName("REF");
	int repeat_ci_idx = repeat_expansions.annotationIndexByName("REPCI");
	int filter_idx = repeat_expansions.annotationIndexByName("FILTER");
	int coverage_idx = repeat_expansions.annotationIndexByName("LC");
	int fl_reads_idx = repeat_expansions.annotationIndexByName("ADFL");
	int ir_reads_idx = repeat_expansions.annotationIndexByName("ADIR");
	int sp_reads_idx = repeat_expansions.annotationIndexByName("ADSP");

	// define table backround colors
	QColor bg_red = Qt::red;
	bg_red.setAlphaF(0.5);
	QColor bg_green = Qt::darkGreen;
	bg_green.setAlphaF(0.5);
	QColor bg_orange = QColor(255, 135, 60);
	bg_orange.setAlphaF(0.5);


	// fill table widget with variants/repeat expansions
	ui_->repeat_expansions->setRowCount(repeat_expansions.count());
	for(int row_idx=0; row_idx<repeat_expansions.count(); ++row_idx)
	{
		// add vertical header item
		ui_->repeat_expansions->setVerticalHeaderItem(row_idx, new QTableWidgetItem(QString::number(row_idx+1)));
		Variant re = repeat_expansions[row_idx];
		int col_idx = 0;

		//add position
		ui_->repeat_expansions->setItem(row_idx, col_idx++, new QTableWidgetItem(QString(re.chr().strNormalized(true))));
		ui_->repeat_expansions->setItem(row_idx, col_idx++, new NumericWidgetItem(QString::number(re.start())));
		ui_->repeat_expansions->setItem(row_idx, col_idx++, new NumericWidgetItem(QString(re.annotations()[end_idx])));

		//add repeat
		ui_->repeat_expansions->setItem(row_idx, col_idx++, new QTableWidgetItem(QString(re.annotations()[repeat_id_idx])));
		ui_->repeat_expansions->setItem(row_idx, col_idx++, new QTableWidgetItem(QString(re.annotations()[repeat_unit_idx])));

		//add allele/ref copy number
		QTableWidgetItem* repeat_cell = new QTableWidgetItem(QString(re.annotations()[allele_repeats_idx]));

		//color table acording to cutoff table
		QPair<QByteArray, QByteArray> key = QPair<QByteArray, QByteArray>(re.annotations()[repeat_id_idx].trimmed(), re.annotations()[repeat_unit_idx].trimmed());
		QString tool_tip_text = "No repeat information available.";
		if (filters.contains(key))
		{
			RepeatCutoff cutoff = filters.value(key);
			tool_tip_text = "normal: \t ≤ " + QString::number(cutoff.max_normal)
					+ "\npathogenic: \t > " + QString::number(cutoff.min_pathogenic)
					+ "\ninheritance: \t " + cutoff.inheritance;

			QByteArrayList repeats = re.annotations()[allele_repeats_idx].split('/');
			int repeats_allele1, repeats_allele2;
			if ((repeats.size() < 1) || (repeats.size() > 2)) THROW(FileParseException, "Invalid allele count in repeat entries!");

			// skip coloring if no repeat information is available:
			if (repeats.at(0).trimmed() != ".")
			{
				repeats_allele1 = Helper::toInt(repeats.at(0), "Repeat allele 1", QString::number(row_idx));

				if (repeats.size() == 1) repeats_allele2 = 0; // special case for male samples on chrX
				else repeats_allele2 = Helper::toInt(repeats.at(1), "Repeat allele 2", QString::number(row_idx));

				if ((repeats_allele1 > cutoff.min_pathogenic) || (repeats_allele2 > cutoff.min_pathogenic))
				{
					// at least one allele has a pathogenic repeat count
					ui_->repeat_expansions->verticalHeaderItem(row_idx)->setTextColor(Qt::red);
					repeat_cell->setBackgroundColor(bg_red);
				}
				else if ((repeats_allele1 <= cutoff.max_normal) && (repeats_allele2 <= cutoff.max_normal))
				{
					// repeat count in both alleles is in normal range
					ui_->repeat_expansions->verticalHeaderItem(row_idx)->setTextColor(Qt::darkGreen);
					repeat_cell->setBackgroundColor(bg_green);
				}
				else
				{
					// at least one allele is above normal range, but not in pathogenic range
					ui_->repeat_expansions->verticalHeaderItem(row_idx)->setTextColor(QColor(255, 135, 60)); //orange
					repeat_cell->setBackgroundColor(bg_orange);
				}
			}
		}
		else
		{
			// no information available --> color black
			ui_->repeat_expansions->verticalHeaderItem(row_idx)->setTextColor(Qt::black);
		}

		repeat_cell->setToolTip(tool_tip_text);

		ui_->repeat_expansions->setItem(row_idx, col_idx++, repeat_cell);
		ui_->repeat_expansions->setItem(row_idx, col_idx++, new NumericWidgetItem(QString(re.annotations()[ref_repeats_idx])));

		//add additional info
		ui_->repeat_expansions->setItem(row_idx, col_idx++, new QTableWidgetItem(QString(re.annotations()[repeat_ci_idx])));

		//add filter column and color background if not 'PASS'
		QTableWidgetItem* filter_cell = new QTableWidgetItem(QString(re.annotations()[filter_idx]));
		if(filter_cell->text().trimmed() != "PASS") filter_cell->setBackgroundColor(bg_orange);
		ui_->repeat_expansions->setItem(row_idx, col_idx++, filter_cell);

		//round local coverage
		double coverage = Helper::toDouble(re.annotations()[coverage_idx]);
		ui_->repeat_expansions->setItem(row_idx, col_idx++, new NumericWidgetItem(QString::number(coverage, 'f', 2)));

		//add read counts
		ui_->repeat_expansions->setItem(row_idx, col_idx++, new QTableWidgetItem(QString(re.annotations()[fl_reads_idx])));
		ui_->repeat_expansions->setItem(row_idx, col_idx++, new QTableWidgetItem(QString(re.annotations()[ir_reads_idx])));
		ui_->repeat_expansions->setItem(row_idx, col_idx++, new QTableWidgetItem(QString(re.annotations()[sp_reads_idx])));

	}

	// display vertical header
	ui_->repeat_expansions->verticalHeader()->setVisible(true);


}




