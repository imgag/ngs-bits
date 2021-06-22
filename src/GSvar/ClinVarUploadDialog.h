#ifndef CLINVARUPLOADDIALOG_H
#define CLINVARUPLOADDIALOG_H

#include <QDialog>
#include "ui_ClinvarUploadDialog.h"

#include "Phenotype.h"
#include "NGSD.h"

//struct ClinvarUploadDataSchema
//{
	//clinvarSubmisson

		//assertionCriteria
			//citation
				//db
				//id
				//url
			//method

		//clinicalSignificance
			//citation
				//db
				//id
				//url
			//clinicalSignificanceDescription
			//comment
			//dateLastEvaluated
			//modeOfInheritance


		//clinvarAccession

		//conditionSet
			//condition
				//db
				//id
				//name

		//localID

		//localKey

		//observedIn
			//affectedStatus
			//alleleOrigin
			//clinicalFeatures
				//db
				//id
				//name
			//clinicalFeaturesComment
			//collectionMethod
			//numberOfIndividuals
			//structVarMethodType

		//recordStatus

		//releaseStatus

		//variantSet
			//variant
				//chromosomeCoordinates
					//accession
					//alternateAllele
					//assembly
					//chromosome
					//innertStart
					//innertStop
					//outerStart
					//outerStop
					//referenceAllele
					//start
					//stop
					//variantLength
				//copyNumber
				//gene
					//id
					//symbol
				//hgvs
				//referenceCopyNumber
				//variantType


//};

//namespace Ui {
//class ClinvarUploadDialog;
//}

//class ClinvarUploadDialog
//		: public QDialog
//{
//	Q_OBJECT

//public:
//	explicit ClinvarUploadDialog(QWidget *parent = 0);
//	~ClinvarUploadDialog();

//private:
//	Ui::ClinvarUploadDialog *ui;
//};

//Datastructure for upload data
struct ClinvarUploadData
{
	//sample data
	QString processed_sample;
	QString gender;

	//phenotype data
	PhenotypeList phenos;

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

///ClinVar upload dialog
class ClinvarUploadDialog
	: public QDialog
{
	Q_OBJECT

public:

	ClinvarUploadDialog(QWidget *parent = 0);

	void setData(ClinvarUploadData data);

private slots:
    void initGui();
	void upload();
	void checkGuiData();
	void printResults();
	void updatePrintButton();
	void updateSecondVariantGui();
	void setTranscriptInfoVariant1();
	void setTranscriptInfoVariant2();

private:
	Ui::ClinVarUploadDialog ui_;
	NGSD db_;
	Variant variant1_;
	Variant variant2_;
	ClinvarUploadData data_;

	//Returns if compound-heterozygous mode is active, i.e. two variants were set through 'setData' or using the GUI
	bool isCompHet() const;
//	QByteArray createJson();
    QJsonObject createJson();

	static void createJsonForVariant(QTextStream& stream, QString chr, QString gene, QString transcript, QLineEdit* hgvs_g, QLineEdit* hgvs_c, QLineEdit* hgvs_p, QComboBox* genotype, QComboBox* classification);
	static QString getSettings(QString key);
	static QString convertGender(QString gender);
	static QString convertGenotype(QString genotype);
	static QString convertClassification(QString classification);
	static QString chromosomeToAccession(const Chromosome& chr);

    static const QStringList CLINICAL_SIGNIFICANCE_DESCRIPTION;
    static const QStringList MODE_OF_INHERITANCE;
    static const QStringList AFFECTED_STATUS;
    static const QStringList ALLELE_ORIGIN;
    static const QStringList COLLECTION_METHOD;
    static const QStringList STRUCT_VAR_METHOD_TYPE;
    static const QStringList CHR;
    static const QStringList VARIANT_TYPE;

};

#endif // CLINVARUPLOADDIALOG_H
