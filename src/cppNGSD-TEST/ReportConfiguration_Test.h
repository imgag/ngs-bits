#include "TestFramework.h"
#include "ReportConfiguration.h"

TEST_CLASS(ReportConfiguration_Test)
{
	Q_OBJECT

private slots:

	void ReportVariantConfiguration_isValid()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex ref_idx(ref_file);

		ReportVariantConfiguration rvc;
		rvc.variant_type = VariantType::SNVS_INDELS;
		rvc.variant_index = 0;
		rvc.report_type = "diagnostic variant";

		//only base settings
		QStringList errors;
		IS_TRUE(rvc.isValid(errors, ref_idx));
		I_EQUAL(errors.count(), 0);

		//manual small variant curation
		rvc.manual_var = "chr1:1-1 N>T";
		rvc.manual_genotype = "hom";
		IS_TRUE(rvc.isValid(errors, ref_idx));
		I_EQUAL(errors.count(), 0);

		//manual CNV curation set for other type
		rvc.manual_cnv_cn = "0";
		rvc.manual_cnv_start = "1";
		rvc.manual_cnv_end = "1";
		IS_FALSE(rvc.isValid(errors, ref_idx));
		I_EQUAL(errors.count(), 3);

		//manual small variant curation set for other type
		rvc.variant_type = VariantType::CNVS;
		IS_FALSE(rvc.isValid(errors, ref_idx));
		I_EQUAL(errors.count(), 2);

		//manual CNV curation
		rvc.manual_var = "";
		rvc.manual_genotype = "";
		IS_TRUE(rvc.isValid(errors, ref_idx));
		I_EQUAL(errors.count(), 0);
	}

	void ReportVariantConfiguration_isManuallyCurated()
	{
		ReportVariantConfiguration rvc;
		rvc.variant_type = VariantType::SNVS_INDELS;
		rvc.variant_index = 0;
		rvc.report_type = "diagnostic variant";

		//only base settings
		IS_FALSE(rvc.isManuallyCurated());

		//small variant
		rvc.manual_var = "chr1:1-1 N>T";
		IS_TRUE(rvc.isManuallyCurated());

		//small variant genotype
		rvc.manual_var = "";
		rvc.manual_genotype = "het";
		IS_TRUE(rvc.isManuallyCurated());

		//CNV start
		rvc.manual_genotype = "";
		rvc.variant_type = VariantType::CNVS;
		rvc.manual_cnv_start = "1";
		IS_TRUE(rvc.isManuallyCurated());

		//CNV end
		rvc.manual_cnv_start = "";
		rvc.manual_cnv_end = "1";
		IS_TRUE(rvc.isManuallyCurated());

		//CNV copy-number
		rvc.manual_cnv_end = "";
		rvc.manual_cnv_cn = "0";
		IS_TRUE(rvc.isManuallyCurated());

		//CNV no curation
		rvc.manual_cnv_cn = "";
		IS_FALSE(rvc.isManuallyCurated());

		//SV start
		rvc.manual_cnv_cn = "";
		rvc.variant_type = VariantType::SVS;
		rvc.manual_sv_start = "1";
		IS_TRUE(rvc.isManuallyCurated());

		//SV end
		rvc.manual_sv_start = "";
		rvc.manual_sv_end = "1";
		IS_TRUE(rvc.isManuallyCurated());

		//SV genotype
		rvc.manual_sv_end = "";
		rvc.manual_sv_genotype = "het";
		IS_TRUE(rvc.isManuallyCurated());

		//SV start 2
		rvc.manual_sv_genotype = "";
		rvc.manual_sv_start_bnd = "1";
		IS_TRUE(rvc.isManuallyCurated());

		//SV end 2
		rvc.manual_sv_start_bnd = "";
		rvc.manual_sv_end_bnd = "1";
		IS_TRUE(rvc.isManuallyCurated());
	}

	void ReportVariantConfiguration_manualVarIsValid()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex ref_idx(ref_file);

		ReportVariantConfiguration rvc;
		rvc.variant_type = VariantType::SNVS_INDELS;
		rvc.variant_index = 0;
		rvc.report_type = "diagnostic variant";

		IS_FALSE(rvc.manualVarIsValid(ref_idx));

		rvc.manual_var = "chr1:1-1 N>T";
		IS_TRUE(rvc.manualVarIsValid(ref_idx));

		//invalid reference base
		rvc.manual_var = "chr1:1-1 A>T";
		IS_FALSE(rvc.manualVarIsValid(ref_idx));
	}

	void ReportVariantConfiguration_manualVarGenoIsValid()
	{
		ReportVariantConfiguration rvc;
		rvc.variant_type = VariantType::SNVS_INDELS;
		rvc.variant_index = 0;
		rvc.report_type = "diagnostic variant";

		IS_FALSE(rvc.manualVarGenoIsValid());

		rvc.manual_genotype = "het";
		IS_TRUE(rvc.manualVarGenoIsValid());

		rvc.manual_genotype = "hom";
		IS_TRUE(rvc.manualVarGenoIsValid());

		rvc.manual_genotype = "bla";
		IS_FALSE(rvc.manualVarGenoIsValid());
	}

	void ReportVariantConfiguration_manualCnvStartIsValid()
	{
		ReportVariantConfiguration rvc;
		rvc.variant_type = VariantType::CNVS;
		rvc.variant_index = 0;
		rvc.report_type = "diagnostic variant";

		IS_FALSE(rvc.manualCnvStartIsValid());

		rvc.manual_cnv_start = "1";
		IS_TRUE(rvc.manualCnvStartIsValid());

		//invalid coordinate
		rvc.manual_cnv_start = "0";
		IS_FALSE(rvc.manualCnvStartIsValid());
	}

	void ReportVariantConfiguration_manualCnvEndIsValid()
	{
		ReportVariantConfiguration rvc;
		rvc.variant_type = VariantType::CNVS;
		rvc.variant_index = 0;
		rvc.report_type = "diagnostic variant";

		IS_FALSE(rvc.manualCnvEndIsValid());

		rvc.manual_cnv_end = "1";
		IS_TRUE(rvc.manualCnvEndIsValid());

		//invalid coordinate
		rvc.manual_cnv_end = "0";
		IS_FALSE(rvc.manualCnvEndIsValid());
	}

	void ReportVariantConfiguration_manualCnvCnIsValid()
	{
		ReportVariantConfiguration rvc;
		rvc.variant_type = VariantType::CNVS;
		rvc.variant_index = 0;
		rvc.report_type = "diagnostic variant";

		IS_FALSE(rvc.manualCnvCnIsValid());

		rvc.manual_cnv_cn = "0";
		IS_TRUE(rvc.manualCnvCnIsValid());

		//invalid copy-number
		rvc.manual_cnv_cn = "-1";
		IS_FALSE(rvc.manualCnvCnIsValid());
	}

	void ReportVariantConfiguration_manualSvStartIsValid()
	{
		ReportVariantConfiguration rvc;
		rvc.variant_type = VariantType::SVS;
		rvc.variant_index = 0;
		rvc.report_type = "diagnostic variant";

		IS_FALSE(rvc.manualSvStartIsValid());

		rvc.manual_sv_start = "1";
		IS_TRUE(rvc.manualSvStartIsValid());

		//invalid coordinate
		rvc.manual_sv_start = "0";
		IS_FALSE(rvc.manualSvStartIsValid());
	}

	void ReportVariantConfiguration_manualSvEndIsValid()
	{
		ReportVariantConfiguration rvc;
		rvc.variant_type = VariantType::SVS;
		rvc.variant_index = 0;
		rvc.report_type = "diagnostic variant";

		IS_FALSE(rvc.manualSvEndIsValid());

		rvc.manual_sv_end = "1";
		IS_TRUE(rvc.manualSvEndIsValid());

		//invalid coordinate
		rvc.manual_sv_end = "0";
		IS_FALSE(rvc.manualSvEndIsValid());
	}

	void ReportVariantConfiguration_manualSVGenoIsValid()
	{
		ReportVariantConfiguration rvc;
		rvc.variant_type = VariantType::SVS;
		rvc.variant_index = 0;
		rvc.report_type = "diagnostic variant";

		IS_FALSE(rvc.manualSvGenoIsValid());

		rvc.manual_sv_genotype = "het";
		IS_TRUE(rvc.manualSvGenoIsValid());

		rvc.manual_sv_genotype = "hom";
		IS_TRUE(rvc.manualSvGenoIsValid());

		rvc.manual_sv_genotype = "bla";
		IS_FALSE(rvc.manualSvGenoIsValid());
	}

	void ReportVariantConfiguration_manualSvStartBndIsValid()
	{
		ReportVariantConfiguration rvc;
		rvc.variant_type = VariantType::SVS;
		rvc.variant_index = 0;
		rvc.report_type = "diagnostic variant";

		IS_FALSE(rvc.manualSvStartBndIsValid());

		rvc.manual_sv_start_bnd = "1";
		IS_TRUE(rvc.manualSvStartBndIsValid());

		//invalid coordinate
		rvc.manual_sv_start_bnd = "0";
		IS_FALSE(rvc.manualSvStartBndIsValid());
	}

	void ReportVariantConfiguration_manualSvEndBndIsValid()
	{
		ReportVariantConfiguration rvc;
		rvc.variant_type = VariantType::SVS;
		rvc.variant_index = 0;
		rvc.report_type = "diagnostic variant";

		IS_FALSE(rvc.manualSvEndBndIsValid());

		rvc.manual_sv_end_bnd = "1";
		IS_TRUE(rvc.manualSvEndBndIsValid());

		//invalid coordinate
		rvc.manual_sv_end_bnd = "0";
		IS_FALSE(rvc.manualSvEndBndIsValid());
	}

};
