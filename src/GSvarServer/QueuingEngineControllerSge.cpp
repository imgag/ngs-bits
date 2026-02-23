#include "QueuingEngineControllerSge.h"
#include "Log.h"

QueuingEngineControllerSge::QueuingEngineControllerSge()
{

}

QString QueuingEngineControllerSge::getEngineName() const
{
	return "SGE";
}

void QueuingEngineControllerSge::submitJob(NGSD& db, int threads, QStringList queues, QStringList pipeline_args, QString working_directory, QString script, int job_id) const
{
	//Prepare qsub command
	QString sge_out_base = PipelineSettings::dataFolder() + "/sge/megSAP_sge_job_" + QString::number(job_id);
    QStringList qsub_args;
    qsub_args << "-V";
	if (debug_) QTextStream(stdout) << "megSAP pipeline:\t " << script << Qt::endl;
    if (script == "analyze_dragen.php") qsub_args << "-pe" << "smp" << "1";
    else qsub_args << "-pe" << "smp" << QString::number(threads);
    qsub_args << "-b" << "y";
	qsub_args << "-wd" << working_directory;
    qsub_args << "-m" << "n";
    qsub_args << "-e" << (sge_out_base + ".err");
    qsub_args << "-o" << (sge_out_base + ".out");
    qsub_args << "-q" << queues.join(",");
    qsub_args << "php";
    qsub_args << PipelineSettings::rootDir()+"/src/Pipelines/"+script;
    qsub_args << pipeline_args;

	if (debug_) QTextStream(stdout) << "SGE command:\t qsub " << qsub_args.join(" ") << Qt::endl;

	//Execute qsub command
	QByteArrayList result;
	QString command = "qsub";
	int exit_code = Helper::executeCommand(command, qsub_args, &result);
	Log::info(command + " " + qsub_args.join(" "));

	//Update NGSD on qsub failure
	if (exit_code != 0)
	{
		QByteArrayList details;
		details << ("SGE job submission failed: returned exit code " + QByteArray::number(exit_code) + "!");
		details << "Command:";
		details << (command + " " + qsub_args.join(" ")).toLatin1();
		details << "Output:";
		details << result.join('\n');
		db.addAnalysisHistoryEntry(job_id, "error", details);
		return;
	}

	//Update NGSD on qsub success
	QByteArrayList qe_result = result.join(" ").simplified().split(' ');
	QByteArray sge_id = qe_result[2]; //sge result has format: "Your job 17 ("php") has been submitted"

	if (Helper::isNumeric(sge_id) && sge_id.toInt()>0)
	{
		if (debug_) QTextStream(stdout) << "  Started with SGE id " << sge_id << Qt::endl;
		db.getQuery().exec("UPDATE analysis_job SET sge_id='"+sge_id+"' WHERE id="+QString::number(job_id));

		db.addAnalysisHistoryEntry(job_id, "started", QByteArrayList());
	}
	else
	{
		QByteArrayList details;
		details << "SGE job submission failed - could not determine queuing engine job number!";
		details << "Command:";
		details << (command + qsub_args.join(" ")).toLatin1();
		details << "Output:";
		details << result.join('\n');
		db.addAnalysisHistoryEntry(job_id, "error", details);
	}
}

bool QueuingEngineControllerSge::updateRunningJob(NGSD& db, const AnalysisJob &job, int job_id) const
{
	bool job_finished = true;
	QByteArrayList result;
	QString command = "qstat";
	QStringList qstat_args = QStringList() << "-u" << "*";
	int exit_code = Helper::executeCommand(command, qstat_args, &result);
	Log::info(command + " " + qstat_args.join(" "));

	if (exit_code == 0)
	{
		foreach(QByteArray line, result)
		{
			line = line.simplified();
			if (line.startsWith(job.sge_id.toLatin1() + ' '))
			{
				job_finished = false;
				QByteArrayList parts = line.split(' ');
				if (parts.count()<8) continue;

				QByteArray status = parts[4].trimmed().toLower();
				if (debug_) QTextStream(stdout) << "  Job queued/running (state: " << status << " queue: " << job.sge_queue << ")" << Qt::endl;

				if (status=="r" && job.sge_queue.isEmpty())
				{
					QByteArray queue = parts[7].split('@')[0].trimmed();

					SqlQuery query = db.getQuery();
					query.prepare("UPDATE analysis_job SET sge_queue=:0 WHERE id=:1");
					query.bindValue(0, queue);
					query.bindValue(1, job_id);
					query.exec();
					break;
				}
			}
		}
	}
	else
	{
		Log::warn(command + " " + qstat_args.join(" ") + " failed - skipping update of SGE job with id " + job.sge_id);
	}

	return job_finished;
}

void QueuingEngineControllerSge::checkCompletedJob(NGSD& db, QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const
{
	QByteArrayList result;
	QString command = "qacct";
	QStringList qacct_args = QStringList() << "-j" << qe_job_id;
	int exit_code = Helper::executeCommand(command, qacct_args, &result);

	Log::info(command + " " + qacct_args.join(" "));

	if (exit_code == 0)
	{
		QString sge_exit_code = "";

		foreach(QByteArray line, result)
		{
			line = line.simplified();
			if (line.startsWith("exit_status "))
			{
				sge_exit_code = line.split(' ')[1];
			}
		}

		if (sge_exit_code == "0")
		{
			if (debug_) QTextStream(stdout) << "	Job finished successfully" << Qt::endl;
			db.addAnalysisHistoryEntry(job_id, "finished", stdout_stderr);
		}
		else
		{
			if (debug_) QTextStream(stdout) << "	Job failed with exit code: " << sge_exit_code << Qt::endl;
			stdout_stderr.prepend(("job exit code: " + sge_exit_code).toLatin1());
			db.addAnalysisHistoryEntry(job_id, "error", stdout_stderr);
		}
	}
	else
	{
		Log::warn(command + " " + qacct_args.join(" ") + "' failed with exit code " + QString::number(exit_code));
	}
}

void QueuingEngineControllerSge::deleteJob(NGSD &db, const AnalysisJob &job, int job_id) const
{
	if (debug_) QTextStream(stdout) << "Canceling job " << job_id << " (type: " << job.type << " SGE-id: " << job.sge_id << ")" << Qt::endl;

	QByteArrayList result;
	if (!job.sge_id.isEmpty())
	{
		QString command = "qdel";
		QStringList qdel_args = QStringList() << job.sge_id;
		int exit_code = Helper::executeCommand(command, qdel_args, &result);
		Log::info(command + " " + qdel_args.join(" "));

		if (exit_code != 0)
		{
			Log::warn(command + " " + job.sge_id + "' failed with exit code " + QString::number(exit_code));
		}
	}

	db.addAnalysisHistoryEntry(job_id, "canceled", result);
}
