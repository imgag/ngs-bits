#include "TestFramework.h"
#include "BedpeFile.h"

TEST_CLASS(StructuralVariantType_Test)
{
Q_OBJECT
private slots:

	void stringConversion()
	{
		for ( int type_int = StructuralVariantType::DEL; type_int != StructuralVariantType::BND; type_int++ )
		{
		   StructuralVariantType type = static_cast<StructuralVariantType>(type_int);
		   I_EQUAL(type, StructuralVariantTypeFromString(StructuralVariantTypeToString(type)));
		}
	}



};
