#ifndef BAMTRACKDATAMANAGER_H
#define BAMTRACKDATAMANAGER_H

#include "TrackData.h"


class BamTrackDataManager
{
public:
	static QSharedPointer<BamTrackData> getOrCreate(QString file_path);
	static void reload(QString file_path);
};

#endif // BAMTRACKDATAMANAGER_H
