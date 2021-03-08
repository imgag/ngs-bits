#include "FileLocationProviderRemote.h"
#include "HttpRequestHandler.h"

FileLocationProviderRemote::FileLocationProviderRemote(const QString sample_id,const QString server_host, const int server_port)
	: sample_id_(sample_id)
	, server_host_(server_host)
	, server_port_(server_port)
{
}

QList<FileLocation> FileLocationProviderRemote::requestFileInfoByType(PathType type)
{
	QList<FileLocation> output {};
	if (sample_id_ == nullptr)
	{
		THROW(ArgumentException, "File name has not been specified")
		return output;
	}

	HttpHeaders add_headers;
	add_headers.insert("Accept", "application/json");
	QString reply = HttpRequestHandler(HttpRequestHandler::NONE).get("https://" + server_host_ + ":" + QString::number(server_port_) + "/v1/file_location?ps=" + sample_id_ + "&type=" + FileLocationHelper::pathTypeToString(type), add_headers);

	QJsonDocument json_doc = QJsonDocument::fromJson(reply.toLatin1());
	QJsonArray file_list = json_doc.array();

	if (file_list.count() == 0)
	{
		THROW(Exception, "Could not find file info: " + FileLocationHelper::pathTypeToString(type));
	}

	qDebug() << "Requested files:" << file_list;
	output = mapJsonArrayToFileLocationList(file_list);
	return output;
}

FileLocation FileLocationProviderRemote::mapJsonObjectToFileLocation(QJsonObject obj)
{
	return FileLocation {
		obj.value("id").toString(),
		FileLocationHelper::stringToPathType(obj.value("type").toString()),
		obj.value("filename").toString(),
		obj.value("is_found").toBool()
	};
}

QList<FileLocation> FileLocationProviderRemote::mapJsonArrayToFileLocationList(QJsonArray array)
{
	QList<FileLocation> output {};
	for (int i = 0; i < array.count(); ++i)
	{
		output.append(mapJsonObjectToFileLocation(array[i].toObject()));
	}
	return output;
}

QList<FileLocation> FileLocationProviderRemote::getBamFiles()
{
	return requestFileInfoByType(PathType::BAM);
}

QList<FileLocation> FileLocationProviderRemote::getSegFilesCnv()
{
		return requestFileInfoByType(PathType::COPY_NUMBER_RAW_DATA);
}

QList<FileLocation> FileLocationProviderRemote::getIgvFilesBaf()
{
	return requestFileInfoByType(PathType::BAF);
}

QList<FileLocation> FileLocationProviderRemote::getMantaEvidenceFiles()
{
	return requestFileInfoByType(PathType::MANTA_EVIDENCE);
}

QList<FileLocation> FileLocationProviderRemote::getAnalysisLogFiles()
{
	return QList<FileLocation>{};
}

QList<FileLocation> FileLocationProviderRemote::getCircosPlotFiles()
{
	return QList<FileLocation>{};
}

QList<FileLocation> FileLocationProviderRemote::getVcfGzFiles()
{
	return QList<FileLocation>{};
}

QList<FileLocation> FileLocationProviderRemote::getExpansionhunterVcfFiles()
{
	return QList<FileLocation>{};
}

QList<FileLocation> FileLocationProviderRemote::getPrsTsvFiles()
{
	return QList<FileLocation>{};
}

QList<FileLocation> FileLocationProviderRemote::getClincnvTsvFiles()
{
	return QList<FileLocation>{};
}

QList<FileLocation> FileLocationProviderRemote::getLowcovBedFiles()
{
	return QList<FileLocation>{};
}

QList<FileLocation> FileLocationProviderRemote::getStatLowcovBedFiles()
{
	return QList<FileLocation>{};
}

QList<FileLocation> FileLocationProviderRemote::getCnvsClincnvSegFiles()
{
	return QList<FileLocation>{};
}

QList<FileLocation> FileLocationProviderRemote::getCnvsClincnvTsvFiles()
{
	return QList<FileLocation>{};
}

QList<FileLocation> FileLocationProviderRemote::getCnvsSegFiles()
{
	return QList<FileLocation>{};
}

QList<FileLocation> FileLocationProviderRemote::getCnvsTsvFiles()
{
	return QList<FileLocation>{};
}

QList<FileLocation> FileLocationProviderRemote::getRohsTsvFiles()
{
	return QList<FileLocation>{};
}

QString FileLocationProviderRemote::getProjectAbsolutePath()
{
	return "";
}

QString FileLocationProviderRemote::getProjectParentAbsolutePath()
{
	return "";
}

QString FileLocationProviderRemote::getRohFileAbsolutePath()
{
	return "";
}

QString FileLocationProviderRemote::processedSampleName()
{
	return "";
}
