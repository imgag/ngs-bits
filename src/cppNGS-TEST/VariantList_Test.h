#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "VariantList.h"
#include "Settings.h"

TEST_CLASS(VariantList_Test)
{
Q_OBJECT
private slots:


	void VariantTranscript_idWithoutVersion()
	{
		VariantTranscript trans;
		trans.id = "ENST00000493901";
		S_EQUAL(trans.idWithoutVersion(), "ENST00000493901");
		trans.id = "ENST00000493901.1";
		S_EQUAL(trans.idWithoutVersion(), "ENST00000493901");
	}

	void leftAlign()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		VariantList vl;
		vl.load(TESTDATA("data_in/LeftAlign_in.GSvar"));
		vl.checkValid();
		vl.leftAlign(ref_file, false);
		vl.store("out/LeftAlign_out.GSvar");
		COMPARE_FILES("out/LeftAlign_out.GSvar", TESTDATA("data_out/LeftAlign_out.GSvar"));
	}

	void leftAlign_nothing_to_do()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		VariantList vl;
		vl.load(TESTDATA("data_in/LeftAlign_in2.GSvar"));
		vl.checkValid();
		vl.leftAlign(ref_file, false);
		vl.store("out/LeftAlign_out2.GSvar");
		COMPARE_FILES("out/LeftAlign_out2.GSvar", TESTDATA("data_out/LeftAlign_out2.GSvar"));
	}

	void removeDuplicates_TSV()
	{
		VariantList vl,vl2;
		vl.load(TESTDATA("data_in/variantList_removeDuplicates_in.tsv"));
		vl.checkValid();
        vl.removeDuplicates(false);
		vl2.load(TESTDATA("data_out/variantList_removeDuplicates_out.tsv"));
		vl2.checkValid();
		vl2.sort();
		//after removal of duplicates vl and vl2 should be the same
		I_EQUAL(vl.count(),vl2.count());
		for (int i=0; i<vl2.count(); ++i)
		{
			S_EQUAL(vl[i].start(),vl2[i].start());
			S_EQUAL(vl[i].obs() ,vl2[i].obs());
		}
	}

	//check that it works with empty variant lists
	void removeDuplicates_Empty()
	{
		VariantList vl;
		vl.removeDuplicates(true);
	}

	void loadFromTSV()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel_vep.GSvar"));
		vl.checkValid();
		I_EQUAL(vl.count(), 329);
		I_EQUAL(vl.annotations().count(), 30);
		S_EQUAL(vl.annotations()[0].name(), QString("NA12878_03"));
		S_EQUAL(vl.annotations()[27].name(), QString("validation"));
		I_EQUAL(vl.filters().count(), 2);
		S_EQUAL(vl.filters()["gene_blacklist"], QString("The gene(s) are contained on the blacklist of unreliable genes."));
		S_EQUAL(vl.filters()["off-target"], QString("Variant marked as 'off-target'."));

		X_EQUAL(vl[0].chr(), Chromosome("chr1"));
		I_EQUAL(vl[0].start(), 27682481);
		I_EQUAL(vl[0].end(), 27682481);
		S_EQUAL(vl[0].ref(), Sequence("G"));
		S_EQUAL(vl[0].obs(), Sequence("A"));
		S_EQUAL(vl[0].annotations().at(0), QByteArray("het"));
		S_EQUAL(vl[0].annotations().at(7), QByteArray("rs12569127"));
		S_EQUAL(vl[0].annotations().at(9), QByteArray("0.2659"));
		I_EQUAL(vl[0].filters().count(), 1);

		X_EQUAL(vl[328].chr(), Chromosome("chr20"));
		I_EQUAL(vl[328].start(), 48301146);
		I_EQUAL(vl[328].end(), 48301146);
		S_EQUAL(vl[328].ref(), Sequence("G"));
		S_EQUAL(vl[328].obs(), Sequence("A"));
		S_EQUAL(vl[328].annotations().at(0), QByteArray("hom"));
		S_EQUAL(vl[328].annotations().at(7), QByteArray("rs6512586"));
		S_EQUAL(vl[328].annotations().at(9), QByteArray("0.5178"));
		I_EQUAL(vl[328].filters().count(), 0);

		//load a second time to check initialization
		vl.load(TESTDATA("data_in/panel_vep.GSvar"));
		vl.checkValid();
		I_EQUAL(vl.count(), 329);
		I_EQUAL(vl.annotations().count(), 30);
	}

	void loadFromTSV_withROI()
	{
		BedFile roi;
		roi.append(BedLine("chr16", 89805260, 89805978));
		roi.append(BedLine("chr19", 17379550, 17382510));

		VariantList vl;
		vl.load(TESTDATA("data_in/panel_vep.GSvar"), roi);
		I_EQUAL(vl.count(), 4);
		I_EQUAL(vl.annotations().count(), 30);
		S_EQUAL(vl.annotations()[0].name(), QString("NA12878_03"));
		S_EQUAL(vl.annotations()[27].name(), QString("validation"));
		I_EQUAL(vl.filters().count(), 2);
		S_EQUAL(vl.filters()["gene_blacklist"], QString("The gene(s) are contained on the blacklist of unreliable genes."));
		S_EQUAL(vl.filters()["off-target"], QString("Variant marked as 'off-target'."));

		X_EQUAL(vl[0].chr(), Chromosome("chr16"));
		I_EQUAL(vl[0].start(), 89805261);
		X_EQUAL(vl[1].chr(), Chromosome("chr16"));
		I_EQUAL(vl[1].start(), 89805977);
		X_EQUAL(vl[2].chr(), Chromosome("chr19"));
		I_EQUAL(vl[2].start(), 17379558);
		X_EQUAL(vl[3].chr(), Chromosome("chr19"));
		I_EQUAL(vl[3].start(), 17382505);
	}

	void storeToTSV()
	{
		//store loaded tsv file
		VariantList vl;
		vl.load(TESTDATA("data_in/panel_vep.GSvar"));
		vl.checkValid();
		vl.store("out/VariantList_store_01.tsv");
		vl.clear();

		//reload and check that everything stayed the same
		vl.load("out/VariantList_store_01.tsv");
		vl.checkValid();
		I_EQUAL(vl.count(), 329);

		I_EQUAL(vl.annotations().count(), 30);
		S_EQUAL(vl.annotations()[0].name(), QString("NA12878_03"));
		S_EQUAL(vl.annotations()[27].name(), QString("validation"));

		I_EQUAL(vl.filters().count(), 2);
		S_EQUAL(vl.filters()["gene_blacklist"], QString("The gene(s) are contained on the blacklist of unreliable genes."));
		S_EQUAL(vl.filters()["off-target"], QString("Variant marked as 'off-target'."));

		X_EQUAL(vl[0].chr(), Chromosome("chr1"));
		I_EQUAL(vl[0].start(), 27682481);
		I_EQUAL(vl[0].end(), 27682481);
		S_EQUAL(vl[0].ref(), Sequence("G"));
		S_EQUAL(vl[0].obs(), Sequence("A"));
		S_EQUAL(vl[0].annotations().at(0), QByteArray("het"));
		S_EQUAL(vl[0].annotations().at(7), QByteArray("rs12569127"));
		S_EQUAL(vl[0].annotations().at(9), QByteArray("0.2659"));

		X_EQUAL(vl[328].chr(), Chromosome("chr20"));
		I_EQUAL(vl[328].start(), 48301146);
		I_EQUAL(vl[328].end(), 48301146);
		S_EQUAL(vl[328].ref(), Sequence("G"));
		S_EQUAL(vl[328].obs(), Sequence("A"));
		S_EQUAL(vl[328].annotations().at(0), QByteArray("hom"));
		S_EQUAL(vl[328].annotations().at(7), QByteArray("rs6512586"));
		S_EQUAL(vl[328].annotations().at(9), QByteArray("0.5178"));

		//load a second time to check initialization
		vl.load(TESTDATA("data_in/panel_vep.GSvar"));
		vl.checkValid();
		I_EQUAL(vl.count(), 329);
		I_EQUAL(vl.annotations().count(), 30);
	}

	void annotationIndexByName()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel_vep.GSvar"));
		vl.checkValid();
		I_EQUAL(vl.annotationIndexByName("NA12878_03", true, false), 0);
		I_EQUAL(vl.annotationIndexByName("NA12878_03", false, false), 0);
		I_EQUAL(vl.annotationIndexByName("validation", true, false), 27);
		I_EQUAL(vl.annotationIndexByName("validation", false, false), 27);
		I_EQUAL(vl.annotationIndexByName("ESP_", false, false), 12);
		I_EQUAL(vl.annotationIndexByName("fathmm-", false, false), 16);
	}

	//test sort function for TSV files
	void sort2()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/sort_in.tsv"));
		vl.checkValid();
		vl.sort();
		vl.store("out/sort_out.tsv");
		COMPARE_FILES("out/sort_out.tsv",TESTDATA("data_out/sort_out.tsv"));
	}

	//test sortByFile function for *.tsv-files
	void sortByFile2()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/sort_in.tsv"));
		vl.checkValid();
		vl.sortByFile(TESTDATA("data_in/variantList_sortbyFile.fai"));
		vl.store("out/sortByFile_out.tsv");
		COMPARE_FILES("out/sortByFile_out.tsv",TESTDATA("data_out/sortByFile_out.tsv"));
	}

	void removeAnnotation()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel_vep.GSvar"));
		vl.checkValid();
		int index = vl.annotationIndexByName("1000g", true, false);

		I_EQUAL(vl.annotations().count(), 30);
		I_EQUAL(vl.count(), 329);
		I_EQUAL(vl[0].annotations().count(), 30);
		S_EQUAL(vl[0].annotations()[index-1], QByteArray("rs12569127"));
		S_EQUAL(vl[0].annotations()[index], QByteArray("0.1903"));
		S_EQUAL(vl[0].annotations()[index+1], QByteArray("0.2659"));

		vl.removeAnnotation(index);

		I_EQUAL(vl.annotations().count(), 29);
		I_EQUAL(vl.count(), 329);
		I_EQUAL(vl[0].annotations().count(), 29);
		S_EQUAL(vl[0].annotations()[index-1], QByteArray("rs12569127"));
		S_EQUAL(vl[0].annotations()[index], QByteArray("0.2659"));
		S_EQUAL(vl[0].annotations()[index+1], QByteArray(""));
	}

	//bug (number of variants was used to checked if index is out of range)
	void removeAnnotation_bug()
	{
		VariantList vl;
		vl.annotationDescriptions().append(VariantAnnotationDescription("bla", "some desciption"));
		vl.annotations().append(VariantAnnotationHeader("bla"));

		vl.removeAnnotation(0);

		I_EQUAL(vl.annotations().count(), 0)
	}

	void copyMetaData()
	{
		VariantList vl;
		vl.annotationDescriptions().append(VariantAnnotationDescription("bla", "some desciption"));
		vl.annotations().append(VariantAnnotationHeader("bla"));
		vl.filters().insert("MAF", "Minor allele frequency filter");
		vl.addCommentLine("Comment1");
		vl.append(Variant(Chromosome("chr1"), 1, 2, "A", "C"));

		//copy meta data
		VariantList vl2;
		vl2.copyMetaData(vl);

		//check meta data
		I_EQUAL(vl2.annotationDescriptions().count(), 1);
		I_EQUAL(vl2.annotations().count(), 1);
		I_EQUAL(vl2.filters().count(), 1);
		I_EQUAL(vl2.comments().count(), 1);

		//check no variants
		I_EQUAL(vl2.count(), 0);
	}

	void addAnnotation()
	{
		VariantList vl;
		vl.append(Variant(Chromosome("chr1"), 1, 2, "A", "C"));
		vl.append(Variant(Chromosome("chr2"), 1, 2, "A", "C"));

		int index = vl.addAnnotation("name", "desc", "default");

		I_EQUAL(index, 0);
		I_EQUAL(vl.annotations().count(), 1);
		S_EQUAL(vl.annotationDescriptionByName("name", true).description(), "desc");
		I_EQUAL(vl[0].annotations().count(), 1);
		S_EQUAL(vl[0].annotations()[index], "default");
		I_EQUAL(vl[1].annotations().count(), 1);
		S_EQUAL(vl[1].annotations()[index], "default");
	}

	void addAnnotationIfMissing()
	{
		VariantList vl;
		vl.append(Variant(Chromosome("chr1"), 1, 2, "A", "C"));
		vl.append(Variant(Chromosome("chr2"), 1, 2, "A", "C"));

		I_EQUAL(vl.addAnnotation("name", "desc", "default"), 0);
		S_EQUAL(vl.annotationDescriptionByName("name").description(), "desc");
		I_EQUAL(vl.addAnnotationIfMissing("name", "desc_new", "default"), 0);
		S_EQUAL(vl.annotationDescriptionByName("name").description(), "desc_new");
		I_EQUAL(vl.addAnnotationIfMissing("name2", "desc2", "default2"), 1);
		S_EQUAL(vl.annotationDescriptionByName("name2").description(), "desc2");
		I_EQUAL(vl.addAnnotationIfMissing("name2", "desc_new2", "default2"), 1);
		S_EQUAL(vl.annotationDescriptionByName("name2").description(), "desc_new2");
	}

	void removeAnnotationByName()
	{
		VariantList vl;
		vl.append(Variant(Chromosome("chr1"), 1, 2, "A", "C"));
		vl.append(Variant(Chromosome("chr2"), 1, 2, "A", "C"));
		vl.addAnnotation("name", "desc", "default");

		vl.removeAnnotationByName("name", true, true);

		I_EQUAL(vl.annotations().count(), 0);
		I_EQUAL(vl[0].annotations().count(), 0);
		I_EQUAL(vl[1].annotations().count(), 0);
	}

	void getSampleHeader_singlesample()
	{
		QString input = TESTDATA("data_in/panel_vep.GSvar");
		VariantList vl;
		vl.load(input);

		SampleHeaderInfo info = vl.getSampleHeader();
		I_EQUAL(info.count(), 1);
		I_EQUAL(info.sampleColumns(true).count(), 1);
		I_EQUAL(info.sampleColumns(true)[0], 0);
		I_EQUAL(info.sampleColumns(false).count(), 0);

		SampleInfo entry = info.infoByStatus(true);
		S_EQUAL(entry.id, "NA12878_03");
		S_EQUAL(entry.column_name, "NA12878_03");
		I_EQUAL(entry.column_index, 0);

		S_EQUAL(vl.analysisName(), "single-sample analysis NA12878_03");
	}

	void getSampleHeader_multisample()
	{
		QString input = TESTDATA("data_in/VariantFilter_in_multi.GSvar");
		VariantList vl;
		vl.load(input);

		SampleHeaderInfo info = vl.getSampleHeader();
		I_EQUAL(info.count(), 4);
		I_EQUAL(info.sampleColumns(true).count(), 2);
		I_EQUAL(info.sampleColumns(true)[0], 0);
		I_EQUAL(info.sampleColumns(true)[1], 3);
		I_EQUAL(info.sampleColumns(false).count(), 2);
		I_EQUAL(info.sampleColumns(false)[0], 1);
		I_EQUAL(info.sampleColumns(false)[1], 2);

		S_EQUAL(vl.analysisName(), "multi-sample analysis Affected1/Affected2/Control1/Control2");
	}

	void getPipeline()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel_vep.GSvar"));
		S_EQUAL(vl.getPipeline(), "megSAP 0.1-742-ged8ba02");

		//header not set
		vl.load(TESTDATA("data_in/VariantFilter_in_multi.GSvar"));
		S_EQUAL(vl.getPipeline(), "n/a");
	}

	void getCreationDate()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel_vep.GSvar"));
		IS_TRUE(vl.getCreationDate().isValid());
		S_EQUAL(vl.getCreationDate().toString("yyyy-MM-dd"), "2020-08-15");

		//header not set
		vl.load(TESTDATA("data_in/VariantFilter_in_multi.GSvar"));
		IS_FALSE(vl.getCreationDate().isValid());
	}
};
