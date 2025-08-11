#ifndef VCFTOBEDPE_H
#define VCFTOBEDPE_H

#include <QObject>
#include <QFile>
#include <QDate>
#include "Exceptions.h"
#include "Helper.h"
#include "BedpeFile.h"
#include "VersatileFile.h"

class VcfToBedpe
{
	friend class VcfToBedpe_Test;
public:
	///Constructor, expects path of vcf.gz file as input parameter
	VcfToBedpe(const QByteArray& filename);

	///Writes bedpe file to "out_file"
	void convert(QString out_file);

private:
	QByteArray filename_;
	VersatileFile file_;
	QList<QByteArray> out_headers_;
	QList<QByteArray> samples_;

	///struct for the data that is contained in a single (input!) vcf line
	struct VcfLineInternal;

	///Struct for the data that is contained in a single (output!) bedpe line
	struct BedpeLineInternal;

	///parses an INFO field (usually contained in header)  and returns entries as key value pairs
	static QMap<QByteArray,QByteArray> parseInfoField(const QByteArray& field);

	///Adds info field to header after another header string contains "before"
	void addHeaderInfoFieldAfter(const QByteArray& before,const QByteArray& key, const QByteArray& type, int number, const QByteArray& desc);

	///Converts VCF line with "simple" SV (=> no mate) into BedpeLine, the end entry in INFO is in this case on same chromosome
	VcfToBedpe::BedpeLineInternal convertSingleLine(const VcfLineInternal& line_in, bool single_manta_bnd = false);

	///Converts VCF line with "complex" SV (where we have a Mate) into BedpeLine
	VcfToBedpe::BedpeLineInternal convertComplexLine(const VcfLineInternal& line_a, const VcfLineInternal& line_b, bool mate_missing = false);

	///adds entry into INFO field after certain entry
	static QByteArray newInfoFieldAfterKey(const QByteArray& info_old,const QByteArray& key_before, const QByteArray& key, const QByteArray& data);

	///neccessary for test cases
	VcfToBedpe();
};

#endif // VCFTOBEDPE_H
