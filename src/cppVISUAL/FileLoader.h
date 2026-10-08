#ifndef FILELOADER_H
#define FILELOADER_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "BedFile.h"
#include "BamReader.h"
#include <QSharedPointer>
#include <QFileInfo>

class CPPVISUALSHARED_EXPORT FileLoader
{
public:
	// factory function that loads depending on the extension of the file
	static QVector<TrackWidget*> loadTracks(QString file_path, QWidget* parent = nullptr);
	// loads bed file
	static QSharedPointer<BedFile> loadBedFile(QString file_path);
	// loads bam File
	static QSharedPointer<BamReader> loadBamFile(QString file_path);
	// loads baf file
	static QSharedPointer<BedFile> loadIgvFile(QString file_path);
private:
	// checks if the Bed File is an IGV file
	static bool isValidIgvFile(QSharedPointer<BedFile> bed_file);
};

#endif // FILELOADER_H
