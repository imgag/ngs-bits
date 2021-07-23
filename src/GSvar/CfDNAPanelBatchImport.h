#ifndef CFDNAPANELBATCHIMPORT_H
#define CFDNAPANELBATCHIMPORT_H

#include <QDialog>
#include <VcfFile.h>

struct cfDNATableLine
{
	QByteArray processed_sample;
	QByteArray processing_system;
	QByteArray vcf_file_path;
};

namespace Ui {
class CfDNAPanelBatchImport;
}

class CfDNAPanelBatchImport : public QDialog
{
	Q_OBJECT

public:
	explicit CfDNAPanelBatchImport(QWidget *parent = nullptr);
	~CfDNAPanelBatchImport();

private:
	Ui::CfDNAPanelBatchImport *ui_;
	QVector<cfDNATableLine> input_table_;

	void initGUI();
	void parseInput();
	void fillTable();
	void writeToDbImportLog(const QString& text, bool critical = false);
	VcfFile createCfdnaPanelVcf(const QString& ps_name, const QString& vcf_file_path);



private slots:
	void showRawInputView();
	void showValidationTableView();
	void importTextInput();
	void validateTable();
	void importPanels();

};

#endif // CFDNAPANELBATCHIMPORT_H
