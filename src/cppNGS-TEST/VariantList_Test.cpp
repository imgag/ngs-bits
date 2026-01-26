#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "VariantList.h"
#include "Settings.h"

TEST_CLASS(VariantList_Test)
{
private:


	TEST_METHOD(VariantTranscript_idWithoutVersion)
	{
		VariantTranscript trans;
		trans.id = "ENST00000493901";
		S_EQUAL(trans.idWithoutVersion(), "ENST00000493901");
		trans.id = "ENST00000493901.1";
		S_EQUAL(trans.idWithoutVersion(), "ENST00000493901");
	}

	TEST_METHOD(leftAlign)
	{
		SKIP_IF_NO_HG38_GENOME();

		FastaFileIndex ref_index(Settings::string("reference_genome"));

		QString ref_file = Settings::string("reference_genome", true);
		VariantList vl;
		vl.load(TESTDATA("data_in/LeftAlign_in.GSvar"));
		vl.checkValid(ref_index);
		vl.leftAlign(ref_file);
		vl.store("out/LeftAlign_out.GSvar");
		COMPARE_FILES("out/LeftAlign_out.GSvar", TESTDATA("data_out/LeftAlign_out.GSvar"));
	}

	TEST_METHOD(leftAlign_nothing_to_do)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);
		FastaFileIndex ref_index(ref_file);

		VariantList vl;
		vl.load(TESTDATA("data_in/LeftAlign_in2.GSvar"));
		vl.checkValid(ref_index);
		vl.leftAlign(ref_file);
		vl.store("out/LeftAlign_out2.GSvar");
		COMPARE_FILES("out/LeftAlign_out2.GSvar", TESTDATA("data_out/LeftAlign_out.GSvar"));
	}

	TEST_METHOD(removeDuplicates_TSV)
	{
		VariantList vl,vl2;
		vl.load(TESTDATA("data_in/variantList_removeDuplicates_in.tsv"));
		vl.checkValid();
		vl.removeDuplicates();
		vl2.load(TESTDATA("data_out/variantList_removeDuplicates_out.tsv"));
		vl2.checkValid();
		vl2.sort();
		//after removal of duplicates vl and vl2 should be the same
		I_EQUAL(vl.count(),vl2.count());
		for (int i=0; i<vl2.count(); ++i)
		{
            I_EQUAL(vl[i].start(), vl2[i].start());
            S_EQUAL(vl[i].obs(), vl2[i].obs());
		}
	}

	//check that it works with empty variant lists
	TEST_METHOD(removeDuplicates_Empty)
	{
		VariantList vl;
		vl.removeDuplicates();
	}

	TEST_METHOD(loadFromTSV)
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

	TEST_METHOD(loadFromTSV_withROI)
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

	TEST_METHOD(loadHeaderOnly)
	{
		VariantList vl;
		vl.loadHeaderOnly(TESTDATA("data_in/panel_vep.GSvar"));
		I_EQUAL(vl.count(), 0);
		I_EQUAL(vl.annotations().count(), 30);
		S_EQUAL(vl.annotations()[0].name(), QString("NA12878_03"));
		S_EQUAL(vl.annotations()[27].name(), QString("validation"));
		I_EQUAL(vl.filters().count(), 2);
		S_EQUAL(vl.filters()["gene_blacklist"], QString("The gene(s) are contained on the blacklist of unreliable genes."));
		S_EQUAL(vl.filters()["off-target"], QString("Variant marked as 'off-target'."));
	}

	TEST_METHOD(storeToTSV)
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

	TEST_METHOD(annotationIndexByName)
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
	TEST_METHOD(sort2)
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/sort_in.tsv"));
		vl.checkValid();
		vl.sort();
		vl.store("out/sort_out.tsv");
		COMPARE_FILES("out/sort_out.tsv",TESTDATA("data_out/sort_out.tsv"));
	}

	//test sortByFile function for *.tsv-files
	TEST_METHOD(sortByFile2)
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/sort_in.tsv"));
		vl.checkValid();
		vl.sortByFile(TESTDATA("data_in/variantList_sortbyFile.fai"));
		vl.store("out/sortByFile_out.tsv");
		COMPARE_FILES("out/sortByFile_out.tsv",TESTDATA("data_out/sortByFile_out.tsv"));
	}

	TEST_METHOD(removeAnnotation)
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
	TEST_METHOD(removeAnnotation_bug)
	{
		VariantList vl;
		vl.annotationDescriptions().append(VariantAnnotationDescription("bla", "some desciption"));
		vl.annotations().append(VariantAnnotationHeader("bla"));

		vl.removeAnnotation(0);

		I_EQUAL(vl.annotations().count(), 0)
	}

	TEST_METHOD(copyMetaData)
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

	TEST_METHOD(addAnnotation)
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

	TEST_METHOD(addAnnotationIfMissing)
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

	TEST_METHOD(removeAnnotationByName)
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

	TEST_METHOD(getSampleHeader_singlesample)
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
		S_EQUAL(entry.name, "NA12878_03");
		I_EQUAL(entry.column_index, 0);

		S_EQUAL(vl.analysisName(), "single-sample analysis NA12878_03");
	}

	TEST_METHOD(getSampleHeader_multisample)
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

	TEST_METHOD(getPipeline)
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel_vep.GSvar"));
		S_EQUAL(vl.getPipeline(), "megSAP 0.1-742-ged8ba02");

		//header not set
		vl.load(TESTDATA("data_in/VariantFilter_in_multi.GSvar"));
		S_EQUAL(vl.getPipeline(), "n/a");
	}

	TEST_METHOD(getCreationDate)
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel_vep.GSvar"));
		IS_TRUE(vl.getCreationDate().isValid());
		S_EQUAL(vl.getCreationDate().toString(Qt::ISODate), "2020-08-15");

		//header not set
		vl.load(TESTDATA("data_in/VariantFilter_in_multi.GSvar"));
		IS_FALSE(vl.getCreationDate().isValid());
	}

	TEST_METHOD(getBuild)
	{
		VariantList vl;
		I_EQUAL(vl.build(), GenomeBuild::HG19);

		vl.addCommentLine("##GENOME_BUILD=GRCh38");
		I_EQUAL(vl.build(), GenomeBuild::HG38);
	}

	TEST_METHOD(getCaller)
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel_vep.GSvar"));
		S_EQUAL(vl.caller(), "freebayes");
		S_EQUAL(vl.callerVersion(), "v1.3.3");

		//header not set
		vl.load(TESTDATA("data_in/VariantFilter_in_multi.GSvar"));
		S_EQUAL(vl.caller(), "");
		S_EQUAL(vl.callerVersion(), "");
	}

	TEST_METHOD(getCallingDate)
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel_vep.GSvar"));
		IS_TRUE(vl.callingDate().isValid());
		S_EQUAL(vl.callingDate().toString(Qt::ISODate), "2022-04-25");

		//header not set
		vl.load(TESTDATA("data_in/VariantFilter_in_multi.GSvar"));
		IS_FALSE(vl.callingDate().isValid());
	}


	TEST_METHOD(constructorFromVCF)
	{
		//SNP
		Variant v(VcfLine("chr13", 32332271, "G", QList<Sequence>() << "A"));
		S_EQUAL(v.chr().str(), "chr13");
		I_EQUAL(v.start(), 32332271);
		I_EQUAL(v.end(), 32332271);
		S_EQUAL(v.ref(), "G");
		S_EQUAL(v.obs(), "A");

		//INS
		v = Variant(VcfLine("chr13", 32332271, "G", QList<Sequence>() << "GC"));
		S_EQUAL(v.chr().str(), "chr13");
		I_EQUAL(v.start(), 32332271);
		I_EQUAL(v.end(), 32332271);
		S_EQUAL(v.ref(), "-");
		S_EQUAL(v.obs(), "C");

		//DEL
		v = Variant(VcfLine("chr22", 28734461, "CTCCTCAGGTTCTTGG", QList<Sequence>() << "C"));
		S_EQUAL(v.chr().str(), "chr22");
		I_EQUAL(v.start(), 28734462);
		I_EQUAL(v.end(), 28734476);
		S_EQUAL(v.ref(), "TCCTCAGGTTCTTGG");
		S_EQUAL(v.obs(), "-");

		//COMPLEX INDEL (no prefix base)
		v = Variant(VcfLine("chr13", 32339964, "TC", QList<Sequence>() << "AG"));
		S_EQUAL(v.chr().str(), "chr13");
		I_EQUAL(v.start(), 32339964);
		I_EQUAL(v.end(), 32339965);
		S_EQUAL(v.ref(), "TC");
		S_EQUAL(v.obs(), "AG");


		//COMPLEX INDEL (with prefix base)
		v = Variant(VcfLine("chr13", 32339963, "TTC", QList<Sequence>() << "TAG"));
		S_EQUAL(v.chr().str(), "chr13");
		I_EQUAL(v.start(), 32339964);
		I_EQUAL(v.end(), 32339965);
		S_EQUAL(v.ref(), "TC");
		S_EQUAL(v.obs(), "AG");
	}


	TEST_METHOD(toVCF)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);
		FastaFileIndex ref_index(ref_file);

		//SNV
		{
			Variant v("chr13", 32332271, 32332271, "G", "A", QByteArrayList() << "het");
			VcfLine v2 = v.toVCF(ref_index);
			S_EQUAL(v2.chr().str(), "chr13");
			I_EQUAL(v2.start(), 32332271);
			S_EQUAL(v2.ref(), "G");
			S_EQUAL(v2.altString(), "A");
			I_EQUAL(v2.samples().count(), 0);
			//genotype
			v2 = v.toVCF(ref_index, 0);
			I_EQUAL(v2.samples().count(), 1);
			S_EQUAL(v2.formatValueFromSample("GT"), "0/1")
		}

		//DEL
		{
			Variant v("chr13", 32332271, 32332272, "GG", "-", QByteArrayList() << "hom");
			VcfLine v2 = v.toVCF(ref_index);
			S_EQUAL(v2.chr().str(), "chr13");
			I_EQUAL(v2.start(), 32332270);
			S_EQUAL(v2.ref(), "AGG");
			S_EQUAL(v2.altString(), "A");
			I_EQUAL(v2.samples().count(), 0);
			//genotype
			v2 = v.toVCF(ref_index, 0);
			I_EQUAL(v2.samples().count(), 1);
			S_EQUAL(v2.formatValueFromSample("GT"), "1/1")
		}

		//INS
		{
			Variant v("chr13", 32332271, 32332271, "-", "C", QByteArrayList() << "wt");
			VcfLine v2 = v.toVCF(ref_index);
			S_EQUAL(v2.chr().str(), "chr13");
			I_EQUAL(v2.start(), 32332271);
			S_EQUAL(v2.ref(), "G");
			S_EQUAL(v2.altString(), "GC");
			I_EQUAL(v2.samples().count(), 0);
			//genotype
			v2 = v.toVCF(ref_index, 0);
			I_EQUAL(v2.samples().count(), 1);
			S_EQUAL(v2.formatValueFromSample("GT"), "0/0")
		}
	}

};
