#include "TestFramework.h"

TEST_CLASS(BedExtractMethylationPlotData_Test)
{
	private:
		TEST_METHOD(default_test)
		{
			EXECUTE("BedExtractMethylationPlotData", "-gender male -sites " + TESTDATA("data_in/BedExtractMethylationPlotData_in1_catalog.tsv") + " -out out/BedExtractMethylationPlotData_out1.bed "
				   + " -in_hp1 " + TESTDATA("data_in/BedExtractMethylationPlotData_in1_index_modkit_hp1.bed.gz") + " -in_hp2 " + TESTDATA("data_in/BedExtractMethylationPlotData_in1_index_modkit_hp2.bed.gz") + " -in_all " + TESTDATA("data_in/BedExtractMethylationPlotData_in1_index_modkit.bed.gz")
				   + " -cohort_hp1 " + TESTDATA("data_in/BedExtractMethylationPlotData_in1_cohort01_modkit_hp1.bed.gz") +  " "  + TESTDATA("data_in/BedExtractMethylationPlotData_in1_cohort02_modkit_hp1.bed.gz") +  " "  + TESTDATA("data_in/BedExtractMethylationPlotData_in1_cohort03_modkit_hp1.bed.gz")
				   + " -cohort_hp2 " + TESTDATA("data_in/BedExtractMethylationPlotData_in1_cohort01_modkit_hp2.bed.gz") +  " "  + TESTDATA("data_in/BedExtractMethylationPlotData_in1_cohort02_modkit_hp2.bed.gz") +  " "  + TESTDATA("data_in/BedExtractMethylationPlotData_in1_cohort03_modkit_hp2.bed.gz")
				   + " -cohort_all " + TESTDATA("data_in/BedExtractMethylationPlotData_in1_cohort01_modkit.bed.gz") +  " "  + TESTDATA("data_in/BedExtractMethylationPlotData_in1_cohort02_modkit.bed.gz") +  " "  + TESTDATA("data_in/BedExtractMethylationPlotData_in1_cohort03_modkit.bed.gz")
				   + " -cohort_phasing_tracks " + TESTDATA("data_in/BedExtractMethylationPlotData_in1_cohort01_phasing_track.bed") +  " "  + TESTDATA("data_in/BedExtractMethylationPlotData_in1_cohort02_phasing_track.bed") +  " "  + TESTDATA("data_in/BedExtractMethylationPlotData_in1_cohort03_phasing_track.bed"));
			COMPARE_FILES("out/BedExtractMethylationPlotData_out1.bed", TESTDATA("data_out/BedExtractMethylationPlotData_out1.bed"));
		}
};
