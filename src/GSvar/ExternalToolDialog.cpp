#include <QMessageBox>
#include "ExternalToolDialog.h"
#include "Exceptions.h"
#include "Settings.h"
#include "Helper.h"
#include "BedFile.h"
#include "QCCollection.h"
#include "QFileDialog"
#include "Statistics.h"
#include "SampleSimilarity.h"
#include "LoginManager.h"
#include "ProcessedSampleSelector.h"

ExternalToolDialog::ExternalToolDialog(QString tool_name, QString mode, QWidget* parent)
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
	connect(ui_.browse_ngsd, SIGNAL(clicked()), this, SLOT(browse()));

	ui_.browse_ngsd->setEnabled(LoginManager::active());
}

void ExternalToolDialog::browse()
{
	ui_.output->clear();

	QString output;
	QTextStream stream(&output);
	bool ngsd_instead_of_filesystem = sender()==ui_.browse_ngsd;

	if (tool_name_ == "BED file information")
	{
		QString filename = getFileName(BED, ngsd_instead_of_filesystem);
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
		QString filename = getFileName(BAM, ngsd_instead_of_filesystem);
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
		//get file names
		FileType type = GSVAR;
		if (mode_=="vcf") type = VCF;
		if (mode_=="bam") type = BAM;
		QString filename1 = getFileName(type, ngsd_instead_of_filesystem);
		if (filename1=="") return;
		QString filename2 = getFileName(type, ngsd_instead_of_filesystem);
		if (filename2=="") return;

		//process
		QApplication::setOverrideCursor(Qt::BusyCursor);
		if (mode_=="bam")
		{
			SampleSimilarity::VariantGenotypes geno1 = SampleSimilarity::genotypesFromBam("hg19", filename1, 30, 500, false);
			SampleSimilarity::VariantGenotypes geno2 = SampleSimilarity::genotypesFromBam("hg19", filename2, 30, 500, false);

			SampleSimilarity sc;
			sc.calculateSimilarity(geno1, geno2);

			stream << "Variants overlapping: " << QString::number(sc.olCount()) << endl;
			stream << "Correlation: " << QString::number(sc.sampleCorrelation(), 'f', 4) << endl;
		}
		else //VCF/GSvar
		{
			SampleSimilarity::VariantGenotypes geno1;
			SampleSimilarity::VariantGenotypes geno2;
			if (mode_=="vcf")
			{
				geno1 = SampleSimilarity::genotypesFromVcf(filename1, false, true);
				geno2 = SampleSimilarity::genotypesFromVcf(filename2, false, true);
			}
			else
			{
				geno1 = SampleSimilarity::genotypesFromGSvar(filename1, false);
				geno2 = SampleSimilarity::genotypesFromGSvar(filename2, false);
			}

			SampleSimilarity sc;
			sc.calculateSimilarity(geno1, geno2);

			stream << "Variants file1: " << QString::number(sc.noVariants1()) << endl;
			stream << "Variants file2: " << QString::number(sc.noVariants2()) << endl;
			stream << "Variants overlapping: " << QString::number(sc.olCount()) << " (" << sc.olPerc() << "%)" << endl;
			stream << "Correlation: " << QString::number(sc.sampleCorrelation(), 'f', 4) << endl;
		}
		QApplication::restoreOverrideCursor();
	}
	else if (tool_name_ == "Sample ancestry")
	{
		QString filename = getFileName(VCF, ngsd_instead_of_filesystem);
		if (filename=="") return;

		//process
		QApplication::setOverrideCursor(Qt::BusyCursor);
		AncestryEstimates ancestry = Statistics::ancestry("hg19", filename);

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

QString ExternalToolDialog::getFileName(FileType type, bool ngsd_instead_of_filesystem)
{
	//prepare title
	QString title = "";
	if (type==BAM) title = "Select BAM file";
	if (type==GSVAR) title = "Select single-sample GSvar file";
	if (type==VCF) title = "Select single-sample VCF file";
	if (type==BED) title = "Select BED file";

	if (ngsd_instead_of_filesystem && type==BED) //BED not supported from NGSD
	{
		QMessageBox::information(this, title, "Please select BED file from file system!");
	}
	else if (ngsd_instead_of_filesystem) //from NGSD
	{
		ProcessedSampleSelector dlg(this, false);
		if (dlg.exec()==QDialog::Accepted && dlg.isValidSelection())
		{
			QString ps_id = dlg.processedSampleId();
			NGSD db;

			if (type==BAM) return db.processedSamplePath(ps_id, PathType::BAM);
			if (type==GSVAR) return db.processedSamplePath(ps_id, PathType::GSVAR);
			if (type==VCF) return db.processedSamplePath(ps_id, PathType::VCF);
		}
	}
	else //from filesystem
	{
		//prepare filters
		QStringList filters;
		if (type==BAM) filters << "BAM files (*.bam)";
		if (type==GSVAR) filters << "GSvar files (*.GSvar)";
		if (type==VCF) filters << "VCF files (*.vcf *.vcf.gz)";
		if (type==BED) filters << "BED files (*.bed)";
		filters << "All files(*.*)";

		QString open_path = Settings::path("path_variantlists", true);
		QString file_name = QFileDialog::getOpenFileName(this, title, open_path, filters.join(";;"));
		if (file_name!="")
		{
			Settings::setPath("path_variantlists", file_name);
			return file_name;
		}
	}

	return "";
}
