#include <stdio.h>
#include <stddef.h>
#include <iostream>
#include "GlobalServiceProvider.h"


GlobalServiceProvider* GlobalServiceProvider::theOnlyInstance = 0;

GlobalServiceProvider::GlobalServiceProvider()
{
}

VariantList GlobalServiceProvider::getVariants()
{
	return variants;
}

void GlobalServiceProvider::setVariants(VariantList v)
{
	variants = v;
}

QString GlobalServiceProvider::getFilename()
{
	return filename;
}
void GlobalServiceProvider::setFilename(QString f)
{
	filename = f;
}

bool GlobalServiceProvider::exists()
{
   return (theOnlyInstance != NULL);
}

GlobalServiceProvider* GlobalServiceProvider::getInstance()
{
	if(theOnlyInstance)
		std::cout << "Singleton has already been created" << std::endl;
	else
		theOnlyInstance = new GlobalServiceProvider();

	if(theOnlyInstance == 0) std::cout << "Class has not been created" << std::endl;

   return theOnlyInstance;
}
