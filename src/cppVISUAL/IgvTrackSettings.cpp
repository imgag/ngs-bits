#include "IgvTrackSettings.h"

QMap<QString, QVariant> IgvTrackSettings::getSettings() const
{
	QMap<QString, QVariant> settings;
	settings["graph_mode"] = graph_mode;
	settings["track_height"] = track_height;
	settings["view_min"] = view_min;
	settings["view_max"] = view_max;
	return settings;
}


QString IgvTrackSettings::getValidationErrors() const
{
	QString errors = "";
	if (view_min >= view_max) errors += "View min >= view max\n";
	if (track_height <= 0) errors += "Track Height <= 0\n";
	return errors.trimmed();
}

void IgvTrackSettings::loadKeyValueFromXml(QString key, QString value)
{
	bool ok;
	if (key == "graph_mode")
	{
		int val = value.toInt(&ok);
		if (ok && val >= 0) graph_mode = static_cast<GraphType>(val);
	}
	else if (key == "track_height")
	{
		int val = value.toInt(&ok);
		if (ok && val > 0) track_height = val;
	}
	else if (key == "view_min")
	{
		float val = value.toFloat(&ok);
		if (ok && val >= 0.f) view_min = val;
	}
	else if (key == "view_max")
	{
		float val = value.toFloat(&ok);
		if (ok && val <= 1. && val >= 0.f) view_max = val;
	}
}

QSharedPointer<IgvTrackSettings> IgvTrackSettings::parseFromFile(QSharedPointer<BedFile> bed_file)
{
	QSharedPointer<IgvTrackSettings> settings = QSharedPointer<IgvTrackSettings>::create();
	if (!bed_file) return settings;

	for (const QByteArray& header : bed_file->headers())
	{
		if (!header.startsWith("#track")) continue;

		QList<QByteArray> kv_pairs = header.split(' ');
		foreach (const QByteArray attr, kv_pairs)
		{
			int idx = attr.indexOf('=');
			if (idx == -1) continue;

			QByteArray key = attr.left(idx).toLower();
			QByteArray val = attr.mid(idx + 1);

			// 1. Parse graph type
			if (key == "graphtype")
			{
				QByteArray lower_val = val.toLower();
				if (lower_val == "points") { settings->graph_mode = IgvTrackSettings::POINTS; }
				else if (lower_val == "heatmap") { settings->graph_mode = IgvTrackSettings::HEATMAP; }
				else if (lower_val == "bar_chart") { settings->graph_mode = IgvTrackSettings::BAR_CHART; }
				else if (lower_val == "line_plot") { settings->graph_mode = IgvTrackSettings::LINE_PLOT; }
			}
			// 2. Parse view limits
			else if (key == "viewlimits")
			{
				QList<QByteArray> limits = val.split(':');
				if (limits.size() == 2)
				{
					bool okMin, okMax;
					float minVal = limits[0].toFloat(&okMin);
					float maxVal = limits[1].toFloat(&okMax);
					if (okMin && okMax)
					{
						settings->view_min = minVal;
						settings->view_max = maxVal;
					}
				}
			}
			// 3. Parse track height
			else if (key == "maxheightpixels")
			{
				QList<QByteArray> heights = val.split(':');
				if (!heights.isEmpty())
				{
					bool ok;
					int parsed_height = heights[0].toInt(&ok);
					if (ok)
					{
						settings->track_height = parsed_height;
					}
				}
			}
		}
		break; // Found #track, exit loop
	}
	return settings;
}
