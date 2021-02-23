#include "PrsTable.h"
#include "Exceptions.h"

PrsTable::PrsTable()
	: TsvFile()
{
}

void PrsTable::load(QString filename)
{
	TsvFile::load(filename);

	//check if all required columns are available
	foreach (const QByteArray& required_column, QByteArrayList() << "pgs_id" << "trait" << "score" << "percentile" << "build" << "pgp_id" << "citation")
	{
		if (!headers().contains(required_column))
		{
			THROW(FileParseException, "Required column '" + required_column + "' not found in PRS table!");
		}
	}
}
