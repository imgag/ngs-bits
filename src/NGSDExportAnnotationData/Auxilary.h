#ifndef AUXILARY_H
#define AUXILARY_H

#include <QString>
#include <QHash>
#include "cmath"

//Parameters for germline export
struct GermlineParameters
{
	//general stuff
	QString ref_file;
	QString version;

	//main paramters
	QString output_file;
	int threads;
	double max_af;
	int max_vcf_lines;

	//gene export parameters
	QString genes;
	int gene_offset;

	//debugging
	bool use_test_db;
	bool verbose;

	QString toString() const
	{
		return QString("test_db=")+(use_test_db ? "yes" : "no");
	}
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
	QHash<QString, QString> chr2vcf;
	QHash<int, ProcessedSampleInfo> ps_infos; //Sample data cached from NGSD to speed up processing
	QHash<int, ClassificationData> class_infos; //Classification data cached from NGSD to speed up processing
};

//Returns a formatted time string from a given time in milliseconds
QByteArray getTimeString(double milliseconds);

#endif // AUXILARY_H
