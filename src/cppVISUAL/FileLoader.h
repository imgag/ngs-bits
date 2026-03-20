#ifndef FILELOADER_H
#define FILELOADER_H


#include "SharedData.h"

#include <QSharedPointer>
#include <QFileInfo>

class FileLoader
{
public:
	static TrackList load(QString file_path)
	{
		if (file_path.endsWith(".bed")) return loadBedFile(file_path);

		SharedData::displayError("Unsupported file type.");
		return TrackList();
	}

	static TrackList loadBedFile(QString file_path)
	{
		TrackList out;

		BedFile file;
		const QFileInfo info(file_path);
		if (info.isFile())
		{
			try
			{
				file.load(file_path);
			}
			catch (const FileParseException& e)
			{
				SharedData::displayError(e.message());
				return out;
			}

			if (file.chromosomes().count() == 0)
			{
				SharedData::displayError("Bed file does not contain any chromosomes, will be discarded.");
				return out;
			}

			file.sort();
			out << QSharedPointer<TrackData>::create(file_path, info.fileName(), file);
		}
		return out;
	}
};

#endif // FILELOADER_H
