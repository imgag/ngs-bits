#ifndef VCFTOBEDPE_H
#define VCFTOBEDPE_H

#include <QObject>
#include <QFile>
#include <QDate>
#include "zlib.h"
#include "Exceptions.h"
#include "Helper.h"

class VcfToBedpe
{

public:
	///Constructor, expects path of vcf.gz file as input parameter
	VcfToBedpe(const QByteArray& in_file);

	///struct for the data that is contained in a single (input!) vcf line
	struct vcf_line;

	///Struct for the data that is contained in a single (output!) bedpe line
	struct bedpe_line;

	///parse info field
	static QMap<QByteArray,QByteArray> parseInfoField(const QByteArray& field);

	///Writes bedpe file to "out_file"
	void writeFile(const QString& out_file);



private:
	gzFile file_;
	QList<QByteArray> out_headers_;
	QList<QByteArray> samples_;

	//buffer size for gz file
	int buffer_size_;
	char* buffer_;

	///Returns line from gz file
	QByteArray getLine();

	///Adds info field to header after another header string contains "before"
	void addHeaderInfoFieldAfter(const QByteArray& before,const QByteArray& key, const QByteArray& type, int number, const QByteArray& desc);

	///Parses a whole line from VCF and converts it into Bedpe line
	bedpe_line parseLine(const QByteArray& line_in);

	///Converts VCF line with "simple" SV (=> no mate) into BedpeLine, the end entry in INFO is in this case on same chromosome
	VcfToBedpe::bedpe_line convertSingleLine(const VcfToBedpe::vcf_line& line_in);

	///Converts VCF line with "complex" SV (where we have a Mate) into BedpeLine
	VcfToBedpe::bedpe_line convertComplexLine(const VcfToBedpe::vcf_line& line_in);

};

#endif // VCFTOBEDPE_H
