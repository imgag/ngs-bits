#include "FileLoader.h"
#include "BedTrack.h"
#include "GenomeVisualizationWidget.h"

TrackWidgetList FileLoader::loadTracks(QString file_path, QWidget* parent)
{
	if (file_path.endsWith(".bed")) return loadBedFileTracks(file_path, parent);

	GenomeVisualizationWidget::displayError("Unsupported file type.");
	return TrackWidgetList();
}

TrackWidgetList FileLoader::loadBedFileTracks(QString file_path, QWidget* parent)
{
	TrackWidgetList out;
	QSharedPointer<BedFile> bedfile = QSharedPointer<BedFile>::create();
	bool success = loadBedFile(file_path, bedfile);
	if (success)
	{
		const QFileInfo info(file_path);
		out << new BedTrack(parent, file_path, info.fileName(), bedfile);
	}
	return out;
}

bool FileLoader::loadBedFile(QString file_path, QSharedPointer<BedFile>& bedfile)
{
	const QFileInfo info(file_path);
	if (!info.isFile()) return false;
	try
	{
		bedfile->load(file_path);
		bedfile->sort();
		return true;
	}
	catch (const FileParseException& e)
	{
		GenomeVisualizationWidget::displayError(e.message());
		return false;
	}
}
