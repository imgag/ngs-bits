#include <stdio.h>
#include <stddef.h>
#include <iostream>
#include "GlobalServiceProvider.h"


//int filename {};
//int variants {};





GlobalServiceProvider* GlobalServiceProvider::theOnlyInstance = 0;

GlobalServiceProvider::GlobalServiceProvider() {

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



//GlobalServiceProvider::GlobalServiceProvider(int _initialValueX, int _initialValueY)
//{
//	filename = getFilename();
//	variants = getVariants();
//}

//bool GlobalServiceProvider::exists()
//{
//   return (theOnlyInstance != NULL);
//}

GlobalServiceProvider* GlobalServiceProvider::getInstance()
{
	if(theOnlyInstance)
		std::cout << "Singleton has already been created" << std::endl;
	else
		theOnlyInstance = new GlobalServiceProvider();

	if(theOnlyInstance == 0) std::cout << "Class has not been created" << std::endl;

   return theOnlyInstance;
}



//void GlobalServiceProvider::create(int _initialValueX, int _initialValueY)
//{
//   if(theOnlyInstance)
//	  std::cout << "Singleton has already been created" << std::endl;
//   else
//	  theOnlyInstance = new FileLocationProvider(_initialValueX, _initialValueY);
//}
