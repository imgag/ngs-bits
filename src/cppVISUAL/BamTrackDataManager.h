#ifndef BAMTRACKDATAMANAGER_H
#define BAMTRACKDATAMANAGER_H

#include "TrackData.h"

// enusres that one file_path only has one TrackData instance
class BamTrackDataManager
{
public:
	// if file_path in cache, returns cached BamTrackData
	static QSharedPointer<BamTrackData> getOrCreate(QString file_path);
	// if BamTrackData associated with file_path still alive, triggers its reload
	static void reload(QString file_path);
};

#endif // BAMTRACKDATAMANAGER_H
