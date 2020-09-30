#include "FileLocationProviderFileSystem.h"
#include <iostream>

void FileLocationProviderFileSystem::doSomething()
{
	std::cout << theOnlyInstance->getFilename();
}
