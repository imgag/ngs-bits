#include "TestFramework.h"

TEST_CLASS(TrioMaternalContamination_Test)
{
Q_OBJECT
private slots:

		void default_parameters()
        {
            EXECUTE("TrioMaternalContamination", "-bam_m " + TESTDATA("data_in/TrioMaternalContaminationMother.bam") + " -bam_f " + TESTDATA("data_in/TrioMaternalContaminationFather.bam") +
					" -bam_c " +TESTDATA("data_in/TrioMaternalContaminationChild10Perc.bam") + " -build hg19 -out out/TrioMaternalContamination_out1.txt");
			COMPARE_FILES("out/TrioMaternalContamination_out1.txt", TESTDATA("data_out/TrioMaternalContamination_out1.txt"));
        }

};
