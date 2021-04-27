#include "DatabaseServiceRemote.h"
#include "Settings.h"
#include "HttpRequestHandler.h"

DatabaseServiceRemote::DatabaseServiceRemote()
	: enabled_(Settings::boolean("NGSD_enabled"))
{
}

bool DatabaseServiceRemote::enabled() const
{
	return enabled_;
}

BedFile DatabaseServiceRemote::processingSystemRegions(int sys_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	BedFile output;
	QByteArray reply = makeApiCall("ps_regions?sys_id="+QString::number(sys_id));

	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get the processing system regions for " + QString::number(sys_id));
	}
	else
	{
		output.fromText(reply);
	}

	return output;
}

BedFile DatabaseServiceRemote::processingSystemAmplicons(int sys_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	BedFile output;
	QByteArray reply = makeApiCall("ps_amplicons?sys_id="+QString::number(sys_id));

	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get the processing system amplicons for " + QString::number(sys_id));
	}
	else
	{
		output.fromText(reply);
	}

	return output;
}

GeneSet DatabaseServiceRemote::processingSystemGenes(int sys_id) const
{
	checkEnabled(__PRETTY_FUNCTION__);

	GeneSet output;
	QByteArray reply = makeApiCall("ps_genes?sys_id="+QString::number(sys_id));

	if (reply.length() == 0)
	{
		THROW(Exception, "Could not get the processing system genes for " + QString::number(sys_id));
	}
	else
	{
		output = GeneSet::createFromText(reply);
	}

	return output;
}

QByteArray DatabaseServiceRemote::makeApiCall(QString url_param) const
{
	HttpHeaders add_headers;
	add_headers.insert("Accept", "text/plain");
	return HttpRequestHandler(HttpRequestHandler::NONE).get(
			"https://" + Settings::string("server_host",true) + ":" + Settings::string("server_port")
			+ "/v1/"+url_param, add_headers);
}

