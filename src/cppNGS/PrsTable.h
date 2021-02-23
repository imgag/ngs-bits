#ifndef PRSTABLE_H
#define PRSTABLE_H

#include "TsvFile.h"

#include "cppNGS_global.h"

///Polygenic risk score table datastructure
class CPPNGSSHARED_EXPORT PrsTable
	: public TsvFile
{
public:
	PrsTable();

	//Loads a PRS file
	void load(QString filename);

};

#endif // PRSTABLE_H
