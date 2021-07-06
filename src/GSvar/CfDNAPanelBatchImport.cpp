#include "CfDNAPanelBatchImport.h"
#include "ui_CfDNAPanelBatchImport.h"
#include "Exceptions.h"
#include "NGSD.h"
#include "LoginManager.h"
#include "GUIHelper.h"

CfDNAPanelBatchImport::CfDNAPanelBatchImport(QWidget *parent) :
	QDialog(parent),
	ui_(new Ui::CfDNAPanelBatchImport)
{
	// abort if no connection to NGSD
	if (!LoginManager::active())
	{
		GUIHelper::showMessage("No connection to the NGSD!", "You need access to the NGSD to modify cfDNA panels!");
		this->close();
	}

	ui_->setupUi(this);
	initGUI();
}

CfDNAPanelBatchImport::~CfDNAPanelBatchImport()
{
	delete ui_;
}

void CfDNAPanelBatchImport::initGUI()
{
	//create table headers
	ui_->tw_import_table->setColumnCount(3);
	ui_->tw_import_table->setHorizontalHeaderItem(0, new QTableWidgetItem("Processed sample (tumor)"));
	ui_->tw_import_table->setHorizontalHeaderItem(1, new QTableWidgetItem("Processing system"));
	ui_->tw_import_table->setHorizontalHeaderItem(2, new QTableWidgetItem("File path to cfDNA panel (VCF)"));
}

void CfDNAPanelBatchImport::fillTable()
{
	ui_->tw_import_table->setColumnCount(3);
	ui_->tw_import_table->setRowCount(input_table_.size());
	for (int row_idx = 0; row_idx < input_table_.size(); ++row_idx)
	{
		ui_->tw_import_table->setItem(row_idx, 0, new QTableWidgetItem(QString(input_table_.at(row_idx).processed_sample)));
		ui_->tw_import_table->setItem(row_idx, 1, new QTableWidgetItem(QString(input_table_.at(row_idx).processing_system)));
		ui_->tw_import_table->setItem(row_idx, 2, new QTableWidgetItem(QString(input_table_.at(row_idx).vcf_file_path)));
	}
}

void CfDNAPanelBatchImport::validateTable()
{
	ui_->b_import->setEnabled(false);
	bool valid = true;
	for (int row_idx = 0; row_idx < ui_->tw_import_table->rowCount(); ++row_idx)
	{
		//check processed sample
		QString ps_id = NGSD().processedSampleId(ui_->tw_import_table->item(row_idx, 0)->text(), false);
		if(ps_id == "")
		{
			valid = false;
			ui_->tw_import_table->item(row_idx, 0)->setBackgroundColor(Qt::red);
			ui_->tw_import_table->item(row_idx, 0)->setToolTip("Processed sample not found in NGSD!");
		}

		//check processing system

		//check if file exists
	}
}

void CfDNAPanelBatchImport::parseInput()
{
	input_table_.clear();
	foreach (const QString& line, ui_->te_tsv_input->toPlainText().split('\n'))
	{
		//ignore comments, headers and empty lines
		if (line.trimmed().isEmpty() || line.startsWith("#")) continue;

		QByteArrayList columns = line.toUtf8().split('\t');

		if (columns.size() < 3) THROW(FileParseException, "Table line has to contain at least 3 columms, got " + QString::number(columns.size()) + "!");

		cfDNATableLine table_line;
		table_line.processed_sample = columns.at(0).trimmed();
		table_line.processing_system = columns.at(1).trimmed();
		table_line.vcf_file_path = columns.at(2).trimmed();
		input_table_.append(table_line);
	}
}

void CfDNAPanelBatchImport::importPanels()
{

}
