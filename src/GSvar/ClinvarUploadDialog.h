#ifndef CLINVARUPLOADDIALOG_H
#define CLINVARUPLOADDIALOG_H

#include <QDialog>
#include "ui_ClinvarUploadDialog.h"
#include "ClinvarUploadData.h"
#include "NGSD.h"


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
	void addCompHetVariant();
	void removeCompHetVariant();
	void selectVariantType(int i);
    void updateButtons();
	void setDiseaseInfo();
	void addDiseaseInfo();
	void removeDiseaseInfo();
	void diseaseContextMenu(QPoint pos);

private:
    Ui::ClinVarUploadDialog ui_;
    NGSD db_;
	bool manual_upload_ = true;
    ClinvarUploadData clinvar_upload_data_;
	bool upload_running_ = false;

    QJsonObject createJson();
    bool validateJson(const QJsonObject& json, QStringList& errors);
	QString getHGVS();
	///Returns db table name for variant
	QString getDbTableName(bool var2 = false);

    void loadData();
    void updateGui();

    static QString getSettings(QString key);
	static QString convertClassification(QString classification, bool reverse=false);
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
