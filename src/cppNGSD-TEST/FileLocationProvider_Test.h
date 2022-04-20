#include "TestFramework.h"
#include "FileLocationProvider.h"
#include "FileLocationProviderLocal.h"

TEST_CLASS(FileLocationProvider_Test)
{
Q_OBJECT
private slots:
	void get_files()
	{
		// Single
		QString filename = "data_in/VariantFilter_in.GSvar";

		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		SampleHeaderInfo emptyHeader {};

		IS_THROWN(ArgumentException, FileLocationProviderLocal("", vl.getSampleHeader(), vl.type()));

		IS_THROWN(ArgumentException, FileLocationProviderLocal(filename, emptyHeader, vl.type()));

		FileLocationProviderLocal fp = FileLocationProviderLocal(filename, vl.getSampleHeader(), vl.type());
		QString sample_folder = QFileInfo(filename).absolutePath();
		QString project_folder = QFileInfo(sample_folder).absolutePath();

		FileLocationList bam_files = fp.getBamFiles(true);
		I_EQUAL(bam_files.length(), 1);
		S_EQUAL(bam_files[0].filename, sample_folder+"/NA12878_03.bam");

		FileLocationList cnv_files = fp.getCnvCoverageFiles(true);
		I_EQUAL(cnv_files.length(), 1);
		S_EQUAL(cnv_files[0].filename, sample_folder+"/NA12878_03_cnvs_clincnv.seg");

		FileLocationList igv_files = fp.getBafFiles(true);
		I_EQUAL(igv_files.length(), 1);
		S_EQUAL(igv_files[0].filename, sample_folder+"/NA12878_03_bafs.igv");

		FileLocationList manta_files = fp.getMantaEvidenceFiles(true);
		I_EQUAL(manta_files.length(), 1);
		S_EQUAL(manta_files[0].filename, sample_folder+"/manta_evid/NA12878_03_manta_evidence.bam");


		// Multi
		filename = "data_in/VariantFilter_in_multi.GSvar";
		vl.load(TESTDATA("data_in/VariantFilter_in_multi.GSvar"));
		fp = FileLocationProviderLocal(filename, vl.getSampleHeader(), vl.type());

		bam_files = fp.getBamFiles(true);
		I_EQUAL(bam_files.length(), 4);
		S_EQUAL(bam_files[0].filename, project_folder+"/Sample_Affected1/Affected1.bam");
		S_EQUAL(bam_files[1].filename, project_folder+"/Sample_Affected2/Affected2.bam");
		S_EQUAL(bam_files[2].filename, project_folder+"/Sample_Control1/Control1.bam");
		S_EQUAL(bam_files[3].filename, project_folder+"/Sample_Control2/Control2.bam");

		cnv_files = fp.getCnvCoverageFiles(true);
		I_EQUAL(cnv_files.length(), 4);
		S_EQUAL(cnv_files[0].filename, project_folder+"/Sample_Affected1/Affected1_cnvs_clincnv.seg");
		S_EQUAL(cnv_files[1].filename, project_folder+"/Sample_Affected2/Affected2_cnvs_clincnv.seg");
		S_EQUAL(cnv_files[2].filename, project_folder+"/Sample_Control1/Control1_cnvs_clincnv.seg");
		S_EQUAL(cnv_files[3].filename, project_folder+"/Sample_Control2/Control2_cnvs_clincnv.seg");

		igv_files = fp.getBafFiles(true);
		I_EQUAL(igv_files.length(), 4);
		S_EQUAL(igv_files[0].filename, project_folder+"/Sample_Affected1/Affected1_bafs.igv");
		S_EQUAL(igv_files[1].filename, project_folder+"/Sample_Affected2/Affected2_bafs.igv");
		S_EQUAL(igv_files[2].filename, project_folder+"/Sample_Control1/Control1_bafs.igv");
		S_EQUAL(igv_files[3].filename, project_folder+"/Sample_Control2/Control2_bafs.igv");


		manta_files = fp.getMantaEvidenceFiles(true);
		I_EQUAL(manta_files.length(), 4);
		S_EQUAL(manta_files[0].filename, project_folder+"/Sample_Affected1/manta_evid/Affected1_manta_evidence.bam");
		S_EQUAL(manta_files[1].filename, project_folder+"/Sample_Affected2/manta_evid/Affected2_manta_evidence.bam");
		S_EQUAL(manta_files[2].filename, project_folder+"/Sample_Control1/manta_evid/Control1_manta_evidence.bam");
		S_EQUAL(manta_files[3].filename, project_folder+"/Sample_Control2/manta_evid/Control2_manta_evidence.bam");
	}
};
