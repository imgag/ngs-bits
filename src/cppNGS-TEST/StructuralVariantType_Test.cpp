#include "TestFramework.h"
#include "BedpeFile.h"

TEST_CLASS(StructuralVariantType_Test)
{
private:

	TEST_METHOD(stringConversion)
	{
		for ( int type_int = StructuralVariantType::DEL; type_int != StructuralVariantType::BND; type_int++ )
		{
		   StructuralVariantType type = static_cast<StructuralVariantType>(type_int);
		   I_EQUAL(type, StructuralVariantTypeFromString(StructuralVariantTypeToString(type)));
		}
	}



};
