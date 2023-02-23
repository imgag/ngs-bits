#ifndef EXPORTCBIOPORTALSTUDY_H
#define EXPORTCBIOPORTALSTUDY_H

#include <QString>
#include <QStringList>
#include <QMap>
#include "Exceptions.h"
#include "cppNGSD_global.h"
#include "VariantList.h"
#include "NGSD.h"
#include "SomaticReportSettings.h"

class CPPNGSDSHARED_EXPORT MetaFile
{
public:
	MetaFile();

	///store values as metafile at the out path
	void store(const QString& out);
	///load the values from the metafile at in
	void load(const QString& in); // needed?

	void addValue(QString key, QString value, bool throw_if_exists=false)
	{
		//TODO verify key and value?
		//key no colon, space, whitespace?
		//value any requirements

		if (values_.contains(key))
		{
			if (throw_if_exists)
			{
				THROW(ArgumentException, "Key already exists: " + key);
			}

			values_[key] = value;
		}
		else
		{
			values_.insert(key, value);
		}
	};
	void removeValue(QString key)
	{
		values_.remove(key);
	};

private:
	QMap<QString, QString> values_;
};


struct CPPNGSDSHARED_EXPORT StudyData
{
	//Study data
	QString name;
	QString cancer_type;
	QString identifier;	// study name in db.
	QString description;
	QString reference_genome;
};

struct CPPNGSDSHARED_EXPORT CancerData
{
	//cancer data
	QString description;
	QString color;
	QString parent;
};

enum Attribute
	{
	  SAMPLE_ID
	, PATIENT_ID
	, MSI_STATUS
	, PLOIDY
	, PURITY_HIST
	, PURITY_CNVS
	, PROCESSING_SYSTEM
	, COMMENT
	, HRD_SCORE
	, TMB
	, ICD10
	, HPO_TERMS
	, CLINICAL_PHENOTYPE
	};

struct CPPNGSDSHARED_EXPORT SampleAttribute
{
	QString name;			// display name for attribute ??? change to enum and write function to get corresponding data out of data structure based on enum value?.
	QString db_name;		// attribute name for the database, should be uppercase.
	QString description;	// Description that can't contain \t
	QString datatype;		// Examples: STRING, NUMBER, BOOLEAN
	Attribute attribute;	// enum that specifies which data from the sample will be exported.
	int priority;			// Visual priority of attribute, decides how prominent the plots of this property are (0 -> hidden, 1 default, no upper limit)


};


struct CPPNGSDSHARED_EXPORT PatientData
{
	QString patient_id;
	QString gender;
	QList<SampleData> samples;

};

struct CPPNGSDSHARED_EXPORT SampleFiles
{
	QString gsvar_somatic;
	QString gsvar_germline;
	QString clincnv_file;
	QString msi_file;
};



class CPPNGSDSHARED_EXPORT CBioPortalExportSettings
{

public:
	CBioPortalExportSettings(StudyData study, bool ngsd_test=false);
	CBioPortalExportSettings(const CBioPortalExportSettings& other);

	StudyData study;
	CancerData cancer;

	QStringList sample_list;
	QList<SomaticReportSettings> report_settings;
	QList<SampleFiles> sample_files;

	QStringList ps_ids;
	QList<ProcessedSampleData> ps_data;


	QList<SampleAttribute> sample_attributes; //required attributes: Patient_id, sample_id  ||  Others listed: cancer_type, cancer_type_detailed, sample_display_name, sample_class)
	QMap<QString, PatientData> patient_map;


	void addSample(SomaticReportSettings settings, PatientData patient, SampleFiles files);

	void setCancerData(CancerData cancer_data)
	{
		cancer = cancer_data;
	}

	void addSampleAttribute(SampleAttribute attribute)
	{
		sample_attributes.append(attribute);
	}

	//get index of
	int getSampleIndex(QString name)
	{
		return sample_list.indexOf(name);
	}

	double getMsiStatus(int sample_idx);
	float getPloidy(int sample_idx);
	float getPurityHist(int sample_idx);
	float getPurityCnvs(int sample_idx);
	QString getProcessingSystem(int sample_idx);
	QString getComments(int sample_idx);
	int getHrdScore(int sample_idx);
	float getTmb(int sample_idx);
	QStringList getIcd10(int sample_idx);
	QStringList getHpoTerms(int sample_idx);
	QString getClinicalPhenotype(int sample_idx);
	QString getFormatedAttribute(Attribute att, int sample_idx);

private:
	NGSD db_;

};


class CPPNGSDSHARED_EXPORT ExportCBioPortalStudy
		: public QObject
{
public:
	ExportCBioPortalStudy(CBioPortalExportSettings settings, bool test_db);

	void exportStudy(const QString& out_folder);

private:
	void gatherData();
	//write meta_study.txt file
	void exportStudyFiles(const QString& out_folder);
	void exportCancerType(const QString& out_folder);
	void exportPatientData(const QString& out_folder);
	void exportSampleData(const QString& out_folder);
	void exportSnvs(const QString& out_folder);

	NGSD db_;
	CBioPortalExportSettings settings_;



};

#endif // EXPORTCBIOPORTALSTUDY_H
