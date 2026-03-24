#ifndef FILELOADER_H
#define FILELOADER_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "BedTrack.h"
#include "GenomeVisualizationWidget.h"

#include <QSharedPointer>
#include <QFileInfo>

class CPPVISUALSHARED_EXPORT FileLoader
{
public:
	static TrackWidgetList load(QString file_path, QWidget* parent = nullptr)
	{
		if (file_path.endsWith(".bed")) return loadBedFile(file_path, parent);

		GenomeVisualizationWidget::displayError("Unsupported file type.");
		return TrackWidgetList();
	}

	static TrackWidgetList loadBedFile(QString file_path, QWidget* parent)
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
};

#endif // FILELOADER_H
