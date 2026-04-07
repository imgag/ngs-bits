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
	const QFileInfo info(file_path);

	BedTrack* bed_track = BedTrack::createTrack(parent, file_path, info.fileName());
	if (bed_track) out << bed_track;

	return out;
}

QSharedPointer<BedFile> FileLoader::loadBedFile(QString file_path)
{
	const QFileInfo info(file_path);
	if (!info.isFile())
	{
		GenomeVisualizationWidget::displayError(file_path + " not found");
		return nullptr;
	}
	try
	{
		QSharedPointer<BedFile> bedfile = QSharedPointer<BedFile>::create();
		bedfile->load(file_path);
		bedfile->sort();
		return bedfile;
	}
	catch (const FileParseException& e)
	{
		GenomeVisualizationWidget::displayError(e.message());
		return nullptr;
	}
}
