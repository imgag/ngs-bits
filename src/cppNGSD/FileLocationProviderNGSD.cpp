#include "FileLocationProviderNGSD.h"
#include <iostream>

void FileLocationProviderNGSD::doSomethingElse()
{
	std::cout << theOnlyInstance->getVariants();
}
