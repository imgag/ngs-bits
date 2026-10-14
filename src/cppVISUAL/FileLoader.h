#ifndef FILELOADER_H
#define FILELOADER_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "BedFile.h"
#include "IgvFile.h"
#include "BamReader.h"
#include "FastaFileIndex.h"
#include <functional>
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
	// Loads numeric IGV records and header metadata.
	static QSharedPointer<IgvFile> loadIgvFile(QString file_path);
	//Scans the reference for contiguous N/n runs. Returning false from progress cancels the scan.
	static BedFile referenceNRegions(const FastaFileIndex& genome, const std::function<bool(int, int)>& progress = {});
private:
	// checks if the Bed File is an IGV file
	static bool isValidIgvFile(QSharedPointer<BedFile> bed_file);
};

#endif // FILELOADER_H
