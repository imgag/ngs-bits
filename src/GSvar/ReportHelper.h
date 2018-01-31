#ifndef REPORTHELPER_H
#define REPORTHELPER_H

#include "VariantList.h"
#include "GeneSet.h"
#include "CnvList.h"
#include "BedFile.h"
#include "NGSD.h"
#include "Settings.h"
#include "QCCollection.h"
#include <QDialog>

///provides methods for generating rtf file
class RtfTools
{
public:
	///specifies the header of an RTF file
	static void writeRtfHeader(QTextStream& stream);
	///generates the header of a single Rtf row
	static void writeRtfTableSingleRowSpec(QTextStream& stream,const QList<int>& col_widths, bool border);
	///generates a RTF table
	static void writeRtfWholeTable(QTextStream& stream, const QList< QList<QString> >& table, const QList<int>& col_widths, int font_size, bool border, bool bold);
	///generates a single row for an RTF table
	static void writeRtfRow(QTextStream& stream, const QList<QString>& columns, const QList<int>& col_widths, int font_size, bool border, bool bold);

	///generates table with important SNVs including mutational burden
	static void writeRtfTableSNV(QTextStream& stream, const QList<int>& colWidths, const VariantList& important_snvs, const VariantList& further_genes, double mutation_burden);
	///generates table with important CNVs
	static void writeRtfTableCNV(QTextStream& stream, const QList<int>& colWidths, const CnvList& important_cnvs, QString target_region);
};


///creates a somatic RTF report
class ReportHelper
{
public:
	ReportHelper();
	ReportHelper(QString snv_filename, GeneSet snv_germline_filter, GeneSet cnv_keep_genes_filter, QString target_region);
	///write Rtf File
	void writeRtf(const QString& out_file);

private:
	///Filters snv_variants_ for SNVs classified by CGI as drivers
	VariantList filterSnvForCGIDrivers();
	///Filters snv_variants_ for SNVs classified by CGI as passengers/non-protein-coding.
	VariantList filterSnvForCGIPassengers();
	///Filters cnv_cariants_, CNVs with zScors < 5 and without genes in cnv_keep_genes_filter_ will be discarded
	///if del_genes_not_in_target_region is set, all genes which do not lie in target region will be removed
	CnvList filterCnv();

	///make gap statistics, grouped by gene as QByteArray and regions as BedFile
	QHash<QByteArray, BedFile> gapStatistics();


	///Somatic filenames
	QString snv_filename_;
	QString cnv_filename_;

	///tumor ID
	QString tumor_id_;
	///normal ID
	QString normal_id_;

	///Input CnvList and VariantList
	CnvList cnv_variants_;
	VariantList snv_variants_;

	NGSD db_;
	///Target region
	BedFile roi_;
	QString target_region_;

	QCCollection qcml_data_;

	///Geneset with important genes for germline SNV report
	GeneSet snv_germline_filter_;
	///Geneset with important genes for germline CNV report
	GeneSet cnv_germline_filter_;
	///Geneset with genes to be kept for CNV report
	GeneSet cnv_keep_genes_filter_;

	///Geneset with genes which appear in the technical report for reimbursement
	GeneSet genes_for_reimbursement_;

	///CGI cancer acronym (extracted from .GSVar file)
	QString cgi_cancer_type_;

	///ICD10 text diagnosis tumor
	QString icd10_diagnosis_text_;

	///indices for variant files
	int snv_index_filter_;
	int snv_index_cgi_driver_statement_;
	int snv_index_cgi_gene_role;
	int snv_index_cgi_transcript;
	int snv_index_variant_type_;
	int snv_index_classification_;
	int snv_index_gene_;

	///indices for cnv files
	int cnv_index_cgi_genes_;
};

#endif // REPORTHELPER_H
