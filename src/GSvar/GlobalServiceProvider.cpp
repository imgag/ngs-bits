#include <stdio.h>
#include <stddef.h>
#include <iostream>
#include "GlobalServiceProvider.h"


GlobalServiceProvider* GlobalServiceProvider::theOnlyInstance = 0;

GlobalServiceProvider::GlobalServiceProvider()
{
}

int GlobalServiceProvider::getVariants()
{
	return variants;
}

void GlobalServiceProvider::setVariants(int _in)
{
	variants = _in;
}

int GlobalServiceProvider::getFilename()
{
	return filename;
}
void GlobalServiceProvider::setFilename(int _in)
{
	filename = _in;
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
