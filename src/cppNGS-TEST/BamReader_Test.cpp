#include "TestFramework.h"
#include "BamReader.h"
#include "BasicStatistics.h"
#include "Settings.h"

int countSequencesContaining(QList<Sequence> sequences, char c)
{
	int output = 0;
	foreach(const Sequence& sequence, sequences)
	{
		output += sequence.contains(c);
	}
	return output;
}

TEST_CLASS(BamReader_Test)
{
private:

	/************************************************************* BamAlignment *************************************************************/

	TEST_METHOD(BamAlignment_getter_tests)
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		BamAlignment al;
		do
		{
			reader.getNextAlignment(al);
		}
		while(al.isUnmapped());

		//check name
		S_EQUAL(al.name(), "PC0226:55:000000000-A5CV9:1:1110:16414:21559");

		//check bases
		QByteArray bases = al.bases();
		S_EQUAL(bases, "CACTTCAGCCTGGGTGACAGAGCCAGACCATGTCACAAAAAGTTAGAAAAAAAAAAGAGAGAGGGAGAGAGACTATACACAGGCACCACCACATTTGGCTAATTTTTAAATATTCTGTAGAGACAAGGTCTTGCTAGGTTGCCCAGGCTAG");
        for (int i=0; i<bases.size(); ++i)
		{
			S_EQUAL(bases.data()[i], al.base(i));
		}

		//check qualities
		QByteArray qualities = al.qualities();
		S_EQUAL(qualities, "BBBBBFFFFFFFGEGEGGGGGGHGHGHFHHGHHHFHHHHHHGHHFHHHHHHGGCEGGFHHGEFGGGGGGGGGGFHHGFHHHHGGEHGHH/FFHHHGHFHHFFFHHHHHGHHFBHFFHHGEFHFHHGHHHFHHFDFEHGHG0CGHFHFHHFF");
        for (int i=0; i<qualities.size(); ++i)
		{
			S_EQUAL(qualities.data()[i], (char)(al.quality(i)+33));
		}

		//check CIGAR
		S_EQUAL(al.cigarDataAsString(), "151M");
		QByteArray cigar_exp = al.cigarDataAsString(true);
		S_EQUAL(cigar_exp, "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");
		QList<CigarOp> cigar_data = al.cigarData();
		int i = 0;
		foreach(const CigarOp& op, cigar_data)
		{
			for(int j=0; j<op.Length; ++j)
			{
				S_EQUAL(cigar_exp.data()[i], op.typeAsChar());
				++i;
			}
		}

		//tag
		S_EQUAL(al.tag("RG"), "Zpanel_realigned");
		S_EQUAL(al.tag("XX"), "");
	}

	TEST_METHOD(BamAlignment_setCigarData)
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		BamAlignment al;
		do
		{
			reader.getNextAlignment(al);
		}
		while(al.isUnmapped());

		//store other data in memory block
		QByteArray bases = al.bases();
		QByteArray qualities = al.qualities();

		//set
		QList<CigarOp> cigar_new;
		cigar_new.append(CigarOp{BAM_CMATCH, 133});
		cigar_new.append(CigarOp{BAM_CSOFT_CLIP, 18});
		al.setCigarData(cigar_new);

		//check
		S_EQUAL(al.bases(), bases);
		S_EQUAL(al.qualities(), qualities);
		S_EQUAL(al.cigarDataAsString(), "133M18S");
	}

	TEST_METHOD(BamAlignment_setBases)
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		BamAlignment al;
		do
		{
			reader.getNextAlignment(al);
		}
		while(al.isUnmapped());

		//store other data in memory block
		QByteArray cigar = al.cigarDataAsString();
		QByteArray qualities = al.qualities();

		//set
		QByteArray bases_new = "ACGTAAAACCCCGGGGTTTTAAAAAAAACCCCCCCCGGGGGGGGTTTTTTTTACGTAAAACCCCGGGGTTTTAAAAAAAACCCCCCCCGGGGGGGGTTTTTTTTACGTNANANCNCNGGNTNTNANAAAAAACCCCCCCCGGGGGGacgtn";
		al.setBases(bases_new);

		//check
		S_EQUAL(al.qualities(), qualities);
		S_EQUAL(al.cigarDataAsString(), cigar);
		S_EQUAL(al.bases(), bases_new.toUpper());
	}

	TEST_METHOD(BamAlignment_setQualities)
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		BamAlignment al;
		do
		{
			reader.getNextAlignment(al);
		}
		while(al.isUnmapped());

		//store other data in memory block
		QByteArray bases = al.bases();
		QByteArray cigar = al.cigarDataAsString();

		//set
		QByteArray qualities_new = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ";
		al.setQualities(qualities_new);

		//check
		S_EQUAL(al.qualities(), qualities_new);
		S_EQUAL(al.cigarDataAsString(), cigar);
		S_EQUAL(al.bases(), bases);
	}


	TEST_METHOD(BamAlignment_addTag)
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		BamAlignment al;
		do
		{
			reader.getNextAlignment(al);
		}
		while(al.isUnmapped());

		//set
		al.addTag("XX", 'Z', "BLA1234");

		//check
		S_EQUAL(al.tag("XX"), "ZBLA1234");
	}


	TEST_METHOD(BamAlignment_skippingData)
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		BamAlignment al;
		reader.getNextAlignment(al);
		IS_TRUE(al.containsBases());
		IS_TRUE(al.containsQualities());
		IS_TRUE(al.containsTags());

		reader.skipBases();
		reader.getNextAlignment(al);
		IS_FALSE(al.containsBases());
		IS_TRUE(al.containsQualities());
		IS_TRUE(al.containsTags());

		reader.skipQualities();
		reader.getNextAlignment(al);
		IS_FALSE(al.containsBases());
		IS_FALSE(al.containsQualities());
		IS_TRUE(al.containsTags());

		reader.skipTags();
		reader.getNextAlignment(al);
		IS_FALSE(al.containsBases());
		IS_FALSE(al.containsQualities());
		IS_FALSE(al.containsTags());

		reader.readBases();
		reader.getNextAlignment(al);
		IS_TRUE(al.containsBases());
		IS_FALSE(al.containsQualities());
		IS_FALSE(al.containsTags());

		reader.readQualities();
		reader.getNextAlignment(al);
		IS_TRUE(al.containsBases());
		IS_TRUE(al.containsQualities());
		IS_FALSE(al.containsTags());

		reader.readTags();
		reader.getNextAlignment(al);
		IS_TRUE(al.containsBases());
		IS_TRUE(al.containsQualities());
		IS_TRUE(al.containsTags());
	}

/************************************************************* BamReader *************************************************************/

	TEST_METHOD(BamReader_build)
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		I_EQUAL(reader.build(), GenomeBuild::HG38);

		BamReader reader2(TESTDATA("data_in/BamReader_insert_only.bam"));
		I_EQUAL(reader2.build(),  GenomeBuild::HG19);
	}

	TEST_METHOD(BamReader_cigarDataAsString)
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		BamAlignment al;
		do
		{
			reader.getNextAlignment(al);
		}
		while(al.isUnmapped());

		S_EQUAL(al.cigarDataAsString(), "151M");
		S_EQUAL(al.cigarDataAsString(true), "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");

		reader.getNextAlignment(al);
		while(al.isUnmapped())
		{
			reader.getNextAlignment(al);
		}
		S_EQUAL(al.cigarDataAsString(), "151M");
		S_EQUAL(al.cigarDataAsString(true), "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");
	}

	TEST_METHOD(BamReader_getPileup)
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		Pileup pileup;
		//SNP
		pileup = reader.getPileup("chr1", 12002148, 1);
		I_EQUAL(pileup.depth(false), 117);
		F_EQUAL2(pileup.frequency('A', 'G'), 0.410, 0.001);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = reader.getPileup("chr1", 12002124, 1);
		I_EQUAL(pileup.depth(false), 167);
		F_EQUAL2(pileup.frequency('A', 'G'), 0.0, 0.001);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = reader.getPileup("1", 12002124, 1);
		I_EQUAL(pileup.depth(false), 167);
		F_EQUAL2(pileup.frequency('G', 'A'), 1.0, 0.001);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = reader.getPileup("1", 12002123, 1);
		I_EQUAL(pileup.depth(false), 167);
		IS_TRUE(!BasicStatistics::isValidFloat(pileup.frequency('A', 'T')));
		I_EQUAL(pileup.indels().count(), 0);
		//SNP not properly paired
		pileup = reader.getPileup("1", 12001405, 1, 1, true);
		I_EQUAL(pileup.depth(false), 292);
		F_EQUAL2(pileup.frequency('G', 'T'), 0.0069, 0.0001);
		I_EQUAL(pileup.indels().count(), 0);
		//INSERTATION
		pileup = reader.getPileup("chr6", 109732622, 1);
		I_EQUAL(pileup.depth(false), 40);
		I_EQUAL(pileup.t(), 40);
		I_EQUAL(pileup.indels().count(), 27);
		I_EQUAL(countSequencesContaining(pileup.indels(), '+'), 25);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 2);
		//DELETION
		pileup = reader.getPileup("chr14", 53046761, 1);
		I_EQUAL(pileup.depth(false), 52);
		I_EQUAL(pileup.a(), 52);
		I_EQUAL(pileup.indels().count(), 14);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 14);
		//INSERTATION -  with window
		pileup = reader.getPileup("chr6", 109732622, 20);
		I_EQUAL(pileup.depth(false), 40);
		I_EQUAL(pileup.t(), 40);
		I_EQUAL(pileup.indels().count(), 30);
		I_EQUAL(countSequencesContaining(pileup.indels(), '+'), 28);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 2);
		//DELETION -  with window
		pileup = reader.getPileup("chr14", 53046761, 10);
		I_EQUAL(pileup.depth(false), 52);
		I_EQUAL(pileup.a(), 52);
		I_EQUAL(pileup.indels().count(), 14);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 14);
	}

	//special test with RNA because it contains the CIGAR operations S and N
	TEST_METHOD(BamReader_getPileup_RNA)
	{
		BamReader reader(TESTDATA("data_in/BamReader_rna.bam"));
		Pileup pileup;
		//SNP
		pileup = reader.getPileup("chr10", 90974727);
		I_EQUAL(pileup.depth(true), 132);
		F_EQUAL2(pileup.frequency('A', 'C'), 0.4621, 0.001);
		I_EQUAL(pileup.indels().count(), 0);
		//DELETION
		pileup = reader.getPileup("chr10", 92675287, 10);
		I_EQUAL(pileup.depth(true), 23);
		I_EQUAL(pileup.indels().count(), 23);
		I_EQUAL(countSequencesContaining(pileup.indels(), '+'), 0);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 23);
		//NO COVERAGE
		pileup = reader.getPileup("chr11", 92675295, 10);
		I_EQUAL(pileup.depth(true), 0);
		I_EQUAL(pileup.indels().count(), 0);
	}

	//special test with CIGAR strings that are
	TEST_METHOD(BamReader_getPileup_insert_only)
	{
		BamReader reader(TESTDATA("data_in/BamReader_insert_only.bam"));
		Pileup pileup;

		//SNP
		pileup = reader.getPileup("chr19", 5787214);
		I_EQUAL(pileup.depth(true), 111);
		F_EQUAL2(pileup.frequency('T', 'C'), 0.556, 0.001);

		//SNP
		pileup = reader.getPileup("chr19", 5787215);
		I_EQUAL(pileup.depth(true), 118);
		F_EQUAL2(pileup.frequency('G', 'A'), 0.389, 0.001);
	}

	TEST_METHOD(BamReader_getVariantDetails)
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		BamReader reader(TESTDATA("data_in/panel.bam"));

		//inseration T (left)
		Variant v("chr6", 109732622, 109732622, "-", "T");
		VariantDetails output = reader.getVariantDetails(reference, v, false);
		I_EQUAL(output.depth, 42);
		F_EQUAL2(output.frequency, 0.381, 0.001);

		//inseration T (right)
		v = Variant("chr16", 89510486, 89510486, "-", "T");
		output = reader.getVariantDetails(reference, v, false);
		I_EQUAL(output.depth, 114);
		F_EQUAL2(output.frequency, 0.132, 0.001);

		//deletion AG
		v = Variant("chr14", 53046761, 53046761, "AG", "-");
		output = reader.getVariantDetails(reference, v, false);
		I_EQUAL(output.depth, 66);
		F_EQUAL2(output.frequency, 0.212, 0.001);

		//SNP A>G (het)
		v = Variant("chr4", 107947255, 107947255, "A", "G");
		output = reader.getVariantDetails(reference, v, false);
		I_EQUAL(output.depth, 80);
		F_EQUAL2(output.frequency, 0.325, 0.001);

		//SNP C>T (hom)
		v = Variant("chr2", 201760892, 201760892, "C", "T");
		output = reader.getVariantDetails(reference, v, false);
		I_EQUAL(output.depth, 166);
		F_EQUAL2(output.frequency, 1.0, 0.001);

		//test on long-read data
		BamReader reader2(TESTDATA("data_in/BamReader_lr.bam"));
		v = Variant("chr17", 43092418, 43092418, "T", "C");
		output = reader2.getVariantDetails(reference, v, true);
		I_EQUAL(output.depth, 36);
		F_EQUAL2(output.frequency, 0.528, 0.001);
	}


	TEST_METHOD(BamReader_getIndels)
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		BamReader reader(TESTDATA("data_in/panel.bam"));
		QVector<Sequence> indels;
		int depth;
		double mapq0_frac;

		//inseration of TT
		reader.getIndels(reference, "chr6", 109732622-20, 109732622+20, indels, depth, mapq0_frac, false);
		I_EQUAL(depth, 42);
		I_EQUAL(indels.count(), 30);
		I_EQUAL(indels.count("+TT"), 10);
		I_EQUAL(indels.count("+T"), 18);
		I_EQUAL(indels.count("-T"), 2);
		F_EQUAL2(mapq0_frac, 0.0, 0.001);

		//deletion of AG
		reader.getIndels(reference, "chr14", 53046761-10, 53046762+10, indels, depth, mapq0_frac, false);
		I_EQUAL(depth, 64);
		I_EQUAL(indels.count(), 14);
		I_EQUAL(indels.count("-AG"), 14);
		F_EQUAL2(mapq0_frac, 0.0, 0.001);

		//depth calculation on spliced reads in RNA samples
		BamReader reader2(TESTDATA("data_in/rna.bam"));
		reader2.getIndels(reference, "chr1", 998764-10, 998764+10, indels, depth, mapq0_frac, false);
		I_EQUAL(depth, 2);

		reader2.getIndels(reference, "chr1", 2401387-10, 2401392+10, indels, depth, mapq0_frac, false);
		I_EQUAL(depth, 0);

		reader2.getIndels(reference, "chr1", 10460908-10, 10460909+10, indels, depth, mapq0_frac, false);
		I_EQUAL(depth, 27);


		//long-read data
		BamReader reader3(TESTDATA("data_in/BamReader_lr.bam"));
		reader3.getIndels(reference, "chr17", 43092000-10, 43092000+10, indels, depth, mapq0_frac, true);
		I_EQUAL(depth, 38);
		I_EQUAL(indels.count(), 21);
		I_EQUAL(indels.count("-A"), 11);
	}

	TEST_METHOD(BamReader_genomeSize)
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		double size_without_special = reader.genomeSize(false);
		double size_with_special = reader.genomeSize(true);
		IS_TRUE(size_without_special < size_with_special);
	}

/************************************************************* Cram Support *************************************************************/

	TEST_METHOD(CramSupport_referenceAsParameter_tests)
	{
		QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		BamReader reader(TESTDATA("data_in/cramTest.cram"), ref_file);

		BamAlignment al;
		do
		{
			reader.getNextAlignment(al);
		}
		while(al.isUnmapped());


		//check name
		S_EQUAL(al.name(), "PC0226:121:000000000-AB2J9:1:2101:19474:26718");

		//check bases
		QByteArray bases = al.bases();
		S_EQUAL(bases, "TGCTGGGATTACAGGTGTGAGCCACCGCGCCCGGCGTTTTGTTTCATTTTTATTTTTGAGACACGGTCTTGCTCTGTCGCCCAGGCTGGAGTGCAGTGTCGCAATCTCGGCTCACTGCATCCTCCGCCTC");
        for (int i=0; i<bases.size(); ++i)
		{
			S_EQUAL(bases.data()[i], al.base(i));
		}

		//check qualities
		QByteArray qualities = al.qualities();
		S_EQUAL(qualities, "3>AABF@FFFFFGGGGGGGGGFHHHFGGGCGGGGEEGGGGHCGHHHHHHHHGHHHGHGFGHHHHGGGGGGHHHHHHHHGFGGGGGHHFEHFHGHHHHHHHGHGGGHHGGFGGGHHHFHHHHHHHHGGFGG");
        for (int i=0; i<qualities.size(); ++i)
		{
			S_EQUAL(qualities.data()[i], (char)(al.quality(i)+33));
		}

		//check CIGAR
		S_EQUAL(al.cigarDataAsString(), "130M");
		QByteArray cigar_exp = al.cigarDataAsString(true);
		S_EQUAL(cigar_exp, "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");
		QList<CigarOp> cigar_data = al.cigarData();
		int i = 0;
		foreach(const CigarOp& op, cigar_data)
		{
			for(int j=0; j<op.Length; ++j)
			{
				S_EQUAL(cigar_exp.data()[i], op.typeAsChar());
				++i;
			}
		}

		//tag
		S_EQUAL(al.tag("MC"), "Z130M");
		S_EQUAL(al.tag("RG"), "");
	}

	TEST_METHOD(CramSupport_cigarDataAsString)
	{
		QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		BamReader reader(TESTDATA("data_in/cramTest.cram"), ref_file);
		BamAlignment al;

		do
		{
			reader.getNextAlignment(al);
		}
		while(al.isUnmapped());

		S_EQUAL(al.cigarDataAsString(), "130M");
		S_EQUAL(al.cigarDataAsString(true), "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");

		reader.getNextAlignment(al);
		while(al.isUnmapped())
		{
			reader.getNextAlignment(al);
		}
		S_EQUAL(al.cigarDataAsString(), "130M");
		S_EQUAL(al.cigarDataAsString(true), "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");

		while(reader.getNextAlignment(al))
		{
				//qDebug() << al.chromosomeID() << al.start() << al.end();
		}
		//qDebug() << "done";
		S_EQUAL(al.cigarDataAsString(), "19S139M");

	}

	TEST_METHOD(CramSupport_getPileup)
	{
		QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

		BamReader reader(TESTDATA("data_in/cramTest.cram"), ref_file);

		Pileup pileup;
		//SNP
		pileup = reader.getPileup("chr1", 27355990, 1);
		I_EQUAL(pileup.depth(false), 169);
		F_EQUAL2(pileup.frequency('G', 'A'), 0.508876, 0.491124);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = reader.getPileup("chr1", 27359572, 1);
		I_EQUAL(pileup.depth(false), 175);
		F_EQUAL2(pileup.frequency('G', 'C'), 1, 0);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = reader.getPileup("1", 27360975, 1);
		I_EQUAL(pileup.depth(false), 736);
		F_EQUAL2(pileup.frequency('G', 'T'), 0.516304, 0.483696);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = reader.getPileup("1", 27363868, 1);
		I_EQUAL(pileup.depth(false), 111);
		IS_TRUE(!BasicStatistics::isValidFloat(pileup.frequency('A', 'G')));
		I_EQUAL(pileup.indels().count(), 0);
		//INSERTATION
		pileup = reader.getPileup("chr3", 10052522, 1);
		I_EQUAL(pileup.depth(false), 25);
		I_EQUAL(pileup.t(), 0);
		I_EQUAL(pileup.indels().count(), 14);
		I_EQUAL(countSequencesContaining(pileup.indels(), '+'), 10);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 4);
		//DELATION
		pileup = reader.getPileup("chr2", 47806751, 1);
        I_EQUAL(pileup.depth(false), 32);
		I_EQUAL(pileup.a(), 0);
		I_EQUAL(pileup.indels().count(), 26);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 14);
		//INSERTATION -  with window
		pileup = reader.getPileup("chr6", 130827751, 3);
		I_EQUAL(pileup.depth(false), 703);
		I_EQUAL(pileup.t(), 0);
		I_EQUAL(pileup.indels().count(), 325);
		I_EQUAL(countSequencesContaining(pileup.indels(), '+'), 298);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 27);
		//DELETION -  with window
		pileup = reader.getPileup("chr5", 80864777, 4);
		I_EQUAL(pileup.depth(false), 16);
		I_EQUAL(pileup.a(), 16);
		I_EQUAL(pileup.indels().count(), 6);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 6);
	}

    TEST_METHOD(info_bam)
    {
        //BAM - long read DNA, HG38, no ALT
        {
            BamReader reader(TESTDATA("data_in/BamReader_lr.bam"));
            BamInfo info = reader.info();
            S_EQUAL(info.file_format, "BAM");
            S_EQUAL(info.build, "hg38");
            IS_FALSE(info.paired_end);
            S_EQUAL(info.mapper, "minimap2");
            S_EQUAL(info.mapper_version, "2.26-r1175");
            IS_TRUE(info.false_duplications_masked);
            IS_FALSE(info.contains_alt_chrs);
        }

        //BAM - short-read RNA, HG19, with ALT
        {
            BamReader reader(TESTDATA("/data_in/BamReader_rna.bam"));
            BamInfo info = reader.info();
            S_EQUAL(info.file_format, "BAM");
            S_EQUAL(info.build, "hg19");
            IS_TRUE(info.paired_end);
            S_EQUAL(info.mapper, "STAR");
            S_EQUAL(info.mapper_version, "2.3.0e_r291");
            IS_TRUE(info.false_duplications_masked);
            IS_TRUE(info.contains_alt_chrs);
        }
    }

    TEST_METHOD(info_cram)
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

        //CRAM - short read DNA, HG38, no ALT
        BamReader reader(TESTDATA("data_in/cramTest.cram"));
        BamInfo info = reader.info();
        S_EQUAL(info.file_format, "CRAM 3.0");
        S_EQUAL(info.build, "hg38");
        IS_TRUE(info.paired_end);
        S_EQUAL(info.mapper, "bwa");
        S_EQUAL(info.mapper_version, "0.7.17-r1188");
        IS_TRUE(info.false_duplications_masked);
        IS_FALSE(info.contains_alt_chrs);
    }

};
