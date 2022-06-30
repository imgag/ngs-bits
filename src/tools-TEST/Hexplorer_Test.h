#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(Hexplorer_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
        EXECUTE("Hexplorer", "-in " + TESTDATA("data_in/hexplorer_variants.vcf") + " -out out/hexplorer_output.vcf");
        COMPARE_FILES("out/hexplorer_output.vcf", TESTDATA("data_out/hexplorer_output.vcf"));
        VCF_IS_VALID("out/hexplorer_output.vcf");
	}

};
