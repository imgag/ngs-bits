#ifndef LOVDUPLOADDIALOG_H
#define LOVDUPLOADDIALOG_H

#include <QDialog>
#include "ui_LovdUploadDialog.h"

#include "Phenotype.h"
#include "NGSD.h"

//Datastructure for upload data
struct LovdUploadData
{
	//sample data
	QString processed_sample;
	QString gender;
	QString genotype;

	//variant data
	Variant variant;
	QString gene;
	QString nm_number;
	QString hgvs_g;
	QString hgvs_c;
	QString hgvs_p;
	QString classification;

	//phenotype data
	QList<Phenotype> phenos;
};

///LOVD upload dialog
class LovdUploadDialog
	: public QDialog
{
	Q_OBJECT

public:
	LovdUploadDialog(QWidget *parent = 0);

	void setData(const Variant& variant, LovdUploadData data);

private slots:
	void upload();
	void dataToGui();
	void guiToData();
	void checkGuiData();
	void printResults();
	void updatePrintButton();

private:
	Ui::LovdUploadDialog ui_;
	LovdUploadData data_;
	NGSD db_;


	static QByteArray create(const LovdUploadData& data);
	static QString getSettings(QString key);
	static QString convertGender(QString gender);
	static QString convertGenotype(QString genotype);
	static QString convertClassification(QString classification);
	static QString chromosomeToAccession(const Chromosome& chr);

};

#endif // LOVDUPLOADDIALOG_H
