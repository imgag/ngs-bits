#include "TestFramework.h"

TEST_CLASS(TrioMaternalContamination_Test)
{
Q_OBJECT
private slots:

        void uncontaminated()
        {
            EXECUTE("TrioMaternalContamination", "-bam_m " + TESTDATA("data_in/TrioMaternalContaminationMother.bam") + " -bam_f " + TESTDATA("data_in/TrioMaternalContaminationFather.bam") +
					" -bam_c " +TESTDATA("data_in/TrioMaternalContaminationChild.bam") + " -out out/TrioMaternalContaminationUncontaminated.txt");
			COMPARE_FILES("out/TrioMaternalContaminationUncontaminated.txt", TESTDATA("data_out/TrioMaternalContaminationUncontaminated.txt"));
        }

        void tenPercentContamination()
        {
            EXECUTE("TrioMaternalContamination", "-bam_m " + TESTDATA("data_in/TrioMaternalContaminationMother.bam") + " -bam_f " + TESTDATA("data_in/TrioMaternalContaminationFather.bam") +
					" -bam_c " +TESTDATA("data_in/TrioMaternalContaminationChild10Perc.bam") + " -out out/TrioMaternalContaminationTenPercContamination.txt");
			COMPARE_FILES("out/TrioMaternalContaminationTenPercContamination.txt", TESTDATA("data_out/TrioMaternalContaminationTenPercContamination.txt"));
        }

};
