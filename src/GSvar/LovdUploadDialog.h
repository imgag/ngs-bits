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

	//phenotype data
	QList<Phenotype> phenos;

	//variant data
	Variant variant;
	QString gene;
	QString nm_number;
	QString hgvs_g;
	QString hgvs_c;
	QString hgvs_p;
	QString classification;
	QString genotype;
	QList<VariantTranscript> trans_data;

	//variant data second variant (e.g. comp-het)
	Variant variant2;
	QString hgvs_g2;
	QString hgvs_c2;
	QString hgvs_p2;
	QString classification2;
	QString genotype2;
	QList<VariantTranscript> trans_data2;
};

///LOVD upload dialog
class LovdUploadDialog
	: public QDialog
{
	Q_OBJECT

public:
	LovdUploadDialog(QWidget *parent = 0);

	void setData(LovdUploadData data);

private slots:
	void upload();
	void checkGuiData();
	void printResults();
	void updatePrintButton();
	void updateSecondVariantGui();
	void setTranscriptInfoVariant1();
	void setTranscriptInfoVariant2();

private:
	Ui::LovdUploadDialog ui_;
	NGSD db_;
	Variant variant1_;
	Variant variant2_;
	LovdUploadData data_;

	//Returns if compound-heterozygous mode is active, i.e. two variants were set through 'setData' or using the GUI
	bool isCompHet() const;
	QByteArray createJson();
	static void createJsonForVariant(QTextStream& stream, QString chr, QString gene, QString transcript, QLineEdit* hgvs_g, QLineEdit* hgvs_c, QLineEdit* hgvs_p, QComboBox* genotype, QComboBox* classification);
	static QString getSettings(QString key);
	static QString convertGender(QString gender);
	static QString convertGenotype(QString genotype);
	static QString convertClassification(QString classification);
	static QString chromosomeToAccession(const Chromosome& chr);
};

#endif // LOVDUPLOADDIALOG_H
