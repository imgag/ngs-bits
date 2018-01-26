#include "ExternalToolDialog.h"
#include "Exceptions.h"
#include "Settings.h"
#include "Helper.h"
#include "BedFile.h"
#include "QCCollection.h"
#include "QFileDialog"
#include "Statistics.h"
#include "SampleCorrelation.h"

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

	QApplication::setOverrideCursor(Qt::BusyCursor);

	QString output;
	QTextStream stream(&output);

	if (tool_name_ == "BED file information")
	{
		QString filename = getFileName("Select BED file", "BED files (*.bed)");
		if (filename=="") return;

		//process
		BedFile file;
		file.load(filename);
		QCCollection stats = Statistics::region(file, true);

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
		QString gender;
		QStringList debug_output;
		if (mode_=="xy")
		{
			gender = Statistics::genderXY(filename, debug_output);
		}
		else if (mode_=="hetx")
		{
			gender = Statistics::genderHetX(filename, debug_output);
		}
		else if (mode_=="sry")
		{
			gender = Statistics::genderSRY(filename, debug_output);
		}

		//output
		foreach(const QString& line, debug_output)
		{
			stream  << line << endl;
		}
		stream << endl;
		stream << "gender: " << gender << endl;

	}
	else if (tool_name_ == "Sample correlation")
	{
		QString header = (mode_=="bam") ? "Select BAM file" : "Select variant list";
		QString filter = (mode_=="bam") ? "BAM files (*.bam)" : "GSvar files (*.GSvar);;VCF files (*.VCF);;VCF.GZ files (*.VCF.GZ)";
		QString filename1 = getFileName(header , filter);
		if (filename1=="") return;
		QString filename2 = getFileName(header , filter);
		if (filename2=="") return;

		//process
		if (mode_=="bam")
		{
			SampleCorrelation sc;
			sc.calculateFromBam(filename1, filename2, 30, 500);

			stream << "Variants used: " << QString::number(sc.noVariants1()) << endl;
			stream << "Correlation: " << QString::number(sc.sampleCorrelation(), 'f', 4) << endl;
		}
		else
		{
			SampleCorrelation sc;
			sc.calculateFromVcf(filename1, filename2, 100);

			stream << "Variants file1: " << QString::number(sc.noVariants1()) << endl;
			stream << "Variants file2: " << QString::number(sc.noVariants2()) << endl;
			stream << "Overlap percentage: " << QString::number(sc.olPerc(), 'f', 2) << endl;
			stream << "Correlation: " << QString::number(sc.sampleCorrelation(), 'f', 4) << endl;
		}
	}
	else
	{
		THROW(ProgrammingException, "Unknown tool '" + tool_name_ + "' requested in ExternalToolDialog!");
	}

	ui_.output->setPlainText(output);
	QApplication::restoreOverrideCursor();
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
