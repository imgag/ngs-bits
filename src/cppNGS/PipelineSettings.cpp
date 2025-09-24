#include "PipelineSettings.h"
#include "Helper.h"
#include <QFileInfo>

PipelineSettings::PipelineSettings()
{
}

void PipelineSettings::loadSettings(QString ini_file)
{
	QSharedPointer<QFile> file = Helper::openFileForReading(ini_file);

	instance().root_dir_ = QFileInfo(ini_file).canonicalPath() + "/";
	while (!file->atEnd())
	{
		QString line = file->readLine().trimmed();

        // Skip comments, empty lines, and section headers
        if (line.isEmpty() || line.startsWith(';') || line.startsWith('#') || (line.startsWith('[') && line.endsWith(']')))
        {
            continue;
        }

        // Key-value pair
        QStringList parts = line.split("=", QT_SKIP_EMPTY_PARTS);
		if (parts.size() == 2)
        {
			QString key = parts.at(0).trimmed();
			QString value = parts.mid(1).join("=").trimmed();

			//handle double quotes around values
			if(value.startsWith('"') && value.endsWith('"'))
			{
                value = value.mid(1, value.size()-2).trimmed();
			}
			if (value.startsWith("[path]"))
			{
				value.replace("[path]", instance().root_dir_);
			}

			//handle PHP-style arrays in keys
			QString array_key;
            if (key.endsWith("]"))
            {
                int key_pos = key.indexOf("[");
                if (key_pos>-1)
                {
					array_key = key.mid(key_pos+1, key.length()-key_pos-2);
					array_key = array_key.replace("\"", "").replace("\'", "").trimmed();
					key = key.mid(0, key_pos);
                }
            }
			//qDebug() << key << array_key << value;

			if (array_key.isEmpty())
			{
				if (key=="data_folder")
				{
					instance().data_folder_ = value;
				}
                if (key=="queuing_engine")
                {
                    instance().queuing_engine = value;
                }
				if (key=="queues_default")
				{
					QStringList tmp = value.split(',');
					Helper::trim(tmp);
					instance().queues_default_ = tmp;
				}
				if (key=="queues_research")
				{
					QStringList tmp = value.split(',');
					Helper::trim(tmp);
					instance().queues_research_ = tmp;
				}
				if (key=="queues_high_priority")
				{
					QStringList tmp = value.split(',');
					Helper::trim(tmp);
					instance().queues_high_priority_ = tmp;
				}
				if (key=="queues_high_mem")
				{
					QStringList tmp = value.split(',');
					Helper::trim(tmp);
					instance().queues_high_mem_ = tmp;
				}
				if (key=="queues_dragen")
				{
					QStringList tmp = value.split(',');
					Helper::trim(tmp);
					instance().queues_dragen_ = tmp;
				}
			}
			else
			{
				if (key=="project_folder")
				{
					value = value.trimmed();
					if (!value.endsWith("/")) value += "/";
					instance().projects_folder_[array_key] = value;
				}
            }
        }
    }

	file->close();
}

bool PipelineSettings::isInitialized()
{
	return !instance().root_dir_.isEmpty();
}

QString PipelineSettings::rootDir()
{
	checkInitialized();
	return instance().root_dir_;
}

QString PipelineSettings::projectFolder(QString type)
{
	checkInitialized();
	return instance().projects_folder_[type];
}

QString PipelineSettings::dataFolder()
{
	checkInitialized();
	return instance().data_folder_;
}

QString PipelineSettings::queuingEngine()
{
    checkInitialized();
    return instance().queuing_engine_;
}

QStringList PipelineSettings::queuesDefault()
{
	checkInitialized();
	return instance().queues_default_;
}

QStringList PipelineSettings::queuesResearch()
{
	checkInitialized();
	return instance().queues_research_;
}

QStringList PipelineSettings::queuesHighPriority()
{
	checkInitialized();
	return instance().queues_high_priority_;
}

QStringList PipelineSettings::queuesHighMemory()
{
	checkInitialized();
	return instance().queues_high_mem_;
}

QStringList PipelineSettings::queuesDragen()
{
	checkInitialized();
	return instance().queues_dragen_;
}

PipelineSettings& PipelineSettings::instance()
{
    static PipelineSettings pipeline_settings;
	return pipeline_settings;
}

void PipelineSettings::checkInitialized()
{
  if (!instance().isInitialized()) THROW(Exception, "PipelineSettings singleton used, but not initialized!");
}
