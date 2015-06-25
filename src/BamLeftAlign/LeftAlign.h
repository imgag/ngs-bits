#ifndef __LEFTALIGN_H
#define __LEFTALIGN_H

#include "api/BamAlignment.h"

using namespace BamTools;

bool leftAlign(BamAlignment& alignment, std::string& referenceSequence);

#endif
