#include "TestFramework.h"
#include "OntologyTermCollection.h"

TEST_CLASS(OntologyTermCollection_Test)
{
Q_OBJECT
private slots:

	void loadFromOboFile()
	{
                OntologyTermCollection colletion("://Resources/so-xp_3_0_0.obo", true);

		IS_THROWN(FileAccessException,OntologyTermCollection test("LKJDSAFL",true));
                IS_THROWN(ArgumentException,OntologyTerm obsolete_term = colletion.findByID("hdskafhkj"));

                IS_TRUE(colletion.containsByID("SO:0000011"));
                IS_FALSE(colletion.containsByID("adskhf"));

                IS_TRUE(colletion.containsByName("scRNA_primary_transcript"));
                IS_FALSE(colletion.containsByName("lajfdslajfe"));

                S_EQUAL(colletion.findByID("SO:0000013").name(), "scRNA");

                IS_TRUE(colletion.findByID("SO:0000013").isChildOf("SO:0000655"));
                IS_FALSE(colletion.findByID("SO:0000013").isChildOf("SO:0000658"));
	}

};
