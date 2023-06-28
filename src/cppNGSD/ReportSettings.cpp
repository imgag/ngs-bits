#include "ReportSettings.h"

ReportSettings::ReportSettings()
	: report_config(new ReportConfiguration())
	, selected_variants()
	, show_coverage_details(true)
	, recalculate_avg_depth(false)
	, min_depth(20)
	, cov_exon_padding(20)
	, cov_based_on_complete_roi(false)
	, show_omim_table(true)
	, show_one_entry_in_omim_table(true)
	, show_class_details(false)
	, show_refseq_transcripts(false)
	, language("german")
{
}
