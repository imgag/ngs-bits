#ifndef IGVTRACKSETTINGS_H
#define IGVTRACKSETTINGS_H

#include "BedFile.h"
#include <QMap>
#include <QSharedPointer>
#include <QVariant>

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
