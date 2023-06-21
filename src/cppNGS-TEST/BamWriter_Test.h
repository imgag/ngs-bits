#include "TestFramework.h"
#include "BamReader.h"
#include "BamWriter.h"
#include "BasicStatistics.h"
#include "Settings.h"


const QString write_bam_file()
{
	//read origional BAM file
	BamReader reader(TESTDATA("data_in/bamWriterTest.bam"));

	//write first line of origional file into new file
	BamWriter writer("out/bamWriterTest.bam");
	writer.writeHeader(reader);
	BamAlignment al;
	reader.getNextAlignment(al);
	writer.writeAlignment(al);

	return al.cigarDataAsString();
}

const QString write_cram_file(QString ref_file)
{
	//read origional BAM file
	BamReader reader(TESTDATA("data_in/bamWriterTest.bam"), ref_file);

	//write first line of origional file into new file
	BamWriter writer("out/bamWriterTest.cram", ref_file);
	writer.writeHeader(reader);
	BamAlignment al;
	reader.getNextAlignment(al);
	writer.writeAlignment(al);

	return al.cigarDataAsString();
}

TEST_CLASS(BamWriter_Test)
{
Q_OBJECT
private slots:

	void write_bam_test()
	{

		//write new Bam file from origional one and return cigar string of first alignment
		const QString al_string = write_bam_file();

		//read new BAM file
		BamReader new_reader("out/bamWriterTest.bam");
		BamAlignment new_al;
		new_reader.getNextAlignment(new_al);

		//compare old and new first alignment
		S_EQUAL(al_string, new_al.cigarDataAsString());
	}

	void write_cram_test()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		if (!ref_file.endsWith("GRCh38.fa")) SKIP("Test needs reference genome GRCh38!");

		const QString al_string = write_cram_file(ref_file);

		//read new BAM file
		BamReader new_reader("out/bamWriterTest.cram", ref_file);
		BamAlignment new_al;
		new_reader.getNextAlignment(new_al);

		//compare old and new first alignment
		IS_TRUE(new_reader.fp_->is_cram);
		S_EQUAL(al_string, new_al.cigarDataAsString());

	}
};
