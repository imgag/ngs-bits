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
Q_OBJECT
private slots:

	/************************************************************* BamAlignment *************************************************************/

	void BamAlignment_getter_tests()
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		BamAlignment al;
		do
		{
			reader.getNextAlignment(al);
		}
		while(al.isUnmapped());

		//check name
		S_EQUAL(al.name(), "PC0226:55:000000000-A5CV9:1:2101:8066:18464");

		//check bases
		QByteArray bases = al.bases();
		S_EQUAL(bases, "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGACGACGCTCTTCCGATCT");
		for (int i=0; i<bases.count(); ++i)
		{
			S_EQUAL(bases.data()[i], al.base(i));
		}

		//check qualities
		QByteArray qualities = al.qualities();
		S_EQUAL(qualities, "@@?@@@=@@@?@@@?@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@?@@@@@@@@@@@@@@@@@@@@@@@@@=@99----;--99-?@99@;G@@AA-@A-?C@B<///>/>//////0000A0GGGGGGFFAAAAAAAAAA");
		for (int i=0; i<qualities.count(); ++i)
		{
			S_EQUAL(qualities.data()[i], (char)(al.quality(i)+33));
		}

		//check CIGAR
		S_EQUAL(al.cigarDataAsString(), "133M13I5M");
		QByteArray cigar_exp = al.cigarDataAsString(true);
		S_EQUAL(cigar_exp, "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMIIIIIIIIIIIIIMMMMM");
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
		S_EQUAL(al.tag("RG"), "ZGS130639_01.000000000-A5CV9.1");
		S_EQUAL(al.tag("XX"), "");
	}

	void BamAlignment_setCigarData()
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

	void BamAlignment_setBases()
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

	void BamAlignment_setQualities()
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


	void BamAlignment_addTag()
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


/************************************************************* BamReader *************************************************************/

	void BamReader_build()
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		S_EQUAL(reader.build(), "");

		BamReader reader2(TESTDATA("data_in/BamReader_insert_only.bam"));
		S_EQUAL(reader2.build(), "GRCh37");
	}

	void BamReader_cigarDataAsString()
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		BamAlignment al;
		do
		{
			reader.getNextAlignment(al);
		}
		while(al.isUnmapped());

		S_EQUAL(al.cigarDataAsString(), "133M13I5M");
		S_EQUAL(al.cigarDataAsString(true), "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMIIIIIIIIIIIIIMMMMM");

		reader.getNextAlignment(al);
		while(al.isUnmapped())
		{
			reader.getNextAlignment(al);
		}
		S_EQUAL(al.cigarDataAsString(), "36M2D115M");
		S_EQUAL(al.cigarDataAsString(true), "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMDDMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");
	}

	void BamReader_getPileup()
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		Pileup pileup;
		//SNP
		pileup = reader.getPileup("chr1", 12062205, 1);
		I_EQUAL(pileup.depth(false), 117);
		F_EQUAL2(pileup.frequency('A', 'G'), 0.4102, 0.001);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = reader.getPileup("chr1", 12062181, 1);
		I_EQUAL(pileup.depth(false), 167);
		F_EQUAL2(pileup.frequency('A', 'G'), 0.0, 0.001);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = reader.getPileup("1", 12062181, 1);
		I_EQUAL(pileup.depth(false), 167);
		F_EQUAL2(pileup.frequency('G', 'A'), 1.0, 0.001);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = reader.getPileup("1", 12062180, 1);
		I_EQUAL(pileup.depth(false), 167);
		IS_TRUE(!BasicStatistics::isValidFloat(pileup.frequency('A', 'T')));
		I_EQUAL(pileup.indels().count(), 0);
		//INSERTATION
		pileup = reader.getPileup("chr6", 110053825, 1);
		I_EQUAL(pileup.depth(false), 40);
		I_EQUAL(pileup.t(), 40);
		I_EQUAL(pileup.indels().count(), 29);
		I_EQUAL(countSequencesContaining(pileup.indels(), '+'), 27);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 2);
		//DELATION
		pileup = reader.getPileup("chr14", 53513479, 1);
		I_EQUAL(pileup.depth(false), 50);
		I_EQUAL(pileup.a(), 50);
		I_EQUAL(pileup.indels().count(), 14);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 14);
		//INSERTATION -  with window
		pileup = reader.getPileup("chr6", 110053825, 20);
		I_EQUAL(pileup.depth(false), 40);
		I_EQUAL(pileup.t(), 40);
		I_EQUAL(pileup.indels().count(), 30);
		I_EQUAL(countSequencesContaining(pileup.indels(), '+'), 28);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 2);
		//DELETION -  with window
		pileup = reader.getPileup("chr14", 53513479, 10);
		I_EQUAL(pileup.depth(false), 50);
		I_EQUAL(pileup.a(), 50);
		I_EQUAL(pileup.indels().count(), 14);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 14);
	}

	//special test with RNA because it contains the CIGAR operations S and N
	void BamReader_getPileup_RNA()
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
	void BamReader_getPileup_insert_only()
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

	void BamReader_getVariantDetails()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		BamReader reader(TESTDATA("data_in/panel.bam"));

		//inseration T (left)
		Variant v("chr6", 110053825, 110053825, "-", "T");
		VariantDetails output = reader.getVariantDetails(reference, v);
		I_EQUAL(output.depth, 42);
		F_EQUAL2(output.frequency, 0.428, 0.001);

		//inseration T (right)
		v = Variant("chr16", 89576894, 89576894, "-", "T");
		output = reader.getVariantDetails(reference, v);
		I_EQUAL(output.depth, 126);
		F_EQUAL2(output.frequency, 0.126, 0.001);

		//deletion AG
		v = Variant("chr14", 53513479, 53513480, "AG", "-");
		output = reader.getVariantDetails(reference, v);
		I_EQUAL(output.depth, 64);
		F_EQUAL2(output.frequency, 0.218, 0.001);

		//SNP A>G (het)
		v = Variant("chr4", 108868411, 108868411, "A", "G");
		output = reader.getVariantDetails(reference, v);
		I_EQUAL(output.depth, 78);
		F_EQUAL2(output.frequency, 0.333, 0.001);

		//SNP C>T (hom)
		v = Variant("chr2", 202625615, 202625615, "C", "T");
		output = reader.getVariantDetails(reference, v);
		I_EQUAL(output.depth, 166);
		F_EQUAL2(output.frequency, 1.0, 0.001);
	}


	void BamReader_getIndels()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		BamReader reader(TESTDATA("data_in/panel.bam"));
		QVector<Sequence> indels;
		int depth;
		double mapq0_frac;

		//inseration of TT
		reader.getIndels(reference, "chr6", 110053825-20, 110053825+20, indels, depth, mapq0_frac);
		I_EQUAL(depth, 42);
		I_EQUAL(indels.count(), 30);
		I_EQUAL(indels.count("+TT"), 10);
		I_EQUAL(indels.count("+T"), 18);
		I_EQUAL(indels.count("-T"), 2);
		F_EQUAL2(mapq0_frac, 0.0, 0.001);

		//deletion of AG
		reader.getIndels(reference, "chr14", 53513479-10, 53513480+10, indels, depth, mapq0_frac);
		I_EQUAL(depth, 64);
		I_EQUAL(indels.count(), 14);
		I_EQUAL(indels.count("-AG"), 14);
		F_EQUAL2(mapq0_frac, 0.0, 0.001);
	}

	void BamReader_genomeSize()
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		double size_without_special = reader.genomeSize(false);
		double size_with_special = reader.genomeSize(true);
		IS_TRUE(size_without_special < size_with_special);
	}

/************************************************************* Cram Support *************************************************************/

	void CramSupport_referenceAsParameter_tests()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		if (!ref_file.endsWith("GRCh37.fa")) SKIP("Test needs reference genome GRCh37!");
		if (Helper::isWindows()) SKIP("CRAM is not supported on Windows!");

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
		for (int i=0; i<bases.count(); ++i)
		{
			S_EQUAL(bases.data()[i], al.base(i));
		}

		//check qualities
		QByteArray qualities = al.qualities();
		S_EQUAL(qualities, "3>AABF@FFFFFGGGGGGGGGFHHHFGGGCGGGGEEGGGGHCGHHHHHHHHGHHHGHGFGHHHHGGGGGGHHHHHHHHGFGGGGGHHFEHFHGHHHHHHHGHGGGHHGGFGGGHHHFHHHHHHHHGGFGG");
		for (int i=0; i<qualities.count(); ++i)
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
		S_EQUAL(al.tag("RG"), "ZNA12878_03");
	}

	void  CramSupport_cigarDataAsString()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		if (!ref_file.endsWith("GRCh37.fa")) SKIP("Test needs reference genome GRCh37!");
		if (Helper::isWindows()) SKIP("CRAM is not supported on Windows!");

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
		S_EQUAL(al.cigarDataAsString(), "17S141M");

	}

	void CramSupport_getPileup()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		if (!ref_file.endsWith("GRCh37.fa")) SKIP("Test needs reference genome GRCh37!");
		if (Helper::isWindows()) SKIP("CRAM is not supported on Windows!");

		BamReader reader(TESTDATA("data_in/cramTest.cram"), ref_file);

		Pileup pileup;
		//SNP
		pileup = reader.getPileup("chr1", 27682481, 1);
		I_EQUAL(pileup.depth(false), 169);
		F_EQUAL2(pileup.frequency('G', 'A'), 0.508876, 0.491124);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = reader.getPileup("chr1", 27686063, 1);
		I_EQUAL(pileup.depth(false), 198);
		F_EQUAL2(pileup.frequency('G', 'C'), 1, 0);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = reader.getPileup("1", 27687466, 1);
		I_EQUAL(pileup.depth(false), 736);
		F_EQUAL2(pileup.frequency('G', 'T'), 0.516304, 0.483696);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = reader.getPileup("1", 27690359, 1);
		I_EQUAL(pileup.depth(false), 111);
		IS_TRUE(!BasicStatistics::isValidFloat(pileup.frequency('A', 'G')));
		I_EQUAL(pileup.indels().count(), 0);
		//INSERTATION
		pileup = reader.getPileup("chr3", 10094206, 1);
		I_EQUAL(pileup.depth(false), 25);
		I_EQUAL(pileup.t(), 0);
		I_EQUAL(pileup.indels().count(), 14);
		I_EQUAL(countSequencesContaining(pileup.indels(), '+'), 10);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 4);
		//DELATION
		pileup = reader.getPileup("chr2", 48033890, 1);
		I_EQUAL(pileup.depth(false), 38);
		I_EQUAL(pileup.a(), 0);
		I_EQUAL(pileup.indels().count(), 30);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 18);
		//INSERTATION -  with window
		pileup = reader.getPileup("chr6", 131148891, 3);
		I_EQUAL(pileup.depth(false), 704);
		I_EQUAL(pileup.t(), 0);
		I_EQUAL(pileup.indels().count(), 331);
		I_EQUAL(countSequencesContaining(pileup.indels(), '+'), 304);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 27);
		//DELETION -  with window
		pileup = reader.getPileup("chr5", 80160596, 4);
		I_EQUAL(pileup.depth(false), 16);
		I_EQUAL(pileup.a(), 16);
		I_EQUAL(pileup.indels().count(), 6);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 6);
	}

};
