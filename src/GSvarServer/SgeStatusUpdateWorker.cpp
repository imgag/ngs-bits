#include "SgeStatusUpdateWorker.h"
#include "Log.h"
#include "PipelineSettings.h"

SgeStatusUpdateWorker::SgeStatusUpdateWorker()
    : QRunnable()
{
}

void SgeStatusUpdateWorker::run()
{
	try
	{
        if (debug_) QTextStream(stdout) << "SGE update started" << QT_ENDL;

		NGSD db;

		//process jobs
		SqlQuery query = db.getQuery();
		query.exec("SELECT id FROM analysis_job ORDER BY id ASC");
		while(query.next())
		{
			int job_id  = query.value("id").toInt();
			AnalysisJob job = db.analysisInfo(job_id);
			QString status = job.lastStatus();
			try
			{
				if(status=="queued")
				{
					job.checkValid(job_id);
					startAnalysis(db, job, job_id);
				}
				if(status=="started")
				{
					job.checkValid(job_id);
					updateAnalysisStatus(db, job, job_id);
				}
				if(status=="cancel")
				{
					job.checkValid(job_id);
					canceledAnalysis(db, job, job_id);
				}
			}
			catch (Exception& e)
			{
                if (debug_) QTextStream(stdout) << "SGE job (id=" << QString::number(job_id) << ") update failed: " << e.message() << QT_ENDL;
				Log::info("SGE job (id=" + QString::number(job_id) + ") update failed: " + e.message());
			}
			catch (...)
			{
                if (debug_) QTextStream(stdout) << "SGE job (id=" << QString::number(job_id) << ") update failed with unkown error" << QT_ENDL;
				Log::info("SGE job (id=" + QString::number(job_id) + ") update failed with unkown error");
			}
		}

		//delete jobs that are older than 60 days
		query.exec("SELECT res.id FROM (SELECT j.id as id , MAX(jh.time) as last_update FROM analysis_job j, analysis_job_history jh WHERE jh.analysis_job_id=j.id GROUP BY j.id) as res WHERE res.last_update < SUBDATE(NOW(), INTERVAL 60 DAY)");
		while(query.next())
		{
			QString job_id = query.value("id").toString();
            if (debug_)
            {
                QTextStream(stdout) <<  "Removing job " << job_id << " because it is older than 60 days" << QT_ENDL;
            }
			db.getQuery().exec("DELETE FROM `analysis_job_history` WHERE analysis_job_id=" + job_id);
			db.getQuery().exec("DELETE FROM `analysis_job_sample` WHERE analysis_job_id=" + job_id);
			db.getQuery().exec("DELETE FROM `analysis_job` WHERE id=" + job_id);
		}

        if (debug_) QTextStream(stdout) << "SGE update done" << QT_ENDL;
	}
	catch (Exception& e)
	{
        if (debug_) QTextStream(stdout) << "SGE update failed: " << e.message() << QT_ENDL;
		Log::info("SGE status update failed: " + e.message());
	}
	catch (...)
	{
        if (debug_) QTextStream(stdout) << "SGE update failed with unkown error" << QT_ENDL;
		Log::info("SGE status update failed with unkown error");
	}

}

void SgeStatusUpdateWorker::startAnalysis(NGSD& db, const AnalysisJob& job, int job_id)
{
    if (debug_) QTextStream(stdout) << "Starting job " << job_id << " (type: " << job.type << ")" << QT_ENDL;

	//init
	QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");

	//get processed sample data
	QString project_folder;
	QList<ProcessedSampleData> ps_data;
	QList<QString> bams;
	QList<QString> infos;
	foreach(const AnalysisJobSample& s, job.samples)
	{
		QString ps_id = db.processedSampleId(s.name);

		ps_data << db.getProcessedSampleData(ps_id);
		bams << db.processedSamplePath(ps_id, PathType::BAM);
		infos << s.info;

		//get project folder from first sample
		if (project_folder.isEmpty())
		{
			project_folder = Helper::canonicalPath(QFileInfo(bams.last()).path()+"/../");
		}
	}

	//determine usable queues
	QStringList queues = PipelineSettings::queuesDefault();
	if(job.high_priority)
	{
		queues = PipelineSettings::queuesHighPriority();
	}

	QString script;
	QStringList args;
	if (job.type=="single sample")
	{
		QString folder = QFileInfo(bams[0]).path() + "/";
		args << "-folder " + folder;
		args << "-name " + ps_data[0].name;

		QString sys_type = ps_data[0].processing_system_type;
		if (sys_type=="RNA") //RNA
		{
			//only high_mem queues for RNA
			queues = PipelineSettings::queuesHighMemory();

			script = "analyze_rna.php";
			args << "--log "+folder+"analyze_rna_"+timestamp+".log";
		}
		else if (sys_type=="cfDNA (patient-specific)" || sys_type=="cfDNA")
		{
			script = "analyze_cfdna.php";
			args << "--log "+folder+"analyze_cfdna_"+timestamp+".log";
		}
		else if (sys_type=="lrGS") //longread WGS
		{
			script = "analyze_longread.php";
			args << "--log "+folder+"analyze_longread_"+timestamp+".log";
		}
		else //DNA
		{
			script = "analyze.php";
			args << "--log "+folder+"analyze_"+timestamp+".log";
		}
	}
	else if (job.type=="trio")
	{
		//check if single analysis is still running > skip for now
		if(singleSampleAnalysisRunning(db, job))
		{
            if (debug_)
            {
                QTextStream(stdout) << "Job " << job_id << " (" << job.type << ") postponed because at least one single sample analysis is still running" << QT_ENDL;
            }
			return;
		}

		//determine sample indices
		int c_idx = infos.indexOf("child");
		int f_idx = infos.indexOf("father");
		int m_idx = infos.indexOf("mother");

		//create output folder
		QString out_folder = project_folder + "/Trio_" + ps_data[c_idx].name + "_" + ps_data[f_idx].name + "_" + ps_data[m_idx].name + "/";
		if (Helper::mkdir(out_folder)==1)
		{
			if (!Helper::set777(out_folder))
			{
				db.addAnalysisHistoryEntry(job_id, "error", QByteArrayList() << ("Error while submitting analysis to SGE: Could not change privileges of folder " + out_folder.toLatin1()));
				return;
			}
		}

		script = (ps_data[0].processing_system_type=="lrGS") ? "trio_longread.php" : "trio.php";
		args << "-c " + bams[c_idx];
		args << "-f " + bams[f_idx];
		args << "-m " + bams[m_idx];
		args << "-out_folder " + out_folder;
		args << "--log " + out_folder + "/trio.log";
	}
	else if (job.type=="multi sample")
	{
		//check if single analysis is still running > skip for now
		if(singleSampleAnalysisRunning(db, job))
		{
            if (debug_)
            {
                QTextStream(stdout) << "Job " << job_id << " (" << job.type << ") postponed because at least one single sample analysis is still running" << QT_ENDL;
            }
			return;
		}

		//create output folder
		QString out_folder = project_folder + "/Multi";
		foreach(const AnalysisJobSample& s, job.samples) out_folder += "_" + s.name;
		if (Helper::mkdir(out_folder)==1)
		{
			if (!Helper::set777(out_folder))
			{
				db.addAnalysisHistoryEntry(job_id, "error", QByteArrayList() << ("Error while submitting analysis to SGE: Could not change privileges of folder " + out_folder.toLatin1()));
				return;
			}
		}

		//determine command and arguments
		script = (ps_data[0].processing_system_type=="lrGS") ? "multisample_longread.php" : "multisample.php";
		args << "-bams " + bams.join(" ");
		args << "-status " + infos.join(" ");
		args << "-out_folder " + out_folder;
		args << "--log " + out_folder + "/multi.log";
	}
	else if (job.type=="somatic")
	{
		//check if single analysis is still running > skip for now
		if(singleSampleAnalysisRunning(db, job))
		{
            if (debug_)
            {
                QTextStream(stdout) << "Job " << job_id << " (" << job.type << ") postponed because at least one single sample analysis is still running" << QT_ENDL;
            }
			return;
		}

		//determine sample indices
		int t_idx = infos.indexOf("tumor");
		int n_idx = infos.indexOf("normal");
		int r_idx = infos.indexOf("tumor_rna");

		//create output folder
		QString out_folder;
		if (n_idx!=-1)
		{
			out_folder = project_folder + "/Somatic_" + ps_data[t_idx].name + "-" + ps_data[n_idx].name + "/";
		}
		else
		{
			out_folder = project_folder + "/Somatic_" + ps_data[t_idx].name + "/";
		}
		if (Helper::mkdir(out_folder)==1)
		{
			if (!Helper::set777(out_folder))
			{
				db.addAnalysisHistoryEntry(job_id, "error", QByteArrayList() << ("Error while submitting analysis to SGE: Could not change privileges of folder " + out_folder.toLatin1()));
				return;
			}
		}

		script = (n_idx!=-1) ? "somatic_tumor_normal.php" : "somatic_tumor_only.php";
		args << "--log "+out_folder+"/somatic_dna_"+timestamp+".log";
		args << "-out_folder " + out_folder;
		args << "-prefix " + ps_data[t_idx].name + (n_idx!=-1 ? "-"+ps_data[n_idx].name : "");
		args << "-t_bam " + bams[t_idx];
		if (n_idx!=-1)
		{
			args << "-n_bam " + bams[n_idx];
		}
		if (r_idx!=-1)
		{
			args << "-t_rna_bam " + bams[r_idx];
		}
	}
	else
	{
		db.addAnalysisHistoryEntry(job_id, "error", QByteArrayList() << "Error while submitting analysis to SGE: Unknown analysis type '"+job.type.toLatin1()+"'!");
		return;
	}

	//determine number of threads to use (equal to the number of SGE slots)
	int threads = 4;
	foreach(const ProcessedSampleData& s, ps_data) //use more slots for WGS and RNA
	{
		if (s.processing_system_type=="WGS" || s.processing_system_type=="WGS (shallow)") threads = std::max(threads, 6);
		if (s.processing_system_type=="RNA") threads = std::max(threads, 5);
		if (s.processing_system_type=="lrGS") threads = std::max(threads, 8);
	}
	//handle number of threads when set in custom arguments
	bool threads_in_args = false;
	QStringList parts = job.args.simplified().split(' ');
	for (int i=0; i<parts.count()-1; ++i)
	{
		if (parts[i]=="-threads" && Helper::isNumeric(parts[i+1]))
		{
			threads_in_args = true;
			threads = Helper::toInt(parts[i+1]);
		}
	}
	if (!threads_in_args)
	{
		args << "-threads "+QString::number(threads);
	}

	//submit to queue
	QString sge_out_base = PipelineSettings::dataFolder() + "/sge/megSAP_sge_job_" + QString::number(job_id);
	QStringList qsub_args;
	qsub_args << "-V";
	qsub_args << "-pe" << "smp" << QString::number(threads);
	qsub_args << "-b" << "y";
	qsub_args << "-wd" << project_folder;
	qsub_args << "-m" << "n";
	qsub_args << "-e" << (sge_out_base + ".err");
	qsub_args << "-o" << (sge_out_base + ".out");
	qsub_args << "-q" << queues.join(",");
	qsub_args << "php "+PipelineSettings::rootDir()+"/src/Pipelines/"+script+" " + job.args + " " + args.join(" ");
	QByteArrayList output;
	int exit_code = Helper::executeCommand("qsub", qsub_args, &output);
	if (exit_code!=0)
	{
		QByteArrayList details;
		details << ("SGE job submission failed: returned exit code "+QByteArray::number(exit_code)+"!");
		details << "Command:";
		details << ("qsub " + qsub_args.join(" ")).toLatin1();
		details << "Output:";
		details << output.join('\n');
		db.addAnalysisHistoryEntry(job_id, "error", details);
	}
	QByteArray sge_id = output.join(" ").simplified().split(' ')[2];

	//handle qsub output
	if (Helper::isNumeric(sge_id) && sge_id.toInt()>0)
	{
        if (debug_) QTextStream(stdout) << "  Started with SGE id " << sge_id << QT_ENDL;
		db.getQuery().exec("UPDATE analysis_job SET sge_id='"+sge_id+"' WHERE id="+QString::number(job_id));

		db.addAnalysisHistoryEntry(job_id, "started", QByteArrayList());
	}
	else
	{
		QByteArrayList details;
		details << "SGE job submission failed - could not determine SGE job number!";
		details << "Command:";
		details << ("qsub " + qsub_args.join(" ")).toLatin1();
		details << "Output:";
		details << output.join('\n');
		db.addAnalysisHistoryEntry(job_id, "error", details);
	}
}

void SgeStatusUpdateWorker::updateAnalysisStatus(NGSD& db, const AnalysisJob& job, int job_id)
{
    if (debug_) QTextStream(stdout) << "Updating status of job " << job_id << " (type: " << job.type << " SGE-id: " << job.sge_id << ")" << QT_ENDL;

	//check if job is still running
	int exit_code = Helper::executeCommand("qstat", QStringList() << "-j" << job.sge_id);
	if (exit_code==0) //still running/queued > update NGSD infos if necessary
	{
		QByteArrayList output;
		int exit_code2 = Helper::executeCommand("qstat", QStringList() << "-u" << "*", &output);
		if (exit_code2==0)
		{
			foreach(QByteArray line, output)
			{
				line = line.simplified();
				if (!line.startsWith(job.sge_id.toLatin1() + ' ')) continue;
				QByteArrayList parts = line.split(' ');
				if (parts.count()<8) continue;

				QByteArray status = parts[4].trimmed();
                if (debug_) QTextStream(stdout) << "  Job queued/running (state: " << status << " queue: " << job.sge_queue << ")" << QT_ENDL;

				if (status=="r" && job.sge_queue.isEmpty())
				{
					QByteArray queue = parts[7].split('@')[0].trimmed();

					SqlQuery query = db.getQuery();
					query.prepare("UPDATE analysis_job SET sge_queue=:0 WHERE id=:1");
					query.bindValue(0, queue);
					query.bindValue(1, job_id);
					query.exec();
				}
			}
		}
		else
		{
			Log::warn("qstat -u '*' failed - skipping update of SGE job with id " + job.sge_id);
		}
	}
	else //finished => add status in NGSD
	{
		//load stdout/stderr output
		QByteArrayList stdout_stderr;
		QString base = PipelineSettings::dataFolder() + "/sge/megSAP_sge_job_" + QString::number(job_id);
		if (QFile::exists(base+".out"))
		{
			foreach(QString line, Helper::loadTextFile(base+".out"))
			{
				stdout_stderr << line.toLatin1();
			}
		}
		if (QFile::exists(base+".err"))
		{
			foreach(QString line, Helper::loadTextFile(base+".err"))
			{
				stdout_stderr << line.toLatin1();
			}
		}

		QByteArrayList output;
		int exit_code2 = Helper::executeCommand("qacct", QStringList() << "-j" << job.sge_id, &output);
		if (exit_code2==0)
		{
			QString sge_exit_code = "";
			foreach(QByteArray line, output)
			{
				line = line.simplified();
				if (line.startsWith("exit_status "))
				{
					sge_exit_code = line.split(' ')[1];
				}
			}
			if (sge_exit_code=="0")
			{
                if (debug_) QTextStream(stdout) << "	Job finished successfully" << QT_ENDL;
				db.addAnalysisHistoryEntry(job_id, "finished", stdout_stderr);
			}
			else
			{
                if (debug_) QTextStream(stdout) << "	Job failed with exit code: " << sge_exit_code << QT_ENDL;
				stdout_stderr.prepend(("job exit code: " + sge_exit_code).toLatin1());
				db.addAnalysisHistoryEntry(job_id, "error", stdout_stderr);
			}
		}
		else
		{
			Log::warn("qacct -j '" + job.sge_id + "' failed with exit code " + QString::number(exit_code2));
		}
	}
}

void SgeStatusUpdateWorker::canceledAnalysis(NGSD& db, const AnalysisJob& job, int job_id)
{
    if (debug_) QTextStream(stdout) << "Canceling job " << job_id << " (type: " << job.type << " SGE-id: " << job.sge_id << ")" << QT_ENDL;

	//cancel job
	QByteArrayList output;
	if (!job.sge_id.isEmpty()) // not started yet => nothing to cancel
	{
		int exit_code = Helper::executeCommand("qdel", QStringList() << job.sge_id, &output);
		if (exit_code!=0)
		{
			Log::warn("qdel " + job.sge_id + "' failed with exit code " + QString::number(exit_code));
		}
	}

	//update NGSD
	db.addAnalysisHistoryEntry(job_id, "canceled", output);
}

bool SgeStatusUpdateWorker::singleSampleAnalysisRunning(NGSD& db, const AnalysisJob& job)
{
	foreach (const AnalysisJobSample& sample, job.samples)
	{
		QString ps_id = db.processedSampleId(sample.name);

		SqlQuery query = db.getQuery();
		query.exec("SELECT j.id FROM analysis_job j, analysis_job_sample js WHERE j.type='single sample' AND js.analysis_job_id=j.id AND js.processed_sample_id='"+ps_id+"' ORDER BY id DESC");
		while(query.next())
		{
			AnalysisJob job_single = db.analysisInfo(query.value("id").toInt());
			if (job_single.isRunning()) return true;
		}

	}
	return false;
}
