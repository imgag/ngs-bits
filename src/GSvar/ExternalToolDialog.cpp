#include "ExternalToolDialog.h"
#include "Exceptions.h"
#include "Settings.h"
#include "Helper.h"
#include "QFileDialog"

ExternalToolDialog::ExternalToolDialog(QString tool_name, QString args, QWidget *parent)
	: QDialog(parent)
	, ui_()
	, args_(args)
	, process_(0)
	, timer_()
{
	ui_.setupUi(this);
	ui_.tool_name->setText(tool_name);
	setWindowTitle("External tool dialog");

	connect(ui_.browse, SIGNAL(clicked()), this, SLOT(browse()));
}

void ExternalToolDialog::browse()
{
	ui_.output->clear();

	QString tool = ui_.tool_name->text();

	if (tool == "BedGeneOverlap")
	{
		QString filename = getFileName("Select BED file", "BED files (*.bed)");
		if (filename=="") return;
		startTool("-db " + Settings::string("kgxref_merged") + " -in " + filename);
	}
	else if (tool == "BedInfo")
	{
		QString filename = getFileName("Select BED file", "BED files (*.bed)");
		if (filename=="") return;
		startTool("-in " + filename);
	}
	else if (tool == "FastaInfo")
	{
		QString filename = getFileName("Select FastA file", "FastA files (*.fa *.fasta)");
		if (filename=="") return;
		startTool("-in " + filename);
	}
	else if (tool == "SampleGender")
	{
		QString filename = getFileName("Select BAM file", "BAM files (*.bam)");
		if (filename=="") return;
		startTool("-in " + filename);
	}
	else if (tool == "SampleCorrelation")
	{
		QString header = "Select variant list";
		QString filter = "GSvar files (*.GSvar)";
		if (args_!="")
		{
			header = "Select BAM file";
			filter = "BAM files (*.bam)";
		}
		QString filename1 = getFileName(header , filter);
		if (filename1=="") return;
		QString filename2 = getFileName(header , filter);
		if (filename2=="") return;
		startTool("-in1 " + filename1 + " -in2 " + filename2);
	}
	else if (tool == "SampleDiff")
	{
		QString filename1 = getFileName("Select variant list", "GSvar files (*.GSvar)");
		if (filename1=="") return;
		QString filename2 = getFileName("Select variant list", "GSvar files (*.GSvar)");
		if (filename2=="") return;
		startTool("-in1 " + filename1 + " -in2 " + filename2);
	}
	else
	{
		THROW(ProgrammingException, "Unknown tool '" + tool + "' requested in ExternalToolDialog!");
	}
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

void ExternalToolDialog::startTool(QString arguments)
{
	//init
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	ui_.output->setPlainText("processing...");

	//add default arguments
	arguments = args_ + " " + arguments;
	arguments = arguments.simplified();

	//start timer
	timer_.restart();

	//execute process
	process_ = new QProcess(this);
	connect(process_, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(stateChanged(QProcess::ProcessState)));
	process_->setProgram(ui_.tool_name->text());
	process_->setArguments(arguments.split(" "));
	process_->setProcessChannelMode(QProcess::MergedChannels);
	process_->start();
}

void ExternalToolDialog::stateChanged(QProcess::ProcessState state)
{
	if (state!=QProcess::NotRunning) return;

	//reset cursor
	QApplication::restoreOverrideCursor();

	//display output
	QString output = ">" + ui_.tool_name->text() + " " + process_->arguments().join(" ") + "\n";
	output += ">Processing time: " + Helper::elapsedTime(timer_) + "\n";
	output += ">Exit code: " + QString::number(process_->exitCode()) + "\n\n" + process_->readAll();
	ui_.output->setPlainText(output);
}
