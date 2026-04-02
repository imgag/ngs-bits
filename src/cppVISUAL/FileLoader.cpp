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
	QSharedPointer<BedFile> bedfile = loadBedFile(file_path);
	if (bedfile)
	{
		const QFileInfo info(file_path);
		BedTrack* bedtrack = new BedTrack(parent, file_path, info.fileName());
		bedtrack->setBedFile(bedfile);
		out << bedtrack;
	}
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
