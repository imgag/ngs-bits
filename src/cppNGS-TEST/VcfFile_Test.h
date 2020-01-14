#include "TestFramework.h"
#include "VcfFile.h"
#include "Settings.h"

TEST_CLASS(VcfFile_Test)
{
Q_OBJECT
private slots:

	void default_params()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");

		QString output;
		QTextStream out_stream(&output);
		bool is_valid = VcfFile::isValid(TESTDATA("data_in/panel_vep.vcf"), ref_file, out_stream);

		IS_TRUE(is_valid);
		I_EQUAL(output.length(), 0);
	}

	void with_info()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");

		QString output;
		QTextStream out_stream(&output);
		bool is_valid = VcfFile::isValid(TESTDATA("data_in/panel_vep.vcf"), ref_file, out_stream, true);

		IS_TRUE(is_valid);
		QStringList output_lines = output.split("\n");
		I_EQUAL(output_lines.length(), 78);
		S_EQUAL(output_lines[0], "VCF version: 4.2");
	}

	void url_encoding()
	{
		QString input_string = "Test-String= blabla%, \t; \r\n; \r";
		QString output_string = VcfFile::encodeInfoValue(input_string);
		S_EQUAL(output_string, "Test-String%3D%20blabla%25%2C%20%09%3B%20%0D%0A%3B%20%0D");
	}

	void url_decoding()
	{
		QString input_string = "Test-String%3D%20blabla%25%2C%20%09%3B%20%0D%0A%3B%20%0D";
		QString output_string = VcfFile::decodeInfoValue(input_string);
		S_EQUAL(output_string, "Test-String= blabla%, \t; \r\n; \r");
	}
};
