#include "EndpointHandler.h"

EndpointHandler::EndpointHandler()
{
}

bool EndpointHandler::isValidUser(QString name, QString password)
{
	try
	{
		NGSD db;
		QString message = db.checkPassword(name, password, true);
		if (message.isEmpty())
		{
			return true;
		}
		else
		{
			return false;
		}

	}
	catch (DatabaseException& e)
	{
		qCritical() << e.message();
	}
	return false;
}

QString EndpointHandler::getGSvarFile(QString sample_name, bool search_multi)
{
	QString file;
	try
	{
		//convert name to file
		NGSD db;
		QString processed_sample_id = db.processedSampleId(sample_name);		
		file = db.processedSamplePath(processed_sample_id, PathType::GSVAR);

		//determine all analyses of the sample
		QStringList analyses;
		if (QFile::exists(file)) analyses << file;

		//somatic tumor sample > ask user if he wants to open the tumor-normal pair
		QString normal_sample = db.normalSample(processed_sample_id);
		if (normal_sample!="")
		{
			analyses << db.secondaryAnalyses(sample_name + "-" + normal_sample, "somatic");
		}
		//check for germline trio/multi analyses
		else if (search_multi)
		{
			analyses << db.secondaryAnalyses(sample_name, "trio");
			analyses << db.secondaryAnalyses(sample_name, "multi sample");
		}

		//determine analysis to load
		if (analyses.count()==0)
		{
			qWarning() << "The GSvar file does not exist:" << file;
			return "";
		}
		else if (analyses.count()==1)
		{
			file = analyses[0];
		}
		else
		{
			bool ok = false;
			QString filename = ""; //QInputDialog::getItem(this, "Several analyses of the sample present", "select analysis:", analyses, 0, false, &ok);
			if (!ok)
			{
				return "";
			}
			file = filename;
		}
	}
	catch (Exception& e)
	{
		qWarning() << "Error opening processed sample from NGSD:" << e.message();
	}
	return file;
}

HttpResponse EndpointHandler::serveIndexPage(HttpRequest request)
{	
	return EndpointHelper::serveStaticFile(":/assets/client/info.html", ByteRange{}, ContentType::TEXT_HTML, false);
}

HttpResponse EndpointHandler::serveApiInfo(HttpRequest request)
{
	return EndpointHelper::serveStaticFile(":/assets/client/api.json", ByteRange{}, ContentType::APPLICATION_JSON, false);
}

HttpResponse EndpointHandler::serveTempUrl(HttpRequest request)
{
	UrlEntity url_entity = UrlManager::getURLById(request.getPathParams()[0]);
	if (url_entity.filename_with_path.isEmpty())
	{
		return HttpResponse(HttpError{StatusCode::NOT_FOUND, request.getContentType(), "Could not find a file linked to the id: " + request.getPathParams()[0]});
	}

	qDebug() << "Serving file: " << url_entity.filename_with_path;
	return EndpointHelper::serveStaticFile(url_entity.filename_with_path, ByteRange{}, HttpProcessor::getContentTypeByFilename(url_entity.filename_with_path), false);
}

HttpResponse EndpointHandler::locateFileByType(HttpRequest request)
{
	qDebug() << "File location service";
	QString path = ServerHelper::getStringSettingsValue("projects_folder");
	QString found_file = getGSvarFile(request.getUrlParams()["ps"], false);

	if (found_file.isEmpty())
	{
		return HttpResponse(HttpError{StatusCode::NOT_FOUND, request.getContentType(), "Could not find the sample: " + request.getUrlParams()["ps"]});
	}

	VariantList variants {};
	variants.load(found_file);

	FileLocationProviderLocal* file_locator = new FileLocationProviderLocal(found_file, variants.getSampleHeader(), variants.type());

	qDebug() << "found_file " << found_file;
	QList<FileLocation> file_list {};
	QJsonDocument json_doc_output {};
	QJsonArray json_list_output {};
	switch(FileLocation::stringToType(request.getUrlParams()["type"].toLower()))
	{
		case PathType::BAM:
			file_list = file_locator->getBamFiles(true);
			break;
		case PathType::COPY_NUMBER_RAW_DATA:
			file_list = file_locator->getCnvCoverageFiles(true);
			break;
		case PathType::BAF:
			file_list = file_locator->getMantaEvidenceFiles(true);
			break;
		case PathType::MANTA_EVIDENCE:
			file_list = file_locator->getMantaEvidenceFiles(true);
			break;

		default:
			{
				FileLocation gsvar_file {};
				gsvar_file.id = request.getUrlParams()["ps"];
				gsvar_file.type = PathType::GSVAR;
				gsvar_file.filename = found_file;
				gsvar_file.exists = true;
				file_list.append(gsvar_file);
			}
	}

	for (int i = 0; i < file_list.count(); ++i)
	{
		qDebug() << file_list[i].filename;
		QJsonObject cur_json_item {};
		cur_json_item.insert("id", file_list[i].id);
		cur_json_item.insert("type", FileLocation::typeToString(file_list[i].type));
		bool needs_url = true;
		if (request.getUrlParams().contains("path"))
		{
			if (request.getUrlParams()["path"].toLower() == "absolute") needs_url = false;

		}
		if (needs_url)
		{
			cur_json_item.insert("filename", createTempUrl(file_list[i].filename));
		}
		else
		{
			cur_json_item.insert("filename", file_list[i].filename);
		}

		cur_json_item.insert("exists", file_list[i].exists);
		json_list_output.append(cur_json_item);
	}

	json_doc_output.setArray(json_list_output);
	return HttpResponse{false, "", EndpointHelper::generateHeaders(json_doc_output.toJson().length(), ContentType::APPLICATION_JSON), json_doc_output.toJson()};
}

HttpResponse EndpointHandler::locateProjectFile(HttpRequest request)
{
	qDebug() << "Project file location";
	QString found_file = getGSvarFile(request.getUrlParams()["ps"], false);
	found_file = createTempUrl(found_file);
	return HttpResponse{false, "", EndpointHelper::generateHeaders(found_file.length(), ContentType::TEXT_PLAIN), found_file.toLocal8Bit()};
}

HttpResponse EndpointHandler::performLogin(HttpRequest request)
{
	QByteArray body {};
	if (!request.getFormUrlEncoded().contains("name") || !request.getFormUrlEncoded().contains("password"))
	{
		return HttpResponse(HttpError{StatusCode::FORBIDDEN, request.getContentType(), "No username or/and password were found"});
	}

	if (isValidUser(request.getFormUrlEncoded()["name"], request.getFormUrlEncoded()["password"]))
	{
		QString secure_token = ServerHelper::generateUniqueStr();
		Session cur_session = Session{request.getFormUrlEncoded()["name"], QDateTime::currentDateTime()};

		SessionManager::addNewSession(secure_token, cur_session);
		body = secure_token.toLocal8Bit();
		return HttpResponse{false, "", EndpointHelper::generateHeaders(body.length(), ContentType::TEXT_PLAIN), body};
	}

	return HttpResponse(HttpError{StatusCode::UNAUTHORIZED, request.getContentType(), "Invalid username or password"});
}

HttpResponse EndpointHandler::performLogout(HttpRequest request)
{
	QByteArray body {};
	if (!request.getFormUrlEncoded().contains("token"))
	{
		return HttpResponse(HttpError{StatusCode::FORBIDDEN, request.getContentType(), "Secure token has not been provided"});
	}
	if (SessionManager::isTokenValid(request.getFormUrlEncoded()["token"]))
	{
		try
		{
			SessionManager::removeSession(request.getFormUrlEncoded()["token"]);
		} catch (Exception& e)
		{
			return HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, request.getContentType(), e.message()});
		}
		body = request.getFormUrlEncoded()["token"].toLocal8Bit();
		return HttpResponse{false, "", EndpointHelper::generateHeaders(body.length(), ContentType::TEXT_PLAIN), body};
	}
	return HttpResponse(HttpError{StatusCode::FORBIDDEN, request.getContentType(), "You have provided an invalid token"});
}

QString EndpointHandler::createTempUrl(QString filename)
{
	QString id = ServerHelper::generateUniqueStr();
	UrlManager::addUrlToStorage(id, filename);
	return ServerHelper::getStringSettingsValue("server_host") +
			+ ":" + ServerHelper::getStringSettingsValue("server_port") +
			+ "/v1/temp/" + id;
}
