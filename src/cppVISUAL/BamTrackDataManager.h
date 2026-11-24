#ifndef BAMTRACKDATAMANAGER_H
#define BAMTRACKDATAMANAGER_H

#include "BamTrackData.h"

class CPPVISUALSHARED_EXPORT BamTrackDataManager
{
public:
	//Returns cached BAM data for this viewer (shared by its alignment/coverage tracks).
	static QSharedPointer<BamTrackData> getOrCreate(GenomeVisualizationWidget* viewer, QString file_path);
	// if BamTrackData associated with file_path still alive, triggers its reload
	static void reload(GenomeVisualizationWidget* viewer, QString file_path);
};

#endif // BAMTRACKDATAMANAGER_H
