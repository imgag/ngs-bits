#include "TestFramework.h"
#include "BamReader.h"


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
};
