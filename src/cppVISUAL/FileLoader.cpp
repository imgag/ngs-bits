#include "FileLoader.h"
#include "BedTrack.h"
#include "GenomeVisualizationWidget.h"

TrackWidgetList FileLoader::load(QString file_path, QWidget* parent)
{
	if (file_path.endsWith(".bed")) return loadBedFile(file_path, parent);

	GenomeVisualizationWidget::displayError("Unsupported file type.");
	return TrackWidgetList();
}

TrackWidgetList FileLoader::loadBedFile(QString file_path, QWidget* parent)
{
	TrackWidgetList out;

	QSharedPointer<BedFile> file = QSharedPointer<BedFile>::create();
	const QFileInfo info(file_path);
	if (info.isFile())
	{
		try
		{
			file->load(file_path);
		}
		catch (const FileParseException& e)
		{
			GenomeVisualizationWidget::displayError(e.message());
			return out;
		}

		file->sort();
		out << new BedTrack(parent, file_path, info.fileName(), file);
	}
	return out;
}
