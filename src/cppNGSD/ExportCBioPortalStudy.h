#ifndef EXPORTCBIOPORTALSTUDY_H
#define EXPORTCBIOPORTALSTUDY_H

#include <QString>
#include <QStringList>
#include <QMap>
#include "Exceptions.h"
#include "cppNGSD_global.h"

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

	bool is_set;
};

struct CPPNGSDSHARED_EXPORT CancerData
{
	//cancer data
	QString description;
	QString color;
	QString parent;

	bool is_set;
};

struct CPPNGSDSHARED_EXPORT SampleAttribute
{
	QString name;			// display name for attribute
	QString db_name;		// attribute name for the database, should be uppercase.
	QString description;	// Description that can't contain \t
	QString datatype;		// Examples: STRING, NUMBER, BOOLEAN
	int priority;			// Visual priority of attribute, decides how prominent the plots of this property are (0 -> hidden, 1 default, no upper limit)
};

struct CPPNGSDSHARED_EXPORT SampleData
{
	QString sample_id;
	QString processing_system;
	QString comments;


//	QString cancer_type;
//	QString cancer_subtype;
	//TODO

};

struct CPPNGSDSHARED_EXPORT PatientData
{
	QString patient_id;
	QString gender;
	QList<SampleData> samples;

};

class CPPNGSDSHARED_EXPORT ExportCBioPortalStudy
		: public QObject
{
public:
	ExportCBioPortalStudy();

	void exportStudy(const QString& out_folder);

	void setStudy(StudyData study);
	void setCancer(CancerData cancer);


private:
	void gatherData();
	//write meta_study.txt file
	void exportStudyFiles(const QString& out_folder);
	void exportCancerType(const QString& out_folder);
	void exportClinicalData(const QString& out_folder);
	void exportSampleData(const QString& out_folder);


	StudyData study_;
	CancerData cancer_;


	QStringList sample_list;
	QList<SampleAttribute> sample_attributes; //required attributes: Patient_id, sample_id  ||  Others listed: cancer_type, cancer_type_detailed, sample_display_name, sample_class)
	QList<PatientData> patients_;
};

#endif // EXPORTCBIOPORTALSTUDY_H
