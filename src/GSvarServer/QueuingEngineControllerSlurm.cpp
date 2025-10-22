#include "QueuingEngineControllerSlurm.h"
#include "Log.h"

QueuingEngineControllerSlurm::QueuingEngineControllerSlurm()
{

}

QString QueuingEngineControllerSlurm::getEngineName() const
{
	return "SLURM";
}

void QueuingEngineControllerSlurm::submitJob(NGSD& db, int threads, QStringList queues, QStringList pipeline_args, QString project_folder, QString script, QString job_args, int job_id) const
{
	QString slurm_out_base = PipelineSettings::dataFolder() + "/slurm/megSAP_slurm_job_" + QString::number(job_id);

	//Prepare sbatch arguments
    QStringList sbatch_args;
	if (debug_) QTextStream(stdout) << "megSAP pipeline:\t " << script << QT_ENDL;
    if (script == "analyze_dragen.php") sbatch_args << "--cpus-per-task=1";
    else sbatch_args << "--cpus-per-task=" + QString::number(threads);
    sbatch_args << "-D" << project_folder;
    sbatch_args << "--mail-type=NONE";
    sbatch_args << "-e" << (slurm_out_base + ".err");
    sbatch_args << "-o" << (slurm_out_base + ".out");

	queues.removeAll("");
	if (!queues.isEmpty()) sbatch_args << "-p" << queues.join(",");

	// Build command line for wrapped job
	QString command_php = "php " + PipelineSettings::rootDir()+"/src/Pipelines/" + script;

    if (!job_args.isEmpty())
    {
		command_php += " " + job_args;
    }
	command_php += " " + pipeline_args.join(" ");

	// Create bash script for command
	QString command_sh = slurm_out_base + "_cmd.sh";

	QFile file(command_sh);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream out(&file);
		out << "#!/bin/sh\n";
		out << command_php << "\n";
		file.close();
		QFile::setPermissions(command_sh, QFileDevice::ExeUser | QFileDevice::ReadUser | QFileDevice::WriteUser |
										   QFileDevice::ReadGroup | QFileDevice::ReadOther);
	}
	else QTextStream(stderr) << "Failed to write command script: " << command_sh << "\n";

	sbatch_args << command_sh;

	// Debug output
	if (debug_) QTextStream(stdout) << "Slurm command:\t sbatch " << sbatch_args.join(" ") << QT_ENDL;

	// Execute sbatch command
	QByteArrayList result;
	QString command = "sbatch";
	int exit_code = Helper::executeCommand(command, sbatch_args, &result);
	QFile::remove(command_sh);
	Log::info(command + " " + sbatch_args.join(" "));

	//Update NGSD on qsub failure
	if (exit_code!=0)
	{
		QByteArrayList details;
		details << ("Slurm job submission failed: returned exit code " + QByteArray::number(exit_code) + "!");
		details << "Command:";
		details << (command + " " + sbatch_args.join(" ")).toLatin1();
		details << "Output:";
		details << result.join('\n');
		db.addAnalysisHistoryEntry(job_id, "error", details);
		return;
	}

	//Update NGSD on qsub success
	QByteArrayList qe_result = result.join(" ").simplified().split(' ');
	QByteArray slurm_id = qe_result[3]; //slurm result has format: "Submitted batch job 17"

	//handle qsub output
	if (Helper::isNumeric(slurm_id) && slurm_id.toInt()>0)
	{
		if (debug_) QTextStream(stdout) << "  Started with Slurm id " << slurm_id << QT_ENDL;
		db.getQuery().exec("UPDATE analysis_job SET sge_id='" + slurm_id + "' WHERE id="+QString::number(job_id));

		db.addAnalysisHistoryEntry(job_id, "started", QByteArrayList());
	}
	else
	{
		QByteArrayList details;
		details << "Slurm job submission failed - could not determine queuing engine job number!";
		details << "Command:";
		details << (command + sbatch_args.join(" ")).toLatin1();
		details << "Output:";
		details << result.join('\n');
		db.addAnalysisHistoryEntry(job_id, "error", details);
	}
}

bool QueuingEngineControllerSlurm::updateRunningJob(NGSD& db, const AnalysisJob &job, int job_id) const
{
	bool job_finished = true;
	QByteArrayList result;
	QString command = "squeue";
	QStringList squeue_args = QStringList();
	int exit_code = Helper::executeCommand(command, squeue_args, &result);
	Log::info(command + " " + squeue_args.join(" "));

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
				if (debug_) QTextStream(stdout) << "  Job queued/running (state: " << status << " queue: " << job.sge_queue << ")" << QT_ENDL;

				if (status=="r" && job.sge_queue.isEmpty())
				{
					QByteArray queue = parts[1].split('@')[0].trimmed();

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
		Log::warn(command + " " + squeue_args.join(" ") + " failed - skipping update of Slurm job with id " + job.sge_id);
	}

	return job_finished;
}

void QueuingEngineControllerSlurm::checkCompletedJob(NGSD& db, QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const
{
	QByteArrayList result;
	QString command = "sacct";
	QStringList sacct_args = QStringList() << "-j" << qe_job_id;
	int exit_code = Helper::executeCommand(command, sacct_args, &result);

	Log::info(command + " " + sacct_args.join(" "));

	if (exit_code == 0)
	{
		QString slurm_exit_code = "";

		foreach(QByteArray line, result)
		{
			if (!line.startsWith(qe_job_id.toLatin1() + ' ')) continue;
			QByteArrayList parts = line.simplified().split(' ');
			if (parts.size() >= 7)
			{
				QByteArray exit_codes = parts.last();
				slurm_exit_code = exit_codes.split(':')[0];
			}
		}

		if (slurm_exit_code == "0")
		{
			if (debug_) QTextStream(stdout) << "	Job finished successfully" << QT_ENDL;
			db.addAnalysisHistoryEntry(job_id, "finished", stdout_stderr);
		}
		else
		{
			if (debug_) QTextStream(stdout) << "	Job failed with exit code: " << slurm_exit_code << QT_ENDL;
			stdout_stderr.prepend(("job exit code: " + slurm_exit_code).toLatin1());
			db.addAnalysisHistoryEntry(job_id, "error", stdout_stderr);
		}
	}
	else
	{
		Log::warn(command + " " + sacct_args.join(" ") + "' failed with exit code " + QString::number(exit_code));
	}
}

void QueuingEngineControllerSlurm::deleteJob(NGSD &db, const AnalysisJob &job, int job_id) const
{
	if (debug_) QTextStream(stdout) << "Canceling job " << job_id << " (type: " << job.type << " Slurm-id: " << job.sge_id << ")" << QT_ENDL;

	QByteArrayList result;
	if (!job.sge_id.isEmpty())
	{
		QString command = "scancel";
		QStringList scancel_args = QStringList() << job.sge_id;
		int exit_code = Helper::executeCommand(command, scancel_args, &result);
		Log::info(command + " " + scancel_args.join(" "));

		if (exit_code != 0)
		{
			Log::warn(command + " " + job.sge_id + "' failed with exit code " + QString::number(exit_code));
		}
	}

	db.addAnalysisHistoryEntry(job_id, "canceled", result);
}
