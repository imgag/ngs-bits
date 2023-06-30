#ifndef REPORTSETTINGS_H
#define REPORTSETTINGS_H

#include "cppNGSD_global.h"
#include "VariantType.h"
#include "ReportConfiguration.h"
#include <QStringList>

///Report meta data.
class CPPNGSDSHARED_EXPORT ReportSettings
{
public:
	///Default constructor
	ReportSettings();

	QSharedPointer<ReportConfiguration> report_config; //report configuration
	QString report_type;
	QList<QPair<VariantType, int>> selected_variants;
	bool select_other_causal_variant = false;

	bool show_coverage_details; //show low-coverage details
	bool recalculate_avg_depth; //average coverage should be calculated for the target region (otherwise the processing system average depth is used)
	int min_depth; //depth cutoff for gap statistics
	int cov_exon_padding; //number of bases to pad the gene exons with.
	bool cov_based_on_complete_roi; //calcualte second gap report based on the complete target region.

	bool show_omim_table; //show OMIM table
	bool show_one_entry_in_omim_table; //show only one phenotype entry per gene in OMIM table

	bool show_class_details; //show classification information
	bool show_refseq_transcripts; //add matching RefSeq transcript names after Ensembl transcript names
	QString language;
};


#endif // REPORTSETTINGS_H
