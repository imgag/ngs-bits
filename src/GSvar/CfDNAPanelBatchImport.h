#ifndef CFDNAPANELBATCHIMPORT_H
#define CFDNAPANELBATCHIMPORT_H

#include <QDialog>

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
	void fillTable();
	void validateTable();


private slots:
	void parseInput();
	void importPanels();

};

#endif // CFDNAPANELBATCHIMPORT_H
