#include <stdio.h>
#include <stddef.h>
#include <iostream>
#include "FileLocationProvider.h"


FileLocationProvider* FileLocationProvider::theOnlyInstance = 0;


FileLocationProvider::FileLocationProvider(int _initialValueX, int _initialValueY)
{
	filename =_initialValueX;
	variants = _initialValueY;
}

bool FileLocationProvider::exists()
{
   return (theOnlyInstance != NULL);
}

FileLocationProvider* FileLocationProvider::getInstance()
{
   if(theOnlyInstance == 0) std::cout << "Class has not been created" << std::endl;

   return theOnlyInstance;
}



void FileLocationProvider::create(int _initialValueX, int _initialValueY)
{
   if(theOnlyInstance)
	  std::cout << "Singleton has already been created" << std::endl;
   else
	  theOnlyInstance = new FileLocationProvider(_initialValueX, _initialValueY);
}
