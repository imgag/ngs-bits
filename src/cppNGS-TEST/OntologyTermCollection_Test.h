#include "TestFramework.h"
#include "OntologyTermCollection.h"

TEST_CLASS(OntologyTermCollection_Test)
{
Q_OBJECT
private slots:

	void loadFromOboFile()
	{
		OntologyTermCollection collection_with_obsoletes(":/W:/share/data/dbs/miso/so-xp_2_5_3_v2.obo",true);

		IS_THROWN(FileAccessException,OntologyTermCollection test("LKJDSAFL",true));
		IS_THROWN(ArgumentException,OntologyTerm obsolete_term = collection_with_obsoletes.findByID("hdskafhkj"));

		IS_TRUE(collection_with_obsoletes.containsByID("SO:0000011"));
		IS_FALSE(collection_with_obsoletes.containsByID("adskhf"));

		IS_TRUE(collection_with_obsoletes.containsByName("scRNA_primary_transcript"));
		IS_FALSE(collection_with_obsoletes.containsByName("lajfdslajfe"));

		S_EQUAL(collection_with_obsoletes.findByID("SO:0000013").name(), "scRNA");

		IS_TRUE(collection_with_obsoletes.findByID("SO:0000013").isChildOf("SO:0000655"));
		IS_FALSE(collection_with_obsoletes.findByID("SO:0000013").isChildOf("SO:0000658"));
	}

};
