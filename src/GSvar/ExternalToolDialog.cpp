#include "ExternalToolDialog.h"
#include "Exceptions.h"
#include "Settings.h"
#include "Helper.h"
#include "BedFile.h"
#include "QCCollection.h"
#include "QFileDialog"
#include "Statistics.h"
#include "SampleSimilarity.h"

ExternalToolDialog::ExternalToolDialog(QString tool_name, QString mode, QWidget *parent)
	: QDialog(parent)
	, ui_()
	, tool_name_(tool_name)
	, mode_(mode)
{
	ui_.setupUi(this);
	setWindowTitle(tool_name);
	if (mode!="")
	{
		ui_.type->setText("Mode: " + mode);
	}

	connect(ui_.browse, SIGNAL(clicked()), this, SLOT(browse()));
}

void ExternalToolDialog::browse()
{
	ui_.output->clear();


	QString output;
	QTextStream stream(&output);

	if (tool_name_ == "BED file information")
	{
		QString filename = getFileName("Select BED file", "BED files (*.bed)");
		if (filename=="") return;

		//process
		QApplication::setOverrideCursor(Qt::BusyCursor);
		BedFile file;
		file.load(filename);
		QCCollection stats = Statistics::region(file, true);
		QApplication::restoreOverrideCursor();

		//output
		stream << "Regions: " << stats.value("roi_fragments").toString() << endl;
		stream << "Bases: " << stats.value("roi_bases").toString(0) << endl;
		stream << "Chromosomes: " << stats.value("roi_chromosomes").toString() << endl;
		stream << endl;
		stream << "Is sorted: " << stats.value("roi_is_sorted").toString() << endl;
		stream << "Is merged: " << stats.value("roi_is_merged").toString() << endl;
		stream << endl;
		stream << "Fragment size (min): " << stats.value("roi_fragment_min").toString() << endl;
		stream << "Fragment size (max): " << stats.value("roi_fragment_max").toString() << endl;
		stream << "Fragment size (mean): " << stats.value("roi_fragment_mean").toString() << endl;
		stream << "Fragment size (stdev): " << stats.value("roi_fragment_stdev").toString() << endl;
	}
	else if (tool_name_ == "Determine gender")
	{
		QString filename = getFileName("Select BAM file", "BAM files (*.bam)");
		if (filename=="") return;

		//process
		QApplication::setOverrideCursor(Qt::BusyCursor);

		GenderEstimate estimate;
		if (mode_=="xy")
		{
			estimate = Statistics::genderXY(filename);
		}
		else if (mode_=="hetx")
		{
			estimate = Statistics::genderHetX(filename, "hg19");
		}
		else if (mode_=="sry")
		{
			estimate = Statistics::genderSRY(filename, "hg19");
		}
		QApplication::restoreOverrideCursor();

		//output
		foreach(auto info, estimate.add_info)
		{
			stream  << info.key << ": " << info.value << endl;
		}
		stream << endl;
		stream << "gender: " << estimate.gender << endl;
	}
	else if (tool_name_ == "Sample similarity")
	{
		QString header = (mode_=="bam") ? "Select BAM file" : "Select variant list";
		QString filter = (mode_=="bam") ? "BAM files (*.bam)" : "GSvar files (*.GSvar);;VCF files (*.VCF *.VCF.GZ)";
		QString filename1 = getFileName(header , filter);
		if (filename1=="") return;
		QString filename2 = getFileName(header , filter);
		if (filename2=="") return;

		//process
		QApplication::setOverrideCursor(Qt::BusyCursor);
		if (mode_=="bam")
		{
			auto geno1 = SampleSimilarity::genotypesFromBam("hg19", filename1, 30, 500, false);
			auto geno2 = SampleSimilarity::genotypesFromBam("hg19", filename2, 30, 500, false);

			SampleSimilarity sc;
			sc.calculateSimilarity(geno1, geno2);

			stream << "Variants overlapping: " << QString::number(sc.olCount()) << endl;
			stream << "Correlation: " << QString::number(sc.sampleCorrelation(), 'f', 4) << endl;
		}
		else
		{
			auto geno1 = SampleSimilarity::genotypesFromVcf(filename1, false, true);
			auto geno2 = SampleSimilarity::genotypesFromVcf(filename2, false, true);

			SampleSimilarity sc;
			sc.calculateSimilarity(geno1, geno2);

			stream << "Variants file1: " << QString::number(sc.noVariants1()) << endl;
			stream << "Variants file2: " << QString::number(sc.noVariants2()) << endl;
			stream << "Variants overlapping: " << QString::number(sc.olCount()) << endl;
			stream << "Correlation: " << QString::number(sc.sampleCorrelation(), 'f', 4) << endl;
		}
		QApplication::restoreOverrideCursor();
	}
	else if (tool_name_ == "Sample ancestry")
	{
		QString filename = getFileName("Select VCF file" , "VCF files (*.VCF *.VCF.GZ)");
		if (filename=="") return;

		//process
		QApplication::setOverrideCursor(Qt::BusyCursor);
		VariantList vl;
		vl.load(filename);
		AncestryEstimates ancestry = Statistics::ancestry("hg19", vl);

		stream << "Informative SNPs: " << QString::number(ancestry.snps) << endl;
		stream << endl;
		stream << "Correlation AFR: " << QString::number(ancestry.afr, 'f', 4) << endl;
		stream << "Correlation EUR: " << QString::number(ancestry.eur, 'f', 4) << endl;
		stream << "Correlation SAS: " << QString::number(ancestry.sas, 'f', 4) << endl;
		stream << "Correlation EAS: " << QString::number(ancestry.eas, 'f', 4) << endl;
		stream << endl;
		stream << "Population: " << ancestry.population << endl;
		QApplication::restoreOverrideCursor();
	}
	else
	{
		THROW(ProgrammingException, "Unknown tool '" + tool_name_ + "' requested in ExternalToolDialog!");
	}

	ui_.output->setPlainText(output);
}

QString ExternalToolDialog::getFileName(QString title, QString filters)
{
	QString open_path = Settings::path("path_variantlists");
	QString file_name = QFileDialog::getOpenFileName(this, title, open_path, filters + ";;All files(*.*)");
	if (file_name!="")
	{
		Settings::setPath("path_variantlists", file_name);
	}
	else
	{
		ui_.output->setPlainText("canceled...");
	}
	return file_name;
}
