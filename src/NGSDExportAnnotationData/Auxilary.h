#ifndef AUXILARY_H
#define AUXILARY_H

#include <QString>
#include <QHash>
#include <QSet>
#include "cmath"

//Parameters for germline export
struct ExportParameters
{
	//general stuff
	QString ref_file;
	QString version;
	QString datetime; //used to generate temporary file names
	int threads;
	int max_vcf_lines;

	//germline paramters
	QString germline;
	double max_af;

	//somatic parameters
	QString somatic;
	bool vicc_config_details;

	//gene export parameters
	QString genes;
	int gene_offset;

	//debugging
	bool use_test_db;
	bool verbose;

	QString tempVcf(QString chr, QString mode) const;
};


//Helper struct for processed sample data
struct ProcessedSampleInfo
{
	bool bad_quality = false;
	int s_id = -1;
	bool affected = false;
	QString disease_group = "";
};

//Helper struct for classification data
struct ClassificationData
{
	QByteArray classification = "";
	QByteArray comment = "";
};

//Data cached from NGSD to speed up processing
struct SharedData
{
	QStringList chrs;
	QHash<int, ProcessedSampleInfo> ps_infos; //Sample data cached from NGSD to speed up processing
	QHash<int, ClassificationData> class_infos; //Classification data cached from NGSD to speed up processing
	QSet<int> somatic_variant_ids; //variant ids of somatic variants (looking them up once is faster then a join between 'variant' and 'detected_somatic_variant'
};

//Returns a formatted time string from a given time in milliseconds
QByteArray getTimeString(double milliseconds);

#endif // AUXILARY_H
