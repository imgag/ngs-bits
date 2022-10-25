#ifndef CLINVARUPLOADDIALOG_H
#define CLINVARUPLOADDIALOG_H

#include <QDialog>
#include "ui_ClinvarUploadDialog.h"

#include "Phenotype.h"
#include "NGSD.h"

enum class ClinvarSubmissiontype
{
	SingleVariant,
	CompoundHeterozygous
};

//Datastructure for upload data
struct ClinvarUploadData
{
    //sample data
	int variant_id1;
	int report_config_variant_id1;
	int variant_id2 = -1;
	int report_config_variant_id2 = -1;
	ReportVariantConfiguration report_variant_config1;
	ReportVariantConfiguration report_variant_config2;
	QString processed_sample;

    //disease data
    QList<SampleDiseaseInfo> disease_info;
    QString affected_status;

    //phenotype data
    PhenotypeList phenos;

	//type data
	ClinvarSubmissiontype submission_type;
	VariantType variant_type1;
	VariantType variant_type2;

    //variant data
	Variant snv1;
	Variant snv2;
	CopyNumberVariant cnv1;
	CopyNumberVariant cnv2;
	BedpeLine sv1;
	BedpeLine sv2;
    GeneSet genes;

	//additional info for re-upload
	QString stable_id;
	int user_id = -1;
	int variant_publication_id = -1;
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
    bool checkGuiData();
    void printResults();
    void updatePrintButton();

private:
    Ui::ClinVarUploadDialog ui_;
    NGSD db_;
    ClinvarUploadData clinvar_upload_data_;

    QJsonObject createJson();
    bool validateJson(const QJsonObject& json, QStringList& errors);

    static QString getSettings(QString key);
    static QString convertClassification(QString classification);
    static QString convertInheritance(QString inheritance);
    static QString convertAffectedStatus(QString affected_status);


    static const QStringList CLINICAL_SIGNIFICANCE_DESCRIPTION;
    static const QStringList MODE_OF_INHERITANCE;
    static const QStringList AFFECTED_STATUS;
    static const QStringList ALLELE_ORIGIN;
    static const QStringList COLLECTION_METHOD;
    static const QStringList STRUCT_VAR_METHOD_TYPE;
    static const QStringList ASSEMBLY;
    static const QStringList CHR;
    static const QStringList VARIANT_TYPE;

};

#endif // CLINVARUPLOADDIALOG_H
