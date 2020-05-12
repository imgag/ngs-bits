#include "TestFramework.h"
#include "BedpeFile.h"

TEST_CLASS(BedpeFile_Test)
{
Q_OBJECT
private slots:

	void findMatch()
	{
		// load 2 identical files and find the SVs in the second 2
		BedpeFile file1, file2;
		file1.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));
		file2.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));


		QVector<int> indices;
		for (int i = 0; i < file1.count(); ++i)
		{
			indices << file2.findMatch(file1[i], true, true);
		}

		for (int i = 0; i < file2.count(); ++i)
		{
			I_EQUAL(indices[i], i);
		}
	}

};
