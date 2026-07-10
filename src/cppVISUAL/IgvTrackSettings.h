#ifndef IGVTRACKSETTINGS_H
#define IGVTRACKSETTINGS_H

#include "BedFile.h"

#include <QDomElement>
#include <QMap>
#include <QSharedPointer>
#include <QVariant>

// TODO: make all TrackSettings generic
// struct TrackSettings //generic track settings
// {
// 	// virtual QMap<QString, QVariant> getSettings() = 0;
// 	// virtual void loadSettings() = 0;
// };

struct IgvTrackSettings
{
	enum GraphType
	{
		HEATMAP,
		BAR_CHART,
		POINTS,
		LINE_PLOT
	};

	GraphType graph_mode = POINTS;
	int track_height = 100;
	float view_min = 0.f;
	float view_max = 1.f;

	QString getValidationErrors() const;

	QMap<QString, QVariant> getSettings() const;

	void loadKeyValueFromXml(QString key, QString value);
	static QSharedPointer<IgvTrackSettings> parseFromFile(QSharedPointer<BedFile> bed_file);
};



#endif // IGVTRACKSETTINGS_H
