#ifndef DATABASESERVICEREMOTE_H
#define DATABASESERVICEREMOTE_H

#include "DatabaseService.h"
#include "Exceptions.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

class DatabaseServiceRemote
	: virtual public DatabaseService
{
public:
	DatabaseServiceRemote();
	virtual ~DatabaseServiceRemote() {}

	virtual bool enabled() const override;

	virtual BedFile processingSystemRegions(int sys_id) const override;
	virtual BedFile processingSystemAmplicons(int sys_id) const override;
	virtual GeneSet processingSystemGenes(int sys_id) const override;

	virtual FileLocation processedSamplePath(const QString& processed_sample_id, PathType type) const override;

protected:
	QByteArray makeApiCall(QString url_param) const;

	//Throws an error if NGSD is not enabled
	void checkEnabled(QString function) const
	{
		if (!enabled_)
		{
			THROW(ProgrammingException, "NGSD is not enabled, but instance requested in '" + function + "'");
		}
	}

	bool enabled_;
};

#endif // DATABASESERVICEREMOTE_H
