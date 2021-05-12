#include "MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include "Settings.h"
#include "Exceptions.h"
#include "ChromosomalIndex.h"
#include "Log.h"
#include "Helper.h"
#include "GUIHelper.h"
#include "GeneSet.h"
#include <QDir>
#include <QBitArray>
#include <QDesktopServices>
#include <QUrl>
#include <QTcpSocket>
#include <QTime>
#include "ExternalToolDialog.h"
#include "ReportDialog.h"
#include <QBrush>
#include <QFont>
#include <QInputDialog>
#include <QClipboard>
#include <QProgressBar>
#include <QToolButton>
#include <QMimeData>
#include <QSqlError>
#include <QChartView>
#include <GenLabDB.h>
#include <QToolTip>
#include <QProcess>
QT_CHARTS_USE_NAMESPACE
#include "ReportWorker.h"
#include "ScrollableTextDialog.h"
#include "AnalysisStatusWidget.h"
#include "HttpHandler.h"
#include "ValidationDialog.h"
#include "ClassificationDialog.h"
#include "BasicStatistics.h"
#include "ApprovedGenesDialog.h"
#include "GeneWidget.h"
#include "PhenoToGenesDialog.h"
#include "GenesToRegionsDialog.h"
#include "SubpanelDesignDialog.h"
#include "SubpanelArchiveDialog.h"
#include "IgvDialog.h"
#include "GapDialog.h"
#include "CnvWidget.h"
#include "CnvList.h"
#include "RohWidget.h"
#include "GeneSelectorDialog.h"
#include "NGSHelper.h"
#include "QCCollection.h"
#include "NGSDReannotationDialog.h"
#include "DiseaseInfoWidget.h"
#include "SmallVariantSearchWidget.h"
#include "TSVFileStream.h"
#include "LovdUploadDialog.h"
#include "OntologyTermCollection.h"
#include "SvWidget.h"
#include "VariantWidget.h"
#include "SomaticReportConfigurationWidget.h"
#include "SingleSampleAnalysisDialog.h"
#include "MultiSampleDialog.h"
#include "TrioDialog.h"
#include "SomaticDialog.h"
#include "Histogram.h"
#include "ProcessedSampleWidget.h"
#include "DBSelector.h"
#include "SequencingRunWidget.h"
#include "SimpleCrypt.h"
#include "ToolBase.h"
#include "BedpeFile.h"
#include "SampleSearchWidget.h"
#include "ProcessedSampleSelector.h"
#include "ReportVariantDialog.h"
#include "SomaticReportVariantDialog.h"
#include "GSvarHelper.h"
#include "SampleDiseaseInfoWidget.h"
#include "QrCodeFactory.h"
#include "SomaticRnaReport.h"
#include "ProcessingSystemWidget.h"
#include "ProjectWidget.h"
#include "DBEditor.h"
#include "TsvTableWidget.h"
#include "DBTableAdministration.h"
#include "SequencingRunOverview.h"
#include "MidCheckWidget.h"
#include "CnvSearchWidget.h"
#include "VariantValidationWidget.h"
#include "SomaticReportDialog.h"
#include "GeneOmimInfoWidget.h"
#include "LoginManager.h"
#include "LoginDialog.h"
#include "GeneInfoDBs.h"
#include "VariantConversionWidget.h"
#include "PasswordDialog.h"
#include "CircosPlotWidget.h"
#include "SomaticReportSettings.h"
#include "CytobandToRegionsDialog.h"
#include "RepeatExpansionWidget.h"
#include "SomaticDataTransferWidget.h"
#include "PRSWidget.h"
#include "EvaluationSheetEditDialog.h"
#include "SvSearchWidget.h"
#include "PublishedVariantsWidget.h"
#include "PreferredTranscriptsWidget.h"
#include "TumorOnlyReportWorker.h"
#include "TumorOnlyReportDialog.h"
#include "VariantScores.h"
#include "CfDNAPanelDesignDialog.h"
#include "DiseaseCourseWidget.h"
#include "CfDNAPanelWidget.h"
#include "ClinvarSubmissionGenerator.h"
#include "SomaticVariantInterpreterWidget.h"
#include "AlleleBalanceCalculator.h"
#include "ExpressionDataWidget.h"
#include "GapClosingDialog.h"
#include "XmlHelper.h"
#include "GermlineReportGenerator.h"
#include "SomaticReportHelper.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
	, var_last_(-1)
	, busy_dialog_(nullptr)
	, notification_label_(new QLabel())
	, filename_()
	, igv_initialized_(false)
	, variants_changed_()
	, last_report_path_(QDir::homePath())
	, init_timer_(this, true)
{
	//setup GUI
	ui_.setupUi(this);
	setWindowTitle(QCoreApplication::applicationName());
	GUIHelper::styleSplitter(ui_.splitter);
	ui_.splitter->setStretchFactor(0, 10);
	ui_.splitter->setStretchFactor(1, 1);
	GUIHelper::styleSplitter(ui_.splitter_2);
	ui_.splitter_2->setStretchFactor(0, 10);
	ui_.splitter_2->setStretchFactor(1, 1);
	connect(ui_.variant_details, SIGNAL(jumbToRegion(QString)), this, SLOT(openInIGV(QString)));
	connect(ui_.variant_details, SIGNAL(openVariantTab(Variant)), this, SLOT(openVariantTab(Variant)));
	connect(ui_.variant_details, SIGNAL(openGeneTab(QString)), this, SLOT(openGeneTab(QString)));
	connect(ui_.tabs, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
	ui_.actionDebug->setVisible(Settings::boolean("debug_mode_enabled", true));

	//NGSD search button
	auto ngsd_btn = new QToolButton();
	ngsd_btn->setObjectName("ngsd_search_btn");
	ngsd_btn->setIcon(QIcon(":/Icons/NGSD_search.png"));
	ngsd_btn->setToolTip("Open NGSD item as tab.");
	ngsd_btn->setMenu(new QMenu());
	ngsd_btn->menu()->addAction(ui_.actionOpenProcessedSampleTabByName);
	ngsd_btn->menu()->addAction(ui_.actionOpenSequencingRunTabByName);
	ngsd_btn->menu()->addAction(ui_.actionOpenGeneTabByName);
	ngsd_btn->menu()->addAction(ui_.actionOpenProjectTab);
	ngsd_btn->menu()->addAction(ui_.actionOpenVariantTab);
	ngsd_btn->menu()->addAction(ui_.actionOpenProcessingSystemTab);
	ngsd_btn->setPopupMode(QToolButton::InstantPopup);
	ui_.tools->insertWidget(ui_.actionSampleSearch, ngsd_btn);

	// add cfdna menu
	cfdna_menu_btn_ = new QToolButton();
	cfdna_menu_btn_->setObjectName("cfdna_btn");
	cfdna_menu_btn_->setIcon(QIcon(":/Icons/cfDNA.png"));
	cfdna_menu_btn_->setToolTip("Open cfDNA menu entries");
	cfdna_menu_btn_->setMenu(new QMenu());
	cfdna_menu_btn_->menu()->addAction(ui_.actionDesignCfDNAPanel);
	cfdna_menu_btn_->menu()->addAction(ui_.actionShowCfDNAPanel);
	cfdna_menu_btn_->menu()->addAction(ui_.actionCfDNADiseaseCourse);
	cfdna_menu_btn_->setPopupMode(QToolButton::InstantPopup);
	ui_.tools->addWidget(cfdna_menu_btn_);
	// deaktivate on default (only available in somatic)
	cfdna_menu_btn_->setVisible(false);
	cfdna_menu_btn_->setEnabled(false);

	//signals and slots
	connect(ui_.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui_.vars, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(varsContextMenu(QPoint)));
	connect(ui_.filters, SIGNAL(filtersChanged()), this, SLOT(refreshVariantTable()));
	connect(ui_.vars, SIGNAL(itemSelectionChanged()), this, SLOT(updateVariantDetails()));
	connect(ui_.vars, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(variantCellDoubleClicked(int, int)));
	connect(ui_.vars->verticalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(variantHeaderDoubleClicked(int)));
	ui_.vars->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_.vars->verticalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(varHeaderContextMenu(QPoint)));

	connect(ui_.actionDesignSubpanel, SIGNAL(triggered()), this, SLOT(openSubpanelDesignDialog()));
	connect(ui_.filters, SIGNAL(phenotypeImportNGSDRequested()), this, SLOT(importPhenotypesFromNGSD()));
	connect(ui_.filters, SIGNAL(phenotypeSubPanelRequested()), this, SLOT(createSubPanelFromPhenotypeFilter()));

	//variants tool bar
	connect(ui_.vars_copy_btn, SIGNAL(clicked(bool)), ui_.vars, SLOT(copyToClipboard()));
	connect(ui_.vars_resize_btn, SIGNAL(clicked(bool)), ui_.vars, SLOT(adaptColumnWidthsCustom()));
	ui_.vars_export_btn->setMenu(new QMenu());
	ui_.vars_export_btn->menu()->addAction("Export GSvar (filtered)", this, SLOT(exportGSvar()));
	ui_.vars_export_btn->menu()->addAction("Export VCF (filtered)", this, SLOT(exportVCF()));
	ui_.report_btn->setMenu(new QMenu());
	ui_.report_btn->menu()->addSeparator();
	ui_.report_btn->menu()->addAction(QIcon(":/Icons/Report.png"), "Generate report", this, SLOT(generateReport()));
	ui_.report_btn->menu()->addAction(QIcon(":/Icons/Report.png"), "Generate evaluation sheet", this, SLOT(generateEvaluationSheet()));
	ui_.report_btn->menu()->addAction("Show report configuration info", this, SLOT(showReportConfigInfo()));
	ui_.report_btn->menu()->addSeparator();
	ui_.report_btn->menu()->addAction(QIcon(":/Icons/Report_finalize.png"), "Finalize report configuration", this, SLOT(finalizeReportConfig()));
	ui_.report_btn->menu()->addSeparator();
	ui_.report_btn->menu()->addAction("Transfer somatic data to MTB", this, SLOT(transferSomaticData()) );
	connect(ui_.vars_folder_btn, SIGNAL(clicked(bool)), this, SLOT(openVariantListFolder()));
	connect(ui_.vars_ranking, SIGNAL(clicked(bool)), this, SLOT(variantRanking()));
	ui_.vars_af_hist->setMenu(new QMenu());
	ui_.vars_af_hist->menu()->addAction("Show AF histogram (all small variants)", this, SLOT(showAfHistogram_all()));
	ui_.vars_af_hist->menu()->addAction("Show AF histogram (small variants after filter)", this, SLOT(showAfHistogram_filtered()));
	ui_.vars_af_hist->menu()->addSeparator();
	ui_.vars_af_hist->menu()->addAction("Show CN histogram (CNVs in given region)", this, SLOT(showCnHistogram()));

	connect(ui_.ps_details, SIGNAL(clicked(bool)), this, SLOT(openProcessedSampleTabsCurrentAnalysis()));

	//if at home, use Patientenserver
	QString gsvar_report_folder = Settings::path("gsvar_report_folder", true);
	if (gsvar_report_folder!="" && QDir(gsvar_report_folder).exists())
	{
		last_report_path_ = gsvar_report_folder;
	}

	//add notification icon
	notification_label_->hide();
	notification_label_->setScaledContents(true);
	notification_label_->setMaximumSize(16,16);
	notification_label_->setPixmap(QPixmap(":/Icons/email.png"));
	ui_.statusBar->addPermanentWidget(notification_label_);
}

void MainWindow::on_actionDebug_triggered()
{
	QString user = Helper::userName();
	if (user=="ahsturm1")
	{
		QTime timer;
		timer.start();

		//Check HPO terms in NGSD
		/*
		NGSD db;
		PhenotypeList valid_terms;
		valid_terms << db.phenotypeChildTerms(db.phenotypeIdByName("Clinical course"), true);
		valid_terms << db.phenotypeChildTerms(db.phenotypeIdByName("Clinical modifier"), true);
		valid_terms << db.phenotypeChildTerms(db.phenotypeIdByName("Past medical history"), true);
		valid_terms << db.phenotypeChildTerms(db.phenotypeIdByName("Phenotypic abnormality"), true);

		auto file = Helper::openFileForWriting("C:\\Marc\\hpos.tsv");
		QTextStream stream(file.data());
		stream << "#hpo_id\thpo_name\tsamples\terrors\n";
		SqlQuery query = db.getQuery();
		query.exec("SELECT `disease_info`, COUNT(`sample_id`) as sample_count FROM `sample_disease_info` WHERE `type`='HPO term id' GROUP BY disease_info");
		while(query.next())
		{
			QStringList errors;

			QByteArray hpo_id = query.value(0).toByteArray().trimmed();
			QString samples = query.value(1).toString().trimmed();

			QString hpo_name;
			try
			{
				int id = db.phenotypeIdByAccession(hpo_id);

				hpo_name = db.phenotype(id).name();

				if (!valid_terms.containsAccession(hpo_id))
				{
					errors << "Not a child of 'phenotypic abnormality', 'Clinical modifier' or 'Past medical history'";
				}
			}
			catch(Exception & e)
			{
				errors << e.message();
			}

			stream << hpo_id << "\t"  << hpo_name << "\t"  << samples << "\t" << errors.join(", ") << "\n";
			stream.flush();
		}
		*/

		//export of recurring variants with similar phenotype
		/*
		NGSD db;
		auto file = Helper::openFileForWriting("C:\\Marc\\vars.tsv");
		QTextStream stream(file.data());
		stream << "#gene\ttranscript\tvariant\tHGVS.p\ttype\timpact\tgnomad_AF\tclassification\tnum_affected\tnum_unaffeacted\tnum_unknown\tshared_disease_group\tsamples_with_hpo\tshared_hpo_term\n";

		//NGSD db;
		int c_gene = 0;
		QStringList genes = db.getValues("SELECT DISTINCT gene FROM omim_gene og WHERE id IN (SELECT DISTINCT omim_gene_id  FROM omim_phenotype) ORDER BY gene ASC");
		foreach(QString gene, genes)
		{
			qDebug() << ++c_gene << gene;
			int gene_id = db.geneToApprovedID(gene.toLatin1());
			if (gene_id==-1)
			{
				stream << "##" << gene << ": skipped - no approved gene name\n";
				continue;
			}

			Transcript lct = db.longestCodingTranscript(gene_id, Transcript::ENSEMBL, false, false);
			if (lct.codingRegions().baseCount()==0)
			{
				stream << "##" << gene << ": skipped - no longest coding transcript\n";
				continue;
			}

			BedFile roi_coding = lct.codingRegions();
			roi_coding.extend(20);
			roi_coding.merge();


			SqlQuery query = db.getQuery();
			QString af = "0.001";
			Chromosome chr = roi_coding[0].chr();
			query.exec("SELECT v.id, v.start, v.end, v.ref, v.obs, v.coding, v.gnomad FROM variant v WHERE chr='" + chr.strNormalized(true)  + "' AND start>='" + QString::number(roi_coding[0].start()) + "' AND end<='" + QString::number(roi_coding[roi_coding.count()-1].end()) + "' AND (1000g IS NULL OR 1000g<=" + af + ") AND (gnomad IS NULL OR gnomad<=" + af + ") ORDER BY start");
			while(query.next())
			{
				QList<VariantTranscript> trans_infos;
				try
				{
					trans_infos = Variant::parseTranscriptString(query.value(5).toByteArray(), true);
				}
				catch(...) {} //do nothing (old RefSeq entries)

				foreach(const VariantTranscript& trans, trans_infos)
				{
					if ((trans.impact=="HIGH" || trans.impact=="MODERATE") &&  trans.id.startsWith(lct.name())) //no direct comparision of transcript name because we have mix transcripts with/without version number in NGSD.
					{
						QString variant_id = query.value(0).toString();
						QString var = chr.strNormalized(true) + ":" + query.value(1).toString() + "-" + query.value(2).toString() + " " + query.value(3).toString() + ">" + query.value(4).toString();
						QString af = query.value(6).toString();
						SqlQuery query2 = db.getQuery();
						query2.exec("SELECT s.disease_group, s.disease_status, s.id FROM sample s, processed_sample ps, project p, detected_variant dv, processing_system sys WHERE ps.processing_system_id=sys.id AND dv.processed_sample_id=ps.id AND ps.sample_id=s.id AND ps.project_id=p.id AND dv.variant_id=" + variant_id + " AND p.type='diagnostic' AND ps.quality!='bad' AND (sys.type='WES' OR sys.type='WGS')");

						int c_affected = 0;
						int c_unaffected = 0;
						int c_unknown = 0;
						QMap<QString, int> dg_affected;
						int samples_with_hpo = 0;
						QMap<QString, int> hpo_affected;
						while(query2.next())
						{
							QString disease_group = query2.value(0).toString();

							QString disease_status = query2.value(1).toString();
							if (disease_status=="Affected")
							{
								++c_affected;

								if (!dg_affected.contains(disease_group)) dg_affected[disease_group] = 0;
								dg_affected[disease_group] += 1;

								auto phenos = db.samplePhenotypes(query2.value(2).toString());
								if (phenos.count()>0) ++samples_with_hpo;
								foreach(const Phenotype& pheno, phenos)
								{
									QString hpo_name = pheno.name();
									if (!hpo_affected.contains(hpo_name)) hpo_affected[hpo_name] = 0;
									hpo_affected[hpo_name] += 1;
								}
							}
							else if (disease_status=="Unaffected")
							{
								++c_unaffected;
							}
							else
							{
								++c_unknown;
							}
						}
						if (c_affected<3) continue; //RESTRICTION at least 3 times in affected diagnostic WES/WGS samples
						QString dg_shared_by_affected;
						if (dg_affected.count()==1)
						{
							dg_shared_by_affected = dg_affected.keys().first();
						}
						QStringList hpos_shared_by_affected;
						foreach(QString hpo_name, hpo_affected.keys())
						{
							if (samples_with_hpo>=3 && hpo_affected[hpo_name]==samples_with_hpo) hpos_shared_by_affected << (hpo_name + " (" + QString::number(hpo_affected[hpo_name])+")");
							if (samples_with_hpo>=4 && hpo_affected[hpo_name]==samples_with_hpo-1) hpos_shared_by_affected << (hpo_name + " (" + QString::number(hpo_affected[hpo_name])+")");
						}
						stream << gene << "\t" << lct.name() << "\t" << var << "\t" << trans.hgvs_p << "\t" << trans.type << "\t" << trans.impact << "\t" << af << "\t" << db.getValue("SELECT class FROM variant_classification WHERE variant_id="+variant_id).toString() << "\t" << QString::number(c_affected) << "\t" << QString::number(c_unaffected) << "\t" << QString::number(c_unknown) << "\t" << dg_shared_by_affected << "\t" << QString::number(samples_with_hpo) << "\t" << hpos_shared_by_affected.join(", ") << "\n";
						stream.flush();
					}
				}
			}
		}
		*/

		//evaluation GSvar score/rank
		/*
		TsvFile output;
		output.addHeader("ps");
		output.addHeader("variants_causal");
		output.addHeader("variants_scored");
		output.addHeader("score");
		output.addHeader("rank");
		int c_top1 = 0;
		int c_top5 = 0;
		int c_top10 = 0;
		NGSD db;
		QStringList ps_names = db.getValues("SELECT DISTINCT CONCAT(s.name, '_0', ps.process_id) FROM sample s, processed_sample ps, diag_status ds, report_configuration rc, report_configuration_variant rcv, project p, processing_system sys WHERE ps.processing_system_id=sys.id AND (sys.type='WGS' OR sys.type='WES') AND ps.project_id=p.id AND p.type='diagnostic' AND ps.sample_id=s.id AND ps.quality!='bad' AND ds.processed_sample_id=ps.id AND ds.outcome='significant findings' AND rc.processed_sample_id=ps.id AND rcv.report_configuration_id=rc.id AND rcv.causal='1' AND rcv.type='diagnostic variant' AND s.disease_status='Affected'");
		qDebug() << "Processed samples to check:" << ps_names.count();
		QString algorithm = "GSvar_v1_noNGSD";
		QString special = "";
		foreach(QString ps, ps_names)
		{
			QString ps_id = db.processedSampleId(ps);
			if (db.getDiagnosticStatus(ps_id).outcome!="significant findings") continue;
			qDebug() << output.rowCount() << ps;

			//create phenotype list
			QHash<Phenotype, BedFile> phenotype_rois;
			QString sample_id = db.sampleId(ps);
			PhenotypeList phenotypes = db.getSampleData(sample_id).phenotypes;
			foreach(Phenotype pheno, phenotypes)
			{
				//pheno > genes
				GeneSet genes = db.phenotypeToGenes(pheno, true);

				//genes > roi
				BedFile roi;
				foreach(const QByteArray& gene, genes)
				{
					if (!gene2region_cache_.contains(gene))
					{
						BedFile tmp = db.geneToRegions(gene, Transcript::ENSEMBL, "gene", true);
						tmp.clearAnnotations();
						tmp.extend(5000);
						tmp.merge();
						gene2region_cache_[gene] = tmp;
					}
					roi.add(gene2region_cache_[gene]);
				}
				roi.merge();

				phenotype_rois[pheno] = roi;
			}

			try
			{
				//load variants
				VariantList variants;
				variants.load(db.processedSamplePath(ps_id, PathType::GSVAR));

				//score
				QList<Variant> blacklist;
				if (false)
				{
					blacklist = VariantScores::blacklist();
					special += "_blacklist";
				}
				VariantScores::Result result = VariantScores::score(algorithm, variants, phenotype_rois, blacklist);
				int c_scored = VariantScores::annotate(variants, result);
				int i_rank = variants.annotationIndexByName("GSvar_rank");
				int i_score = variants.annotationIndexByName("GSvar_score");

				//check rank of causal variant
				int rc_id = db.reportConfigId(ps_id);
				if (rc_id!=-1)
				{
					CnvList cnvs;
					BedpeFile svs;
					QStringList messages;
					QSharedPointer<ReportConfiguration> rc_ptr = db.reportConfig(rc_id, variants, cnvs, svs, messages);
					foreach(const ReportVariantConfiguration& var_conf, rc_ptr->variantConfig())
					{
						if (var_conf.causal && var_conf.variant_type==VariantType::SNVS_INDELS && var_conf.report_type=="diagnostic variant")
						{
							int var_index = var_conf.variant_index;
							if (var_index>=0)
							{
								const Variant& var = variants[var_index];
								if (var.chr().isAutosome() || var.chr().isGonosome())
								{
									output.addRow(QStringList() << ps << var.toString() << QString::number(c_scored) << var.annotations()[i_score] << var.annotations()[i_rank]);

									try
									{
										int rank = Helper::toInt(var.annotations()[i_rank]);
										if (rank==1) ++c_top1;
										if (rank<=5) ++c_top5;
										if (rank<=10) ++c_top10;
									}
									catch(...) {} //nothing to do here
								}
							}
						}
					}
				}
			}
			catch(Exception& e)
			{
				qDebug() << "  Error processing GSvar:" << e.message();
				continue;
			}
		}
		output.addComment("##Rank1: " + QString::number(c_top1) + " (" + QString::number(100.0*c_top1/output.rowCount(), 'f', 2) + "%)");
		output.addComment("##Top5 : " + QString::number(c_top5) + " (" + QString::number(100.0*c_top5/output.rowCount(), 'f', 2) + "%)");
		output.addComment("##Top10: " + QString::number(c_top10) + " (" + QString::number(100.0*c_top10/output.rowCount(), 'f', 2) + "%)");
		output.store("C:\\Marc\\ranking_" + QDate::currentDate().toString("yyyy-MM-dd") + "_" + algorithm + special + ".tsv");
		*/

		//import of sample relations from GenLab
		/*
		QStringList pairs;
		pairs << "DX070696	DX070760";

		NGSD db;
		GenLabDB db_genlab;
		foreach(QString pair, pairs)
		{
			QStringList parts = pair.split("\t");
			if (parts.count()!=2)
			{
				qDebug() << "Error: invalid line: " << pair;
				break;
			}
			QString sample1 = parts[0];
			QString sample2 = parts[1];
			qDebug() << sample1 << sample2;

			//check one direction
			QList<SampleRelation> relatives = db_genlab.relatives(sample1);
			foreach(const SampleRelation& rel, relatives)
			{
				if (rel.sample1==sample2)
				{
					db.addSampleRelation(rel);
				}
			}

			//check other direction
			relatives = db_genlab.relatives(sample2);
			foreach(SampleRelation rel, relatives)
			{
				if (rel.sample1==sample1)
				{
					db.addSampleRelation(rel);
				}
			}
		}
		*/

		//non-causal variants annotation
		/*
		NGSD db;
		QStringList input;
		input << TODO
		foreach(QString ps, input)
		{
			QString ps_id = db.processedSampleId(ps);
			QString text;
			QVariant rc_id = db.getValue("SELECT id FROM report_configuration WHERE processed_sample_id=:0", true, ps_id);
			if (rc_id.isValid())
			{
				//find causal small variants
				QStringList causal_ids = db.getValues("SELECT variant_id FROM report_configuration_variant WHERE causal='0' AND exclude_artefact='0' AND exclude_frequency='0' AND exclude_phenotype='0' AND exclude_mechanism='0' AND exclude_other='0' AND report_configuration_id=" + rc_id.toString());
				foreach(QString id, causal_ids)
				{
					Variant var = db.variant(id);
					QString genotype = db.getValue("SELECT genotype FROM detected_variant WHERE processed_sample_id='" + ps_id + "' AND variant_id='" + id + "'").toString();
					QString genes = db.getValue("SELECT gene FROM variant WHERE id='" + id + "'").toString();
					QString var_class = db.getValue("SELECT class FROM variant_classification WHERE variant_id='" + id + "'").toString();
					text += ", small variant: " + var.toString() + " (genotype:" + genotype + " genes:" + genes;
					if (var_class != "") text += " classification:" + var_class; // add classification, if exists
					text += ")";
				}

				//find causal CNVs
				causal_ids = db.getValues("SELECT cnv_id FROM report_configuration_cnv WHERE  causal='0' AND exclude_artefact='0' AND exclude_frequency='0' AND exclude_phenotype='0' AND exclude_mechanism='0' AND exclude_other='0' AND report_configuration_id=" + rc_id.toString());
				foreach(QString id, causal_ids)
				{
					CopyNumberVariant var = db.cnv(id.toInt());
					QString cn = db.getValue("SELECT cn FROM cnv WHERE id='" + id + "'").toString();
					QString cnv_class = db.getValue("SELECT class FROM report_configuration_cnv WHERE cnv_id='" + id + "'", false).toString();
					text += ", CNV: " + var.toString() + " (cn:" + cn;
					if (cnv_class != "") text += " classification:" + cnv_class; // add classification, if exists
					text += ")";
				}

				//find causal SVs
				QStringList sv_id_columns = QStringList() << "sv_deletion_id" << "sv_duplication_id" << "sv_insertion_id" << "sv_inversion_id" << "sv_translocation_id";
				QList<StructuralVariantType> sv_types = {StructuralVariantType::DEL, StructuralVariantType::DUP, StructuralVariantType::INS, StructuralVariantType::INV, StructuralVariantType::BND};
				BedpeFile svs;
				for (int i = 0; i < sv_id_columns.size(); ++i)
				{
					causal_ids = db.getValues("SELECT " + sv_id_columns.at(i) + " FROM report_configuration_sv WHERE  causal='0' AND exclude_artefact='0' AND exclude_frequency='0' AND exclude_phenotype='0' AND exclude_mechanism='0' AND exclude_other='0' AND report_configuration_id=" + rc_id.toString() + " AND " + sv_id_columns.at(i) + " IS NOT NULL");

					foreach(QString id, causal_ids)
					{
						BedpeLine var = db.structuralVariant(id.toInt(), sv_types.at(i), svs, true);
						QString sv_class = db.getValue("SELECT class FROM report_configuration_sv WHERE " + sv_id_columns[i] + "='" + id + "'", false).toString();
						text += ", SV: " + var.toString();
						if (sv_class != "") text += " (classification:" + sv_class + ")"; // add classification, if exists
					}
				}
			}
			qDebug() << ps << "\t" << text;
		}
		*/

		//Export GenLab dates for reanalysis of unsolved samples
		/*
		TsvFile output;
		output.addHeader("ps");
		output.addHeader("yearOfBirth");
		output.addHeader("yearOfOrderEntry");
		GenLabDB db;
		TsvFile file;
		file.load("W:\\share\\evaluations\\2020_07_14_reanalysis_pediatric_cases\\samples.tsv");
		int i=0;
		QStringList ps_names = file.extractColumn(1);
		foreach(QString ps, ps_names)
		{
			qDebug() << ++i << "/" << ps_names.count() << ps;
			output.addRow(QStringList() << ps << db.yearOfBirth(ps) << db.yearOfOrderEntry(ps));
		}
		output.store("W:\\share\\evaluations\\2020_07_14_reanalysis_pediatric_cases\\+documentation\\genlab_export_dates_" + QDate::currentDate().toString("yyyy_MM_dd")+".tsv");
		*/

		//import preferred transcripts
		/*
		NGSD db;
		QString filename = GSvarHelper::applicationBaseName() + "_preferred_transcripts.tsv";
		QStringList lines = Helper::loadTextFile(filename, true, '#', true);
		foreach(const QString& line, lines)
		{
			QByteArrayList parts = line.toLatin1().replace(',', '\t').split('\t');
			if (parts.count()>=2)
			{
				QByteArray gene = parts[0].trimmed();
				for (int i=1; i<parts.count(); ++i)
				{
					QByteArray transcript = parts[i].trimmed();
					qDebug() << gene << transcript;
					try
					{
						qDebug() << "  success: " << db.addPreferredTranscript(transcript);
					}
					catch(Exception& e)
					{

						qDebug() << "  failed: " << e.message();
					}
				}
			}
		}
		*/

		//import sample meta data from GenLab
		GenLabDB genlab;
		NGSD db;
		ProcessedSampleSearchParameters params;
		params.p_type = "diagnostic";
		params.sys_type = "WES";
		params.include_bad_quality_samples = false;
		params.include_tumor_samples = false;
		params.include_merged_samples = false;
		params.include_bad_quality_runs = false;
		params.run_finished = true;
		DBTable ps_table = db.processedSampleSearch(params);
		QStringList ps_list = ps_table.extractColumn(0);
		int ps_start_index = -1;
		int i=0;
		foreach(QString ps, ps_list)
		{
			++i;
			if (i<ps_start_index) continue;

			qDebug() << i << "/" << ps_list.size() << " - " << ps;
			genlab.addMissingMetaDataToNGSD(ps, true, false, false, false, true);
		}

		qDebug() << Helper::elapsedTime(timer, true);
	}
	else if (user=="ahschul1")
	{
	}
	else if (user=="ahgscha1")
	{
	}
}

void MainWindow::on_actionConvertVcfToGSvar_triggered()
{
	VariantConversionWidget* widget = new VariantConversionWidget();
	widget->setMode(VariantConversionWidget::VCF_TO_GSVAR);
	auto dlg = GUIHelper::createDialog(widget, "Variant conversion (VCF > GSvar)");
	addModelessDialog(dlg);
}

void MainWindow::on_actionConvertHgvsToGSvar_triggered()
{
	VariantConversionWidget* widget = new VariantConversionWidget();
	widget->setMode(VariantConversionWidget::HGVSC_TO_GSVAR);
	auto dlg = GUIHelper::createDialog(widget, "Variant conversion (HGVS.c > GSvar)");
	addModelessDialog(dlg);
}

void MainWindow::on_actionConvertGSvarToVcf_triggered()
{
	VariantConversionWidget* widget = new VariantConversionWidget();
	widget->setMode(VariantConversionWidget::GSVAR_TO_VCF);
	auto dlg = GUIHelper::createDialog(widget, "Variant conversion (GSvar > VCF)");
	addModelessDialog(dlg);
}

void MainWindow::on_actionCytobandsToRegions_triggered()
{
	CytobandToRegionsDialog dlg(this);

	dlg.exec();
}

void MainWindow::on_actionSearchSNVs_triggered()
{
	SmallVariantSearchWidget* widget = new SmallVariantSearchWidget();
	connect(widget, SIGNAL(openVariantTab(Variant)), this, SLOT(openVariantTab(Variant)));
	auto dlg = GUIHelper::createDialog(widget, "Small variants search");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSearchCNVs_triggered()
{
	CnvSearchWidget* widget = new CnvSearchWidget();
	auto dlg = GUIHelper::createDialog(widget, "CNV search");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSearchSVs_triggered()
{
	SvSearchWidget* widget = new SvSearchWidget();
	auto dlg = GUIHelper::createDialog(widget, "SV search");
	addModelessDialog(dlg);
}

void MainWindow::on_actionShowPublishedVariants_triggered()
{
	PublishedVariantsWidget* widget = new PublishedVariantsWidget();

	auto dlg = GUIHelper::createDialog(widget, "Published variants");
	dlg->exec();
}

void MainWindow::on_actionAlleleBalance_triggered()
{
	AlleleBalanceCalculator* widget = new AlleleBalanceCalculator();
	auto dlg = GUIHelper::createDialog(widget, "Allele balance of heterzygous variants");
	dlg->exec();
}

void MainWindow::on_actionClose_triggered()
{
	loadFile();
}

void MainWindow::on_actionCloseMetaDataTabs_triggered()
{
	for (int t=ui_.tabs->count()-1; t>0; --t)
	{
		closeTab(t);
	}
}

void MainWindow::on_actionIgvClear_triggered()
{
	executeIGVCommands(QStringList() << "new");
	igv_initialized_ = false;
}

void MainWindow::on_actionIgvPort_triggered()
{
	bool ok = false;
	int igv_port_new = QInputDialog::getInt(this, "Change IGV port", "Set IGV port for this GSvar session:", igvPort(), 0, 900000, 1, &ok);
	if (ok)
	{
		igv_port_manual = igv_port_new;
	}
}

void MainWindow::on_actionIgvDocumentation_triggered()
{
	QDesktopServices::openUrl(QUrl("https://software.broadinstitute.org/software/igv/UserGuide"));
}

void MainWindow::on_actionSV_triggered()
{
	if(filename_ == "") return;

	if (!svs_.isValid())
	{
		QMessageBox::information(this, "SV file missing", "No structural variant file is present in the analysis folder!");
		return;
	}

	//create list of genes with heterozygous variant hits
	GeneSet het_hit_genes;
	int i_genes = variants_.annotationIndexByName("gene", true, false);
	QList<int> i_genotypes = variants_.getSampleHeader().sampleColumns(true);
	i_genotypes.removeAll(-1);

	if (i_genes!=-1 && i_genotypes.count()>0)
	{
		//check that a filter was applied (otherwise this can take forever)
		int passing_vars = filter_result_.countPassing();
		if (passing_vars>2000)
		{
			int res = QMessageBox::question(this, "Continue?", "There are " + QString::number(passing_vars) + " variants that pass the filters.\nGenerating the list of candidate genes for compound-heterozygous hits may take very long for this amount of variants.\nDo you want to continue?", QMessageBox::Yes, QMessageBox::No);
			if(res==QMessageBox::No) return;
		}
		for (int i=0; i<variants_.count(); ++i)
		{
			if (!filter_result_.passing(i)) continue;

			bool all_genos_het = true;
			foreach(int i_genotype, i_genotypes)
			{
				if (variants_[i].annotations()[i_genotype]!="het")
				{
					all_genos_het = false;
				}
			}
			if (!all_genos_het) continue;
			het_hit_genes.insert(GeneSet::createFromText(variants_[i].annotations()[i_genes], ','));
		}
	}
	else if (variants_.type()!=SOMATIC_PAIR && variants_.type() != SOMATIC_SINGLESAMPLE)
	{
		QMessageBox::information(this, "Invalid variant list", "Column for genes or genotypes not found in variant list. Cannot apply compound-heterozygous filter based on variants!");
	}

	try
	{
		//determine processed sample ID (needed for report config)
		QString ps_id = "";
		QSharedPointer<ReportConfiguration> report_config = nullptr;
		if (germlineReportSupported())
		{
			ps_id = NGSD().processedSampleId(germlineReportSample(), false);
			report_config = report_settings_.report_config;
		}

		//open SV widget
		SvWidget* list;
		if(svs_.isSomatic())
		{
			// somatic
			list = new SvWidget(svs_, ps_id, ui_.filters, het_hit_genes, gene2region_cache_, this);
		}
		else
		{
			// germline single, trio or multi sample
			list = new SvWidget(svs_, ps_id, ui_.filters, report_config, het_hit_genes, gene2region_cache_, this);
		}

		auto dlg = GUIHelper::createDialog(list, "Structural variants of " + variants_.analysisName());
		connect(list,SIGNAL(openInIGV(QString)),this,SLOT(openInIGV(QString)));
		connect(list,SIGNAL(openGeneTab(QString)),this,SLOT(openGeneTab(QString)));
		addModelessDialog(dlg);
	}
	catch(FileParseException error)
	{
		QMessageBox::warning(this,"File Parse Exception",error.message());
	}
	catch(FileAccessException error)
	{
		QMessageBox::warning(this,"SV file not found",error.message());
	}
}

void MainWindow::on_actionCNV_triggered()
{
	if (filename_=="") return;

	if (!cnvs_.isValid())
	{
		QMessageBox::information(this, "CNV file missing", "No copy-number file is present in the analysis folder!");
		return;
	}

	AnalysisType type = variants_.type();

	//create list of genes with heterozygous variant hits
	GeneSet het_hit_genes;
	int i_genes = variants_.annotationIndexByName("gene", true, false);
	QList<int> i_genotypes = variants_.getSampleHeader().sampleColumns(true);
	i_genotypes.removeAll(-1);

	if (i_genes!=-1 && i_genotypes.count()>0)
	{
		//check that a filter was applied (otherwise this can take forever)
		int passing_vars = filter_result_.countPassing();
		if (passing_vars>2000)
		{
			int res = QMessageBox::question(this, "Continue?", "There are " + QString::number(passing_vars) + " variants that pass the filters.\nGenerating the list of candidate genes for compound-heterozygous hits may take very long for this amount of variants.\nPlease set a filter for the variant list, e.g. the recessive filter, and retry!\nDo you want to continue?", QMessageBox::Yes, QMessageBox::No);
			if(res==QMessageBox::No) return;
		}
		for (int i=0; i<variants_.count(); ++i)
		{
			if (!filter_result_.passing(i)) continue;

			bool all_genos_het = true;
			foreach(int i_genotype, i_genotypes)
			{
				if (variants_[i].annotations()[i_genotype]!="het")
				{
					all_genos_het = false;
				}
			}
			if (!all_genos_het) continue;
			het_hit_genes.insert(GeneSet::createFromText(variants_[i].annotations()[i_genes], ','));
		}
	}
	else if (type!=SOMATIC_PAIR && type!=SOMATIC_SINGLESAMPLE)
	{
		QMessageBox::information(this, "Invalid variant list", "Column for genes or genotypes not found in variant list. Cannot apply compound-heterozygous filter based on variants!");
	}

	//determine processed sample ID (needed for report config)
	QString ps_id = "";
	if (germlineReportSupported())
	{
		ps_id = NGSD().processedSampleId(germlineReportSample(), false);
	}

	CnvWidget *list;
	if(cnvs_.type() == CnvListType::CLINCNV_TUMOR_NORMAL_PAIR || cnvs_.type() == CnvListType::CLINCNV_TUMOR_ONLY)
	{
		list = new CnvWidget(cnvs_, ps_id, ui_.filters, somatic_report_settings_.report_config, het_hit_genes, gene2region_cache_);
	}
	else
	{
		list = new CnvWidget(cnvs_, ps_id, ui_.filters, report_settings_.report_config, het_hit_genes, gene2region_cache_);
	}

	connect(list, SIGNAL(openRegionInIGV(QString)), this, SLOT(openInIGV(QString)));
	connect(list, SIGNAL(openGeneTab(QString)), this, SLOT(openGeneTab(QString)));
	connect(list, SIGNAL(storeSomaticReportConfiguration()), this, SLOT(storeSomaticReportConfig()));
	auto dlg = GUIHelper::createDialog(list, "Copy number variants of " + variants_.analysisName());
	addModelessDialog(dlg, true);

	//mosaic CNVs
	if (type==GERMLINE_SINGLESAMPLE)
	{
		FileLocation mosaic_file = GlobalServiceProvider::fileLocationProvider().getAnalysisMosaicCnvFile();
		if (mosaic_file.exists)
		{
			QStringList mosaic_data = Helper::loadTextFile(mosaic_file.filename, false, '#', true);
			if (!mosaic_data.isEmpty())
			{
				QPlainTextEdit* text_edit = new QPlainTextEdit(this);
				text_edit->setReadOnly(true);
				text_edit->setMinimumSize(450, 100);
				text_edit->appendPlainText("#CHR\tSTART\tEND\tCOPY NUMBER");

				foreach (const QString& line, mosaic_data)
				{
					if(line.trimmed().isEmpty() || line.startsWith("#")) continue;

					QStringList parts = line.split("\t");
					if(parts.length()<4)
					{
						Log::warn("Mosaic CNV file '" + mosaic_file.filename + "' has line with less than 4 elements: " + line);
					}
					else
					{
						text_edit->appendPlainText(parts.mid(0, 4).join("\t"));
					}
				}
				auto dlg = GUIHelper::createDialog(text_edit, "Possible mosaic CNV(s) detected!");
				dlg->exec();
			}
		}
	}
}

void MainWindow::on_actionROH_triggered()
{
	if (filename_=="") return;

	AnalysisType type = variants_.type();
	if (type!=GERMLINE_SINGLESAMPLE && type!=GERMLINE_TRIO && type!=GERMLINE_MULTISAMPLE) return;

	//trio special handling: show UPD file is not empty
	if (type==GERMLINE_TRIO)
	{		
		//UPDs
		FileLocation upd_loc = GlobalServiceProvider::fileLocationProvider().getAnalysisUpdFile();
		if (!upd_loc.exists)
		{
			QMessageBox::warning(this, "UPD detection", "The UPD file is missing!\n" + upd_loc.filename);
		}
		else
		{
			QStringList upd_data = Helper::loadTextFile(upd_loc.filename, false, QChar::Null, true);
			if (upd_data.count()>1)
			{
				QPlainTextEdit* text_edit = new QPlainTextEdit(this);
				text_edit->setReadOnly(true);
				QStringList headers = upd_data[0].split("\t");
				for (int r=1; r<upd_data.count(); ++r)
				{
					QStringList parts = upd_data[r].split("\t");
					QString line = parts[0] + ":" + parts[1] + "-" + parts[2];
					for(int c=3 ; c<parts.count(); ++c)
					{
						line += " " + headers[c] + "=" + parts[c];
					}
					text_edit->appendPlainText(line);
				}
				text_edit->setMinimumSize(800, 100);
				auto dlg = GUIHelper::createDialog(text_edit, "UPD(s) detected!");
				dlg->exec();
			}
		}
	}

	//check report sample ROH file exists
	QStringList roh_files = GlobalServiceProvider::fileLocationProvider().getRohFiles(false).filterById(germlineReportSample()).asStringList();
	if (roh_files.isEmpty())
	{
		QMessageBox::warning(this, "Runs of homozygosity", "Could not find a ROH file for sample " + germlineReportSample() + ". Aborting!");
		return;
	}

	RohWidget* list = new RohWidget(roh_files[0], ui_.filters);
	connect(list, SIGNAL(openRegionInIGV(QString)), this, SLOT(openInIGV(QString)));
	auto dlg = GUIHelper::createDialog(list, "Runs of homozygosity of " + variants_.analysisName());
	addModelessDialog(dlg);
}

void MainWindow::on_actionGeneSelector_triggered()
{
	if (filename_=="") return;
	AnalysisType type = variants_.type();
	if (type!=GERMLINE_SINGLESAMPLE && type!=GERMLINE_TRIO && type!=GERMLINE_MULTISAMPLE) return;

	QString ps_name = germlineReportSample();

	//show dialog	
	GeneSelectorDialog dlg(ps_name, this);
	connect(&dlg, SIGNAL(openRegionInIGV(QString)), this, SLOT(openInIGV(QString)));
	if (dlg.exec())
	{
		//copy report to clipboard
		QString report = dlg.report();
		QApplication::clipboard()->setText(report);

		//show message
		if (QMessageBox::question(this, "Gene selection report", "Gene selection report was copied to clipboard.\nDo you want to open the sub-panel design dialog for selected genes?")==QMessageBox::Yes)
		{
			openSubpanelDesignDialog(dlg.genesForVariants());
		}
	}
}

void MainWindow::on_actionCircos_triggered()
{
	if (filename_=="") return;

	//load plot file
	QList<FileLocation> plot_files = GlobalServiceProvider::fileLocationProvider().getCircosPlotFiles(false);
	if (plot_files.isEmpty()) return; //this should not happen because the button is not enabled then...

	//show plot
	CircosPlotWidget* widget = new CircosPlotWidget(plot_files[0].filename);
	auto dlg = GUIHelper::createDialog(widget, "Circos Plot of " + variants_.analysisName());
	addModelessDialog(dlg, false);
}

void MainWindow::on_actionExpressionData_triggered()
{
	if (filename_=="") return;
	if (!LoginManager::active()) return;

	QString title = "Expression data";

	NGSD db;
	QString sample_id = db.sampleId(filename_, false);
	if (sample_id=="")
	{
		QMessageBox::warning(this, title, "Error: Sample not found in NGSD!");
		return;
	}

	//get count files of all RNA processed samples corresponding to the current sample
	QStringList rna_ps_ids;
	foreach (QString rna_sample, db.sameSamples(sample_id, "RNA"))
	{
		rna_ps_ids << db.getValues("SELECT id FROM processed_sample WHERE sample_id=:0", rna_sample);
	}

	QStringList rna_count_files;
	foreach (QString rna_ps_id, rna_ps_ids)
	{
		QString file = db.processedSamplePath(rna_ps_id, PathType::COUNTS);
		if (QFile::exists(file)) rna_count_files << file;
	}
	rna_count_files.removeDuplicates();

	if (rna_count_files.isEmpty())
	{
		QMessageBox::warning(this, title, "Error: No RNA count files of corresponding RNA samples found!");
		return;
	}

	//select file to open
	QString count_file;
	if (rna_count_files.size()==1)
	{
		count_file = rna_count_files.at(0);
	}
	else
	{
		bool ok;
		count_file = QInputDialog::getItem(this, title, "Multiple RNA count files found.\nPlease select a file:", rna_count_files, 0, false, &ok);
		if (!ok) return;
	}

	ExpressionDataWidget* widget = new ExpressionDataWidget(count_file, this);
	auto dlg = GUIHelper::createDialog(widget, "Expression Data");
	addModelessDialog(dlg, false);
}

void MainWindow::on_actionRE_triggered()
{
	if (filename_=="") return;
	if (variants_.type()!=GERMLINE_SINGLESAMPLE) return;

	// determine repeat expansion file name	
	FileLocationList re_files = GlobalServiceProvider::fileLocationProvider().getRepeatExpansionFiles(false);
	if (re_files.isEmpty()) return; //this should not happen because the button is not enabled then...

	QString ps_name = variants_.mainSampleName();

	//get sample type
	bool is_exome = false;
	if (LoginManager::active())
	{
		NGSD db;
		QString ps_id = db.processedSampleId(ps_name, false);
		is_exome = ps_id!="" && db.getProcessedSampleData(ps_id).processing_system_type=="WES";
	}

	//show dialog
	RepeatExpansionWidget* widget = new RepeatExpansionWidget(re_files[0].filename, is_exome);
	auto dlg = GUIHelper::createDialog(widget, "Repeat Expansions of " + variants_.analysisName());

	addModelessDialog(dlg, false);
}

void MainWindow::on_actionPRS_triggered()
{
	if (filename_=="") return;

	// determine PRS file name
	FileLocationList prs_files = GlobalServiceProvider::fileLocationProvider().getPrsFiles(false);
	if (prs_files.isEmpty()) return; //this should not happen because the button is not enabled then...

	//show dialog
	PRSWidget* widget = new PRSWidget(prs_files[0].filename);
	auto dlg = GUIHelper::createDialog(widget, "Polygenic Risk Scores of " + variants_.analysisName());
	addModelessDialog(dlg, false);
}

void MainWindow::on_actionDesignCfDNAPanel_triggered()
{
	if (filename_=="") return;
	if (!LoginManager::active()) return;
	if (!somaticReportSupported()) return;

	DBTable cfdna_processing_systems = NGSD().createTable("processing_system", "SELECT id, name_short FROM processing_system WHERE type='cfDNA (patient-specific)'");

	QSharedPointer<CfDNAPanelDesignDialog> dialog = QSharedPointer<CfDNAPanelDesignDialog>(new CfDNAPanelDesignDialog(variants_, filter_result_, somatic_report_settings_.report_config,
																													  variants_.mainSampleName(),
																													  cfdna_processing_systems, this));
	dialog->setWindowFlags(Qt::Window);

	// link IGV
	connect(&*dialog,SIGNAL(openInIGV(QString)),this,SLOT(openInIGV(QString)));

	addModelessDialog(dialog, false);
}

void MainWindow::on_actionShowCfDNAPanel_triggered()
{
	if (filename_=="") return;
	if (!LoginManager::active()) return;
	if (!somaticReportSupported()) return;

	// get cfDNA panels:
	QList<CfdnaPanelInfo> cfdna_panels = NGSD().cfdnaPanelInfo(NGSD().processedSampleId(variants_.mainSampleName()));
	CfdnaPanelInfo selected_panel;
	if (cfdna_panels.size() < 1)
	{
		// show message
		GUIHelper::showMessage("No cfDNA panel found!", "No cfDNA panel was found for the given tumor sample!");
		return;
	}
	else if (cfdna_panels.size() > 1)
	{
		QStringList cfdna_panel_description;
		foreach (const CfdnaPanelInfo& panel, cfdna_panels)
		{
			cfdna_panel_description.append("cfDNA panel for " + panel.processing_system  + " (" + panel.created_date.toString("dd.MM.yyyy") + " by " + panel.created_by + ")");
		}

		QComboBox* cfdna_panel_selector = new QComboBox(this);
		cfdna_panel_selector->addItems(cfdna_panel_description);

		// create dlg
		auto dlg = GUIHelper::createDialog(cfdna_panel_selector, "Select cfDNA panel", "", true);
		int btn = dlg->exec();
		if (btn!=1)
		{
			return;
		}
		selected_panel = cfdna_panels.at(cfdna_panel_selector->currentIndex());
	}
	else
	{
		selected_panel = cfdna_panels.at(0);
	}

	//show dialog	
	CfDNAPanelWidget* widget = new CfDNAPanelWidget(selected_panel);
	auto dlg = GUIHelper::createDialog(widget, "cfDNA panel for tumor " + variants_.analysisName());
	addModelessDialog(dlg, false);
}

void MainWindow::on_actionCfDNADiseaseCourse_triggered()
{
	if (filename_=="") return;
	if (!somaticReportSupported()) return;

	DiseaseCourseWidget* widget = new DiseaseCourseWidget(variants_.mainSampleName());
	auto dlg = GUIHelper::createDialog(widget, "Personalized cfDNA variants");

	// link IGV
	connect(widget,SIGNAL(openInIGV(QString)),this,SLOT(openInIGV(QString)));
	connect(widget,SIGNAL(executeIGVCommands(QStringList)),this,SLOT(executeIGVCommands(QStringList)));

	addModelessDialog(dlg, false);
}

void MainWindow::on_actionGeneOmimInfo_triggered()
{
	GeneOmimInfoWidget* widget = new GeneOmimInfoWidget(this);
	auto dlg = GUIHelper::createDialog(widget, "OMIM information for genes");
	dlg->exec();
}

void MainWindow::openVariantListFolder()
{
	if (filename_=="") return;

	if (!GlobalServiceProvider::fileLocationProvider().isLocal())
	{
		QMessageBox::information(this, "Open analysis folder", "Cannot open analysis folder in client-server mode!");
		return;
	}

	QDesktopServices::openUrl(QFileInfo(filename_).absolutePath());
}

void MainWindow::on_actionPublishVariantInLOVD_triggered()
{
	LovdUploadDialog dlg(this);
	dlg.exec();
}

void MainWindow::on_actionBatchExportClinVar_triggered()
{
	//allow only for admins
	try
	{
		LoginManager::checkRoleIn(QStringList() << "admin");
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Permissions error", e.message());
		return;
	}

	//show general infos
	QMessageBox::information(this, "Batch import of variants into ClinVar", "This function extracts variants from the NGSD for a batch import into ClinVar.\n\n"
																			"You have to prepare a ClinVar submission at https://submit.ncbi.nlm.nih.gov/clinvar/ and provide the submission identifier.\n"
																			"An XML file with variants is then created which has to be uploaded to ClinVar.\n");
	//get data from user
	QString submitter_id = QInputDialog::getText(this, "Set ClinVar information", "submitter id:", QLineEdit::Normal, "5949").trimmed();
	if (submitter_id=="") return;
	QString organization_id = QInputDialog::getText(this, "Set ClinVar information", "organization id:", QLineEdit::Normal, "506385").trimmed(); //https://www.ncbi.nlm.nih.gov/clinvar/submitters/506385/
	if (organization_id=="") return;
	QString submission_id = QInputDialog::getText(this, "Set ClinVar information", "submission id ('TEST' for dry run):", QLineEdit::Normal, "TEST").trimmed();
	if (submission_id=="") return;
	QString out_file = QFileDialog::getSaveFileName(this, "Select output file name", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), "*.xml").trimmed();
	if (out_file.isEmpty()) return;

	//init
	QString ref_file = Settings::string("reference_genome");
	FastaFileIndex genome_index(ref_file);

	QStringList messages;
	QStringList variant_publication_queries;

	NGSD db;
	SqlQuery query_already_published = db.getQuery();
	query_already_published.prepare("SELECT class FROM variant_publication WHERE db='ClinVar' AND variant_id=:0");

	try
	{
		//get variants that should be published
		QSet<QString> variant_ids_done;
		SqlQuery query = db.getQuery();
		query.exec("SELECT ps.sample_id, rcv.variant_id, rcv.inheritance, vc.class, rcv.id"
				   " FROM processed_sample ps, diag_status ds, project p, report_configuration rc, report_configuration_variant rcv, variant_classification vc"
				   " WHERE ps.project_id=p.id AND ps.id=ds.processed_sample_id AND ps.id=rc.processed_sample_id AND rc.id=rcv.report_configuration_id AND rcv.variant_id = vc.variant_id AND ds.outcome='significant findings' AND rcv.causal='1' AND (vc.class='4' OR vc.class='5') AND p.type='diagnostic'");
		messages << ("Causal class 4/5 variants in diagnostic samples with outcome 'significant findings': " + QString::number(query.size()));

		//merge XML files of variants to publish to one XML file
		QStringList output;
		output << "<?xml version=\"1.0\"?>";
		output << "<ClinvarSubmissionSet sub_id=\"" + submission_id + "\" Date=\"" + QDate::currentDate().toString(Qt::ISODate) + "\">";
		output << "  <SubmitterOfRecordID>" + submitter_id + "</SubmitterOfRecordID>";
		output << "  <OrgID Type=\"primary\">" + organization_id + "</OrgID>";
		while (query.next())
		{
			QString sample_id = query.value(0).toString();
			QString variant_id = query.value(1).toString();
			QString inheritance = query.value(2).toString();
			QString classification = query.value(3).toString();
			QString rcv_id = query.value(4).toString();

			//check if variant is already uploaded to ClinVar
			query_already_published.bindValue(0, variant_id);
			query_already_published.exec();
			if (query_already_published.next()) continue;

			//check if the variant is already exported in the current batch
			if (variant_ids_done.contains(variant_id)) continue;
			variant_ids_done << variant_id;

			//variant
			Variant variant = db.variant(variant_id);
			if(variant.ref() == "-") //for insertions add +1 to end (Clinvar format)
			{
				variant.setEnd(variant.end()+1);
			}
			//small additional check for end or variant position (end is not calculated in toVCF - however should only be affected for insertions due to ClinVar format)
			int end_check = variant.start() + variant.ref().length() - 1;
			if(variant.ref() == "-")
			{
				end_check +=1;
			}
			if(variant.end() != end_check)
			{
				THROW(ProgrammingException, "Error in end of variant '" + variant.toString(true) + "' - end check is '" + QString::number(end_check));
			}

			//inheritance
			QString variant_inheritance = ClinvarSubmissionGenerator::translateInheritance(inheritance);
			if(variant_inheritance=="")
			{
				if(inheritance == "AR+AD")
				{
					variant_inheritance = "Autosomal unknown";
				}
				else if(inheritance == "XLR+XLD")
				{
					variant_inheritance = "X-linked inheritance";
				}
				else if(inheritance == "n/a")
				{
					variant_inheritance = "Unknown mechanism";
				}
			}

			//classification
			QString variant_classification = ClinvarSubmissionGenerator::translateClassification(classification);

			//General Data
			ClinvarSubmissionData data;
			data.date = QDate::currentDate();
			data.local_key = "report_configuration_variant_id:" + rcv_id;

			data.submission_id = submission_id;
			data.submitter_id = submitter_id;
			data.organization_id = organization_id;

			data.variant = variant.toVCF(genome_index);;
			data.variant_classification = variant_classification;
			data.variant_inheritance = variant_inheritance;

			SampleData sample_data = db.getSampleData(sample_id);
			data.sample_name = sample_data.name;
			data.sample_phenotypes = sample_data.phenotypes;
			if(sample_data.gender!="n/a")
			{
				data.sample_gender = sample_data.gender;
			}

			//add XML to output
			QStringList xml = ClinvarSubmissionGenerator::generateXML(data).split("\n");
			bool in_submission = false;
			foreach(const QString& line, xml)
			{
				if (!in_submission && line.contains("<ClinvarSubmission>"))
				{
					in_submission = true;
				}

				if (in_submission)
				{
					output << line;
					if (line.contains("</ClinvarSubmission>"))
					{
						in_submission = false;
					}
				}
			}

			//update NGSD
			QString gene = "gene="+db.getValue("SELECT gene FROM variant WHERE id='" + variant_id + "'").toString();
			variant_publication_queries << "INSERT INTO `variant_publication` (`sample_id`, `variant_id`, `db`, `class`, `details`, `user_id`) VALUES ('"+sample_id+"','"+variant_id+"','ClinVar','"+classification+"','gene="+gene+"',"+LoginManager::userIdAsString()+")";
		}
		messages << ("Exported variants to file: " + QString::number(variant_ids_done.count()));

		//store output
		output << "</ClinvarSubmissionSet>";
		Helper::storeTextFile(out_file, output);
		QString xml_error = XmlHelper::isValidXml(out_file);
		if (xml_error!="")
		{
			THROW(ProgrammingException, "An invalid XML file was produced: " + xml_error);
		}
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Batch import of variants into ClinVar", "An error occured:\n" + e.message());
		return;
	}

	//submit changes to NGSD
	if (submission_id!="TEST")
	{
		foreach(QString query, variant_publication_queries)
		{
			db.getQuery().exec(query);
		}
	}

	//show messages
	QMessageBox::information(this, "Batch import of variants into ClinVar", messages.join("\n"));
}

void MainWindow::on_actionReanalyze_triggered()
{
	if (filename_=="") return;

	AnalysisType type = variants_.type();
	SampleHeaderInfo header_info = variants_.getSampleHeader();

	QList<AnalysisJobSample> samples;
	if (type==GERMLINE_SINGLESAMPLE)
	{
		SingleSampleAnalysisDialog dlg(this);
		samples << AnalysisJobSample {header_info[0].id, ""};
		dlg.setSamples(samples);
		if (dlg.exec()==QDialog::Accepted)
		{
			foreach(const AnalysisJobSample& sample,  dlg.samples())
			{
				NGSD().queueAnalysis("single sample", dlg.highPriority(), dlg.arguments(), QList<AnalysisJobSample>() << sample);
			}
		}
	}
	else if (type==GERMLINE_MULTISAMPLE)
	{
		MultiSampleDialog dlg(this);
		foreach(const SampleInfo& info, header_info)
		{
			samples << AnalysisJobSample {info.id, info.isAffected() ? "affected" : "control"};
		}
		dlg.setSamples(samples);
		if (dlg.exec()==QDialog::Accepted)
		{
			NGSD().queueAnalysis("multi sample", dlg.highPriority(), dlg.arguments(), dlg.samples());
		}
	}
	else if (type==GERMLINE_TRIO)
	{
		TrioDialog dlg(this);
		foreach(const SampleInfo& info, header_info)
		{
			if(info.isAffected())
			{
				samples << AnalysisJobSample {info.id, "child"};
			}
			else
			{
				samples << AnalysisJobSample {info.id, info.gender()=="male" ? "father" : "mother"};
			}
		}
		dlg.setSamples(samples);
		if (dlg.exec()==QDialog::Accepted)
		{
			NGSD().queueAnalysis("trio", dlg.highPriority(), dlg.arguments(), dlg.samples());
		}
	}
	else if (type==SOMATIC_PAIR)
	{
		SomaticDialog dlg(this);
		foreach(const SampleInfo& info, header_info)
		{
			samples << AnalysisJobSample {info.id, info.isTumor() ? "tumor" : "normal"};
		}
		dlg.setSamples(samples);

		if (dlg.exec()==QDialog::Accepted)
		{
			NGSD().queueAnalysis("somatic", dlg.highPriority(), dlg.arguments(), dlg.samples());
		}
	}
}

void MainWindow::delayedInitialization()
{
	//initialize LOG file
	Log::setFileEnabled(true);
	Log::appInfo();

	//load from INI file (if a valid INI file - otherwise restore INI file)
	if (!Settings::contains("igv_genome"))
	{
		QMessageBox::warning(this, "GSvar not configured", "GSvar is not configured correctly.\nThe settings key 'igv_genome' is not set.\nPlease inform your administrator!");
		close();
		return;
	}

	//user login for database
	if (GlobalServiceProvider::database().enabled())
	{
		LoginDialog dlg(this);
		if (dlg.exec()==QDialog::Accepted)
		{
			LoginManager::login(dlg.userName());
		}
	}

	//init GUI
	updateRecentFilesMenu();
	updateIGVMenu();
	updateNGSDSupport();

	//parse arguments
	for (int i=1; i<QApplication::arguments().count(); ++i)
	{
		QString arg = QApplication::arguments().at(i);

		if (i==1) //first argument: sample
		{
			if (QFile::exists(arg)) //file path
			{
				loadFile(arg);
			}
			else if (LoginManager::active()) //processed sample name (via NGSD)
			{
				NGSD db;
				if (db.processedSampleId(arg, false)!="")
				{
					openProcessedSampleFromNGSD(arg, false);
				}
				else if (db.sampleId(arg, false)!="")
				{
					openSampleFromNGSD(arg);
				}
			}
		}
		else if (arg.startsWith("filter:")) //filter (by name)
		{
			int sep_pos = arg.indexOf(':');
			QString filter_name = arg.mid(sep_pos+1).trimmed();

			if (!ui_.filters->setFilter(filter_name))
			{
				qDebug() << "Filter name " << arg << " not found. Ignoring it!";
			}
		}
		else if (arg.startsWith("roi:")) //target region (by name)
		{
			int sep_pos = arg.indexOf(':');
			QString roi_name = arg.mid(sep_pos+1).trimmed();

			if (!ui_.filters->setTargetRegionByDisplayName(roi_name))
			{
				qDebug() << "Target region name " << roi_name << " not found. Ignoring it!";
			}
		}
		else
		{
			qDebug() << "Unprocessed argument: " << arg;
		}
	}
}

void MainWindow::variantCellDoubleClicked(int row, int /*col*/)
{
	const Variant& v = variants_[ui_.vars->rowToVariantIndex(row)];
	openInIGV(v.chr().str() + ":" + QString::number(v.start()) + "-" + QString::number(v.end()));
}

void MainWindow::variantHeaderDoubleClicked(int row)
{
	if (!LoginManager::active()) return;

	int var_index = ui_.vars->rowToVariantIndex(row);
	editVariantReportConfiguration(var_index);
}

bool MainWindow::initializeIGV(QAbstractSocket& socket)
{
	AnalysisType analysis_type = variants_.type();

	IgvDialog dlg(this);

	//sample BAM file(s)
	FileLocationList bams = GlobalServiceProvider::fileLocationProvider().getBamFiles(true);
	foreach(const FileLocation& file, bams)
	{
		dlg.addFile(file, true);
	}

	//sample BAF file(s)
	FileLocationList bafs = GlobalServiceProvider::fileLocationProvider().getBafFiles(true);
	foreach(const FileLocation& file, bafs)
	{
		if(analysis_type == SOMATIC_PAIR && !file.id.contains("somatic")) continue;
		dlg.addFile(file, true);
	}

	//analysis VCF
	FileLocation vcf = GlobalServiceProvider::fileLocationProvider().getAnalysisVcf();
	dlg.addFile(vcf, ui_.actionIgvSample->isChecked());

	//analysis SV file
	FileLocation bedpe = GlobalServiceProvider::fileLocationProvider().getAnalysisSvFile();
	dlg.addFile(bedpe, ui_.actionSampleBedpe->isChecked());

	//CNV files
	if (analysis_type==SOMATIC_SINGLESAMPLE || analysis_type==SOMATIC_PAIR)
	{
		FileLocation file = GlobalServiceProvider::fileLocationProvider().getSomaticCnvCoverageFile();
		dlg.addFile(file, true);

		FileLocation file2 = GlobalServiceProvider::fileLocationProvider().getSomaticCnvCallFile();
		dlg.addFile(file2, true);
	}
	else
	{
		FileLocationList segs = GlobalServiceProvider::fileLocationProvider().getCnvCoverageFiles(true);
		foreach(const FileLocation& file, segs)
		{
			dlg.addFile(file, true);
		}
	}

	//Manta evidence file(s)
	FileLocationList evidence_files = GlobalServiceProvider::fileLocationProvider().getMantaEvidenceFiles(true);
	foreach(const FileLocation& file, evidence_files)
	{
		dlg.addFile(file, false);
	}

	//target region
	if (ui_.filters->targetRegion().isValid())
	{
		QString roi_file = GSvarHelper::localRoiFolder() + ui_.filters->targetRegion().name + ".bed";
		ui_.filters->targetRegion().regions.store(roi_file);

		dlg.addFile(FileLocation{"target region (selected in GSvar)", PathType::OTHER, roi_file, true}, true);
	}

	//amplicon file (of processing system)
	try
	{
		NGSD db;
		int sys_id = db.processingSystemIdFromProcessedSample(germlineReportSample());
		BedFile ampilicons = GlobalServiceProvider::database().processingSystemAmplicons(sys_id);
		if (!ampilicons.isEmpty())
		{
			QString amp_file = GSvarHelper::localRoiFolder() + db.getProcessingSystemData(sys_id).name_short + "_amplicons.bed";
			ampilicons.store(amp_file);

			dlg.addFile(FileLocation{"amplicons track (of processing system)", PathType::OTHER, amp_file, true}, true);
		}
	}
	catch(...) {} //Nothing to do here

	//sample low-coverage
	if (analysis_type==SOMATIC_SINGLESAMPLE || analysis_type==SOMATIC_PAIR)
	{
		FileLocation loc = GlobalServiceProvider::fileLocationProvider().getSomaticLowCoverageFile();
		dlg.addFile(loc, ui_.actionIgvLowcov->isChecked());
	}
	else
	{
		FileLocationList low_cov_files = GlobalServiceProvider::fileLocationProvider().getLowCoverageFiles(true);
		foreach(const FileLocation& file, low_cov_files)
		{
			dlg.addFile(file, ui_.actionIgvLowcov->isChecked());
		}
	}

	//custom tracks
	QList<QAction*> igv_actions = ui_.menuTrackDefaults->findChildren<QAction*>();
	foreach(QAction* action, igv_actions)
	{
		QString text = action->text();
		if (!text.startsWith("custom track:")) continue;

		QString filename = action->toolTip().replace("custom track:", "").trimmed();
		dlg.addFile(FileLocation{text, PathType::OTHER, filename, QFileInfo(filename).exists()}, action->isChecked());
	}

	// switch to MainWindow to prevent dialog to appear behind other widgets
	raise();
	activateWindow();
	setFocus();

	//execute dialog
	if (!dlg.exec()) return false;

	QApplication::setOverrideCursor(Qt::BusyCursor);
	try
	{
		if (dlg.initializationAction()==IgvDialog::INIT)
		{
			QStringList files_to_load = dlg.filesToLoad();
			QStringList init_commands;
			init_commands.append("new");
			init_commands.append("genome " + Settings::path("igv_genome"));

			//load non-BAM files
			foreach(QString file, files_to_load)
			{
				if (!file.endsWith(".bam"))
				{
					init_commands.append("load \"" + Helper::canonicalPath(file) + "\"");
				}
			}

			//collapse tracks
			init_commands.append("collapse");

			//load BAM files
			foreach(QString file, files_to_load)
			{
				if (file.endsWith(".bam"))
				{
					init_commands.append("load \"" + Helper::canonicalPath(file) + "\"");
				}
			}

			//execute commands
			bool debug = false;
			foreach(QString command, init_commands)
			{
				if (debug) qDebug() << QDateTime::currentDateTime() << "EXECUTING:" << command;
				socket.write((command + "\n").toLatin1());
				bool ok = socket.waitForReadyRead(180000); // 3 min timeout (trios can be slow)
				QString answer = socket.readAll().trimmed();
				if (!ok || answer!="OK")
				{
					if (debug) qDebug() << QDateTime::currentDateTime() << "FAILED: answer:" << answer << " socket error:" << socket.errorString();
					THROW(Exception, "Could not execute IGV command '" + command + "'.\nAnswer: " + answer + "\nSocket error:" + socket.errorString());
				}
				else
				{
					if (debug) qDebug() << QDateTime::currentDateTime() << "DONE";
				}
			}

			igv_initialized_ = true;
		}
		else if (dlg.initializationAction()==IgvDialog::SKIP_SESSION)
		{
			igv_initialized_ = true;
		}
		else if (dlg.initializationAction()==IgvDialog::SKIP_ONCE)
		{
			//nothing to do there
		}

		QApplication::restoreOverrideCursor();

		return true;
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();

		QMessageBox::warning(this, "Error while initializing IGV", e.message());

		return false;
	}
}

void MainWindow::openInIGV(QString region)
{
	executeIGVCommands(QStringList() << "goto " + region, true);
}

void MainWindow::openCustomIgvTrack()
{
	QAction* action = qobject_cast<QAction*>(sender());
	if (action==nullptr) return;

	QString name = action->text();

	QStringList entries = Settings::stringList("igv_menu");
	foreach(QString entry, entries)
	{
		QStringList parts = entry.trimmed().split("\t");
		if(parts[0]==name)
		{
			executeIGVCommands(QStringList() << "load \"" + Helper::canonicalPath(parts[2]) + "\"");
		}
	}
}

void MainWindow::editVariantValidation(int index)
{
	Variant& variant = variants_[index];

	try
	{
		QString ps = variants_.mainSampleName();
		NGSD db;

		//get variant ID - add if missing
		QString variant_id = db.variantId(variant, false);
		if (variant_id=="")
		{
			variant_id = db.addVariant(variant, variants_);
		}

		//get sample ID		
		QString sample_id = db.sampleId(ps);

		//get variant validation ID - add if missing
		QVariant val_id = db.getValue("SELECT id FROM variant_validation WHERE variant_id='" + variant_id + "' AND sample_id='" + sample_id + "'", true);
		bool added_validation_entry = false;
		if (!val_id.isValid())
		{
			//get genotype			
			int i_genotype = variants_.getSampleHeader().infoByID(ps).column_index;
			QByteArray genotype = variant.annotations()[i_genotype];

			//insert
			SqlQuery query = db.getQuery();
			query.exec("INSERT INTO variant_validation (user_id, sample_id, variant_type, variant_id, genotype, status) VALUES ('" + LoginManager::userIdAsString() + "','" + sample_id + "','SNV_INDEL','" + variant_id + "','" + genotype + "','n/a')");
			val_id = query.lastInsertId();

			added_validation_entry = true;
		}

		ValidationDialog dlg(this, val_id.toInt());

		if (dlg.exec())
		{
			//update DB
			dlg.store();

			//update variant table
			QByteArray status = dlg.status().toLatin1();
			if (status=="true positive") status = "TP";
			if (status=="false positive") status = "FP";
			int i_validation = variants_.annotationIndexByName("validation", true, true);
			variant.annotations()[i_validation] = status;

			//update details widget and filtering
			ui_.variant_details->updateVariant(variants_, index);
			refreshVariantTable();

			//mark variant list as changed
			markVariantListChanged(variant, "validation", status);
		}
		else if (added_validation_entry)
		{
			// remove created but empty validation if ValidationDialog is aborted
			SqlQuery query = db.getQuery();
			query.exec("DELETE FROM variant_validation WHERE id=" + val_id.toString());
		}
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
		return;
	}
}

void MainWindow::editVariantComment(int index)
{
	Variant& variant = variants_[index];

	try
	{
		//add variant if missing
		NGSD db;
		if (db.variantId(variant, false)=="")
		{
			db.addVariant(variant, variants_);
		}

		bool ok = true;
		QByteArray text = QInputDialog::getMultiLineText(this, "Variant comment", "Text: ", db.comment(variant), &ok).toLatin1();

		if (ok)
		{
			//update DB
			db.setComment(variant, text);

			//update datastructure (if comment column is present)
			int col_index = variants_.annotationIndexByName("comment", true, false);
			if (col_index!=-1)
			{
				variant.annotations()[col_index] = text;
				refreshVariantTable();

				//mark variant list as changed
				markVariantListChanged(variant, "comment", text);
			}
		}
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
		return;
	}
}

void MainWindow::showAfHistogram_all()
{
	showAfHistogram(false);
}

void MainWindow::showAfHistogram_filtered()
{
	showAfHistogram(true);
}

void MainWindow::showCnHistogram()
{
	if (filename_=="") return;

	QString title = "Copy-number histogram";

	AnalysisType type = variants_.type();
	if (type!=GERMLINE_SINGLESAMPLE && type!=GERMLINE_TRIO && type!=GERMLINE_MULTISAMPLE)
	{
		QMessageBox::information(this, title, "This functionality is only available for germline single sample and germline trio analysis.");
		return;
	}

	//check report sample SEG file exists
	QStringList seg_files = GlobalServiceProvider::fileLocationProvider().getCnvCoverageFiles(false).filterById(germlineReportSample()).asStringList();
	if (seg_files.isEmpty())
	{
		QMessageBox::warning(this, title, "Could not find a SEG file for sample " + germlineReportSample() + ". Aborting!");
		return;
	}

	try
	{
		//get region
		Chromosome chr;
		int start, end;
		QString region_text = QInputDialog::getText(this, title, "genomic region");
		if (region_text=="") return;

		NGSHelper::parseRegion(region_text, chr, start, end);

		//determine CN values
		QVector<double> cn_values;
		QSharedPointer<QFile> file = Helper::openFileForReading(seg_files[0]);
		QTextStream stream(file.data());
		while (!stream.atEnd())
		{
			QString line = stream.readLine();
			QStringList parts = line.split("\t");
			if (parts.count()<6) continue;

			//check if range overlaps input interval
			Chromosome chr2(parts[1]);
			if (chr!=chr2) continue;

			int start2 = Helper::toInt(parts[2], "Start coordinate");
			int end2 = Helper::toInt(parts[3], "End coordinate");
			if (!BasicStatistics::rangeOverlaps(start, end, start2, end2)) continue;

			//skip invalid copy-numbers
			QString cn_str = parts[5];
			if (cn_str.toLower()=="nan") continue;
			double cn = Helper::toDouble(parts[5], "Copy-number");
			if (cn<0) continue;

			cn_values << cn;
		}

		//create histogram
		std::sort(cn_values.begin(), cn_values.end());
		double median = BasicStatistics::median(cn_values,false);
		double max = ceil(median*2+0.0001);
		Histogram hist(0.0, max, max/40);
		foreach(double cn, cn_values)
		{
			hist.inc(cn, true);
		}

		//show chart
		QChartView* view = GUIHelper::histogramChart(hist, "Copy-number");
		auto dlg = GUIHelper::createDialog(view, QString("Copy-number in region ") + region_text);
		dlg->exec();
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, title, "Error:\n" + e.message());
		return;
	}
}

void MainWindow::showAfHistogram(bool filtered)
{
	if (filename_=="") return;

	AnalysisType type = variants_.type();
	if (type!=GERMLINE_SINGLESAMPLE && type!=GERMLINE_TRIO)
	{
		QMessageBox::information(this, "Allele frequency histogram", "This functionality is only available for germline single sample and germline trio analysis.");
		return;
	}

	//create histogram
	Histogram hist(0.0, 1.0, 0.05);
	int col_quality = variants_.annotationIndexByName("quality");
	for (int i=0; i<variants_.count(); ++i)
	{

		if (filtered && !filter_result_.passing(i)) continue;

		QByteArrayList parts = variants_[i].annotations()[col_quality].split(';');
		foreach(const QByteArray& part, parts)
		{
			if (part.startsWith("AF="))
			{
				bool ok;
				QString value_str = part.mid(3);
				if (type==GERMLINE_TRIO) value_str = value_str.split(',')[0];
				double value = value_str.toDouble(&ok);
				if (ok)
				{
					hist.inc(value, true);
				}
			}
		}
	}

	//show chart
	QChartView* view = GUIHelper::histogramChart(hist, "Allele frequency");
	auto dlg = GUIHelper::createDialog(view, QString("Allele frequency histogram ") + (filtered ? "(after filter)" : "(all variants)"));
	dlg->exec();
}

void MainWindow::on_actionEncrypt_triggered()
{
	//get input
	QString input = QInputDialog::getText(this, "Text for encryption", "text");
	if (input.isEmpty()) return;

	//decrypt
	QStringList out_lines;
	out_lines << ("Input text: " + input);
	try
	{
		qulonglong crypt_key = ToolBase::encryptionKey("encryption helper");
		out_lines << ("Encrypted text: " + SimpleCrypt(crypt_key).encryptToString(input));
	}
	catch(Exception& e)
	{
		out_lines << ("Error: " + e.message());
	}

	//show output
	QTextEdit* edit = new QTextEdit(this);
	edit->setText(out_lines.join("\n"));
	auto dlg = GUIHelper::createDialog(edit, "Encryption output");
	dlg->exec();
}

void MainWindow::on_actionSampleSearch_triggered()
{
	SampleSearchWidget* widget = new SampleSearchWidget(this);
	connect(widget, SIGNAL(openProcessedSampleTab(QString)), this, SLOT(openProcessedSampleTab(QString)));
	connect(widget, SIGNAL(openProcessedSample(QString)), this, SLOT(openProcessedSampleFromNGSD(QString)));
	openTab(QIcon(":/Icons/NGSD_sample_search.png"), "Sample search", widget);
}

void MainWindow::on_actionRunOverview_triggered()
{
	SequencingRunOverview* widget = new SequencingRunOverview(this);
	connect(widget, SIGNAL(openRun(QString)), this, SLOT(openRunTab(QString)));
	openTab(QIcon(":/Icons/NGSD_run_overview.png"), "Sequencing run overview", widget);
}

void MainWindow::addModelessDialog(QSharedPointer<QDialog> dlg, bool maximize)
{
	if (maximize)
	{
		dlg->showMaximized();
	}
	else
	{
		dlg->show();
	}
	modeless_dialogs_.append(dlg);

	//we always clean up when we add another dialog.
	//Like that, only one dialog can be closed and not destroyed at the same time.
	cleanUpModelessDialogs();
}

void MainWindow::cleanUpModelessDialogs()
{
	for (int i=modeless_dialogs_.count()-1; i>=0; --i)
	{
		if (modeless_dialogs_[i]->isHidden())
		{
			modeless_dialogs_.removeAt(i);
		}
	}
}

void MainWindow::importPhenotypesFromNGSD()
{
	QString ps_name = variants_.mainSampleName();
	try
	{
		NGSD db;
		QString sample_id = db.sampleId(ps_name);
		PhenotypeList phenotypes = db.getSampleData(sample_id).phenotypes;

		ui_.filters->setPhenotypes(phenotypes);
	}
	catch(Exception& /*e*/)
	{
		QMessageBox::warning(this, "Error loading phenotypes", "Cannot load phenotypes from NGSD for " + variants_.analysisName() + "!");
	}
}

void MainWindow::createSubPanelFromPhenotypeFilter()
{
	//convert phenotypes to genes
	QApplication::setOverrideCursor(Qt::BusyCursor);
	NGSD db;
	GeneSet genes;
	foreach(const Phenotype& pheno, ui_.filters->phenotypes())
	{
		genes << db.phenotypeToGenes(db.phenotypeIdByAccession(pheno.accession()), true);
	}
	QApplication::restoreOverrideCursor();

	//open dialog
	openSubpanelDesignDialog(genes);
}

void MainWindow::on_actionOpen_triggered()
{
	//get file to open
	QString path = Settings::path("path_variantlists", true);
	QString filename = QFileDialog::getOpenFileName(this, "Open variant list", path, "GSvar files (*.GSvar);;All files (*.*)");
	if (filename=="") return;

	//update data
	loadFile(filename);
}

void MainWindow::on_actionOpenByName_triggered()
{
	ProcessedSampleSelector dlg(this, false);
	dlg.showSearchMulti(true);
	if (!dlg.exec()) return;

	QString ps_name = dlg.processedSampleName();
	if (ps_name.isEmpty()) return;
	openProcessedSampleFromNGSD(ps_name, dlg.searchMulti());
}

void MainWindow::openProcessedSampleFromNGSD(QString processed_sample_name, bool search_multi)
{
	try
	{
		//convert name to file
		NGSD db;
		QString processed_sample_id = db.processedSampleId(processed_sample_name);
		QString file = db.processedSamplePath(processed_sample_id, PathType::GSVAR);

		//determine all analyses of the sample
		QStringList analyses;
		if (QFile::exists(file)) analyses << file;

		//somatic tumor sample > ask user if he wants to open the tumor-normal pair
		QString normal_sample = db.normalSample(processed_sample_id);
		if (normal_sample!="")
		{
			analyses << db.secondaryAnalyses(processed_sample_name + "-" + normal_sample, "somatic");
		}
		//check for germline trio/multi analyses
		else if (search_multi)
		{
			analyses << db.secondaryAnalyses(processed_sample_name, "trio");
			analyses << db.secondaryAnalyses(processed_sample_name, "multi sample");
		}

		//determine analysis to load
		if (analyses.count()==0)
		{
			QMessageBox::warning(this, "GSvar file missing", "The GSvar file does not exist:\n" + file);
			return;
		}
		else if (analyses.count()==1)
		{
			file = analyses[0];
		}
		else
		{
			bool ok = false;
			QString filename = QInputDialog::getItem(this, "Several analyses of the sample present", "select analysis:", analyses, 0, false, &ok);
			if (!ok)
			{
				return;
			}
			file = filename;
		}

		loadFile(file);
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Error opening processed sample from NGSD", e.message());
	}
}

void MainWindow::openSampleFromNGSD(QString sample_name)
{
	try
	{
		NGSD db;
		QStringList processed_samples = db.getValues("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.quality!='bad' AND ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples) AND s.name=:0", sample_name);
		if (processed_samples.isEmpty())
		{
			THROW(ArgumentException, "No high-quality processed sample found for sample name '" + sample_name + "'");
		}

		if (processed_samples.count()==1)
		{
			openProcessedSampleFromNGSD(processed_samples[0], false);
		}
		else
		{
			bool ok = false;
			QString ps = QInputDialog::getItem(this, "Several processed samples found for sample '" + sample_name + "'", "select processed sample:", processed_samples, 0, false, &ok);
			if (!ok) return;

			openProcessedSampleFromNGSD(ps, false);
		}
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Error opening sample from NGSD", e.message());
	}
}

void MainWindow::checkMendelianErrorRate(double cutoff_perc)
{
	QString output = "";
	try
	{
		SampleHeaderInfo infos = variants_.getSampleHeader();
		int col_c = infos.infoByStatus(true).column_index;

		bool above_cutoff = false;
		QStringList mers;
		foreach(const SampleInfo& info, infos)
		{
			if (info.isAffected()) continue;

			int errors = 0;
			int autosomal = 0;

			int col_p = info.column_index;

			for (int i=0; i<variants_.count(); ++i)
			{
				const Variant& v = variants_[i];
				if (!v.chr().isAutosome()) continue;
				++autosomal;

				QString geno_c = v.annotations()[col_c];
				QString geno_p = v.annotations()[col_p];

				if ((geno_p=="hom" && geno_c=="wt") || (geno_p=="wt" && geno_c=="hom")) ++errors;
			}

			double percentage = 100.0 * errors / autosomal;
			if (percentage>cutoff_perc) above_cutoff = true;
			mers << infos.infoByStatus(true).id + " - " + info.id + ": " + QString::number(errors) + "/" + QString::number(autosomal) + " ~ " + QString::number(percentage, 'f', 2) + "%";
		}

		if (above_cutoff)
		{
			output = "Mendelian error rate too high:\n" + mers.join("\n");
		}
	}
	catch (Exception& e)
	{
		output = "Mendelian error rate calulation not possible:\n" + e.message();
	}

	if (!output.isEmpty())
	{
		QMessageBox::warning(this, "Medelian error rate", output);
	}
}

void MainWindow::openProcessedSampleTab(QString ps_name)
{
	QString ps_id;
	try
	{
		ps_id = NGSD().processedSampleId(ps_name);
	}
	catch (DatabaseException e)
	{
		GUIHelper::showMessage("NGSD error", "The processed sample database ID could not be determined for '"  + ps_name + "'!\nError message: " + e.message());
		return;
	}

	ProcessedSampleWidget* widget = new ProcessedSampleWidget(this, ps_id);
	connect(widget, SIGNAL(openProcessedSampleTab(QString)), this, SLOT(openProcessedSampleTab(QString)));
	connect(widget, SIGNAL(openRunTab(QString)), this, SLOT(openRunTab(QString)));
	connect(widget, SIGNAL(executeIGVCommands(QStringList)), this, SLOT(executeIGVCommands(QStringList)));
	connect(widget, SIGNAL(openProcessingSystemTab(QString)), this, SLOT(openProcessingSystemTab(QString)));
	connect(widget, SIGNAL(openProjectTab(QString)), this, SLOT(openProjectTab(QString)));
	connect(widget, SIGNAL(openProcessedSampleFromNGSD(QString)), this, SLOT(openProcessedSampleFromNGSD(QString)));

	connect(widget, SIGNAL(clearMainTableSomReport(QString)), this, SLOT(clearSomaticReportSettings(QString)));

	int index = openTab(QIcon(":/Icons/NGSD_sample.png"), ps_name, widget);
	if (Settings::boolean("debug_mode_enabled"))
	{
		ui_.tabs->setTabToolTip(index, "NGSD ID: " + ps_id);
	}
}

void MainWindow::openRunTab(QString run_name)
{
	QString run_id;
	try
	{
		run_id = NGSD().getValue("SELECT id FROM sequencing_run WHERE name=:0", true, run_name).toString();
	}
	catch (DatabaseException e)
	{
		GUIHelper::showMessage("NGSD error", "The run database ID could not be determined for '"  + run_name + "'!\nError message: " + e.message());
		return;
	}

	SequencingRunWidget* widget = new SequencingRunWidget(this, run_id);
	connect(widget, SIGNAL(openProcessedSampleTab(QString)), this, SLOT(openProcessedSampleTab(QString)));
	int index = openTab(QIcon(":/Icons/NGSD_run.png"), run_name, widget);
	if (Settings::boolean("debug_mode_enabled"))
	{
		ui_.tabs->setTabToolTip(index, "NGSD ID: " + run_id);
	}
}

void MainWindow::openGeneTab(QString symbol)
{
	QPair<QString, QString> approved = NGSD().geneToApprovedWithMessage(symbol);
	if (approved.second.startsWith("ERROR:"))
	{
		GUIHelper::showMessage("NGSD error", "Gene name '" + symbol + "' is not a HGNC-approved name!\nError message:\n" + approved.second);
		return;
	}

	GeneWidget* widget = new GeneWidget(this, symbol.toLatin1());
	connect(widget, SIGNAL(openGeneTab(QString)), this, SLOT(openGeneTab(QString)));
	int index = openTab(QIcon(":/Icons/NGSD_gene.png"), symbol, widget);
	if (Settings::boolean("debug_mode_enabled"))
	{
		ui_.tabs->setTabToolTip(index, "NGSD ID: " + QString::number(NGSD().geneToApprovedID(symbol.toLatin1())));
	}
}

void MainWindow::openVariantTab(Variant variant)
{
	//check variant is in NGSD
	NGSD db;
	QString v_id = db.variantId(variant, false);
	if (v_id=="")
	{
		QMessageBox::information(this, "Variant not in NGSD", "Variant " + variant.toString() + " was not found in NGSD.");
		return;
	}

	//open tab
	VariantWidget* widget = new VariantWidget(variant, this);
	connect(widget, SIGNAL(openProcessedSampleTab(QString)), this, SLOT(openProcessedSampleTab(QString)));
	connect(widget, SIGNAL(openProcessedSampleFromNGSD(QString)), this, SLOT(openProcessedSampleFromNGSD(QString)));
	connect(widget, SIGNAL(openGeneTab(QString)), this, SLOT(openGeneTab(QString)));
	int index = openTab(QIcon(":/Icons/NGSD_variant.png"), variant.toString(), widget);
	if (Settings::boolean("debug_mode_enabled"))
	{
		ui_.tabs->setTabToolTip(index, "NGSD ID: " + v_id);
	}
}

void MainWindow::openProcessingSystemTab(QString name_short)
{
	NGSD db;
	int sys_id = db.processingSystemId(name_short, false);
	if (sys_id==-1)
	{
		GUIHelper::showMessage("NGSD error", "Processing system name '" + name_short + "' not found in NGSD!");
		return;
	}

	ProcessingSystemWidget* widget = new ProcessingSystemWidget(this, sys_id);
	connect(widget, SIGNAL(executeIGVCommands(QStringList)), this, SLOT(executeIGVCommands(QStringList)));
	int index = openTab(QIcon(":/Icons/NGSD_processing_system.png"), name_short, widget);
	if (Settings::boolean("debug_mode_enabled"))
	{
		ui_.tabs->setTabToolTip(index, "NGSD ID: " + QString::number(sys_id));
	}
}

void MainWindow::openProjectTab(QString name)
{
	ProjectWidget* widget = new ProjectWidget(this, name);
	connect(widget, SIGNAL(openProcessedSampleTab(QString)), this, SLOT(openProcessedSampleTab(QString)));
	int index = openTab(QIcon(":/Icons/NGSD_project.png"), name, widget);
	if (Settings::boolean("debug_mode_enabled"))
	{
		ui_.tabs->setTabToolTip(index, "NGSD ID: " + NGSD().getValue("SELECT id FROM project WHERE name=:0", true, name).toString());
	}
}

int MainWindow::openTab(QIcon icon, QString name, QWidget* widget)
{
	QScrollArea* scroll_area = new QScrollArea(this);
	scroll_area->setFrameStyle(QFrame::NoFrame);
	scroll_area->setWidgetResizable(true);
	scroll_area->setWidget(widget);
	//fix color problems
	QPalette pal;
	pal.setColor(QPalette::Window,QColor(0,0,0,0));
	scroll_area->setPalette(pal);
	scroll_area->setBackgroundRole(QPalette::Window);
	scroll_area->widget()->setPalette(pal);
	scroll_area->widget()->setBackgroundRole(QPalette::Window);
	//show tab
	int index = ui_.tabs->addTab(scroll_area, icon, name);
	ui_.tabs->setCurrentIndex(index);

	return index;
}

void MainWindow::closeTab(int index)
{
	if (index==0)
	{
		int res = QMessageBox::question(this, "Close file?", "Do you want to close the current sample?", QMessageBox::Yes, QMessageBox::No);
		if (res==QMessageBox::Yes)
		{
			loadFile();
		}
	}
	else
	{
		QWidget* widget = ui_.tabs->widget(index);
		ui_.tabs->removeTab(index);
		widget->deleteLater();
	}
}

void MainWindow::on_actionChangeLog_triggered()
{
	QDesktopServices::openUrl(QUrl("https://github.com/imgag/ngs-bits/tree/master/doc/GSvar/changelog.md"));
}

void MainWindow::loadFile(QString filename)
{
	//store variant list in case it changed
	if (!variants_changed_.isEmpty())
	{
		int result = QMessageBox::question(this, "Store GSvar file?", "The GSvar file was changed by you.\nDo you want to store the changes to file?", QMessageBox::Yes, QMessageBox::No);
		if (result==QMessageBox::Yes)
		{
			storeCurrentVariantList();
		}
	}

	QTime timer;
	timer.start();

	//reset GUI and data structures
	setWindowTitle(QCoreApplication::applicationName());
	filename_ = "";
	variants_.clear();
	GlobalServiceProvider::clearFileLocationProvider();
	variants_changed_.clear();
	cnvs_.clear();
	svs_.clear();
	igv_initialized_ = false;
	ui_.vars->clearContents();
	report_settings_ = ReportSettings();
	connect(report_settings_.report_config.data(), SIGNAL(variantsChanged()), this, SLOT(storeReportConfig()));
	germline_report_ps_ = "";
	somatic_report_settings_ = SomaticReportSettings();

	ui_.tabs->setCurrentIndex(0);

	ui_.filters->reset(true);

	Log::perf("Clearing variant table took ", timer);

	if (filename=="") return;

	//load data
	QApplication::setOverrideCursor(Qt::BusyCursor);
	try
	{
		//load variants
		timer.restart();
		variants_.load(filename);
		Log::perf("Loading small variant list took ", timer);
		if (filename.startsWith("http"))
		{
			GlobalServiceProvider::setFileLocationProvider(QSharedPointer<FileLocationProviderRemote>(new FileLocationProviderRemote(filename, Settings::string("server_host"), Settings::integer("server_port"))));
		}
		else
		{
			GlobalServiceProvider::setFileLocationProvider(QSharedPointer<FileLocationProviderLocal>(new FileLocationProviderLocal(filename, variants_.getSampleHeader(), variants_.type())));
		}

		//load CNVs
		timer.restart();
		FileLocation cnv_loc = GlobalServiceProvider::fileLocationProvider().getAnalysisCnvFile();
		if (cnv_loc.exists)
		{
			try
			{
				cnvs_.load(cnv_loc.filename);
				if (cnvs_.count()>80000) THROW(ArgumentException, "CNV file contains too many CNVs - could not load it!")
			}
			catch(Exception& e)
			{
				QMessageBox::warning(this, "Error loading CNVs", e.message());
				cnvs_.clear();
			}
		}
		Log::perf("Loading CNV list took ", timer);

		//load SVs
		timer.restart();
		FileLocation sv_loc = GlobalServiceProvider::fileLocationProvider().getAnalysisSvFile();
		if (sv_loc.exists)
		{
			try
			{
				svs_.load(sv_loc.filename);
			}
			catch(Exception& e)
			{
				QMessageBox::warning(this, "Error loading SVs", e.message());
				svs_.clear();
			}
		}
		Log::perf("Loading SV list took ", timer);

		ui_.filters->setValidFilterEntries(variants_.filters().keys());

		//update data structures
		Settings::setPath("path_variantlists", filename);
		filename_ = filename;

		//update GUI
		setWindowTitle(QCoreApplication::applicationName() + " - " + variants_.analysisName());
		ui_.statusBar->showMessage("Loaded variant list with " + QString::number(variants_.count()) + " variants.");

		refreshVariantTable(false);
		ui_.vars->adaptColumnWidths();

		QApplication::restoreOverrideCursor();
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, "Error", "Loading the file '" + filename + "' or displaying the contained variants failed!\nError message:\n" + e.message());
		loadFile();
		return;
	}

	//update recent files (before try block to remove non-existing files from the recent files menu)
	addToRecentFiles(filename);

	//check if variant list is outdated
	QStringList messages;
	try
	{
		ui_.variant_details->setLabelTooltips(variants_);
	}
	catch(Exception& e)
	{
		messages << e.message();
	}
	checkVariantList(messages);

	//load report config
	if (germlineReportSupported())
	{
		loadReportConfig();
	}
	else if(LoginManager::active() && somaticReportSupported())
	{
		loadSomaticReportConfig();
	}

	//check mendelian error rate for trios
	AnalysisType type = variants_.type();
	if (type==GERMLINE_TRIO)
	{
		checkMendelianErrorRate();
	}

	//notify for variant validation
	checkPendingVariantValidations();

	//activate Circos plot menu item if plot is available
	if (type==GERMLINE_SINGLESAMPLE && !GlobalServiceProvider::fileLocationProvider().getCircosPlotFiles(false).isEmpty())
	{
		ui_.actionCircos->setEnabled(true);
	}
	else
	{
		ui_.actionCircos->setEnabled(false);
	}

	//activate Repeat Expansion menu item if RE calls are available
	if (type==GERMLINE_SINGLESAMPLE && !GlobalServiceProvider::fileLocationProvider().getRepeatExpansionFiles(false).isEmpty())
	{
		ui_.actionRE->setEnabled(true);
	}
	else
	{
		ui_.actionRE->setEnabled(false);
	}

	//activate PRS menu item if PRS are available
	if (type==GERMLINE_SINGLESAMPLE && !GlobalServiceProvider::fileLocationProvider().getPrsFiles(false).isEmpty())
	{
		ui_.actionPRS->setEnabled(true);
	}
	else
	{
		ui_.actionPRS->setEnabled(false);
	}

	//activate cfDNA menu entries and get all available cfDNA samples
	cf_dna_available = false;
	ui_.actionDesignCfDNAPanel->setVisible(false);
	ui_.actionCfDNADiseaseCourse->setVisible(false);
	ui_.actionDesignCfDNAPanel->setEnabled(false);
	ui_.actionCfDNADiseaseCourse->setEnabled(false);
	cfdna_menu_btn_->setVisible(false);
	cfdna_menu_btn_->setEnabled(false);
	if (somaticReportSupported())
	{
		ui_.actionDesignCfDNAPanel->setVisible(true);
		ui_.actionCfDNADiseaseCourse->setVisible(true);
		cfdna_menu_btn_->setVisible(true);

		if (LoginManager::active())
		{
			ui_.actionDesignCfDNAPanel->setEnabled(true);
			cfdna_menu_btn_->setEnabled(true);
			NGSD db;
			QString sample_id;
			QStringList same_sample_ids;
			QStringList cf_dna_sample_ids;

			// get all same samples
			sample_id = db.sampleId(variants_.mainSampleName());
			same_sample_ids = db.relatedSamples(sample_id, "same sample");
			same_sample_ids << sample_id; // add current sample id

			// get all related cfDNA
			foreach (QString cur_sample_id, same_sample_ids)
			{
				cf_dna_sample_ids.append(db.relatedSamples(cur_sample_id, "tumor-cfDNA"));
			}

			if (cf_dna_sample_ids.size() > 0)
			{
				ui_.actionCfDNADiseaseCourse->setEnabled(true);
				cf_dna_available = true;
			}

		}

	}
}

void MainWindow::checkVariantList(QStringList messages)
{
	//check creation date
	QDate create_date = variants_.getCreationDate();
	if (create_date.isValid() && create_date < QDate::currentDate().addDays(-42))
	{
		messages << "annotations are older than six weeks (" + create_date.toString("yyyy-MM-dd") + ")";
	}

	//check sample header
	try
	{
		variants_.getSampleHeader();
	}
	catch(Exception e)
	{
		messages << e.message();
	}

	//create list of required columns
	QStringList cols;
	cols << "filter";
	AnalysisType type = variants_.type();
	if (type==GERMLINE_SINGLESAMPLE || type==GERMLINE_TRIO || type==GERMLINE_MULTISAMPLE)
	{
		cols << "classification";
		cols << "NGSD_hom";
		cols << "NGSD_het";
		cols << "comment";
		cols << "gene_info";
	}
	if (type==SOMATIC_SINGLESAMPLE || type==SOMATIC_PAIR)
	{
		cols << "somatic_classification";
		cols << "somatic_classification_comment";
		cols << "NGSD_som_vicc_interpretation";
		cols << "NGSD_som_vicc_comment";
	}

	//check columns
	foreach(QString col, cols)
	{
		if (variants_.annotationIndexByName(col, true, false)==-1)
		{
			messages << ("column '" + col + "' missing");
		}
	}

	if (!messages.empty())
	{
		QMessageBox::warning(this, "GSvar file outdated", "The GSvar file contains the following error(s):\n  -" + messages.join("\n  -") + "\n\nTo ensure that GSvar works as expected, re-run the annotation steps for the analysis!");
	}
}

void MainWindow::on_actionAbout_triggered()
{
	QMessageBox::about(this, "About " + QCoreApplication::applicationName(), QCoreApplication::applicationName()+ " " + QCoreApplication::applicationVersion()+ "\n\nA free viewing and filtering tool for genomic variants.\n\nInstitute of Medical Genetics and Applied Genomics\nUniversity Hospital Tübingen\nGermany\n\nMore information at:\nhttps://github.com/imgag/ngs-bits");
}

void MainWindow::loadReportConfig()
{
	//check if applicable
	if (!germlineReportSupported()) return;

	//check if report config exists
	NGSD db;
	QString processed_sample_id = db.processedSampleId(germlineReportSample(), false);
	int rc_id = db.reportConfigId(processed_sample_id);
	if (rc_id==-1) return;

	//load
	QStringList messages;
	report_settings_.report_config = db.reportConfig(rc_id, variants_, cnvs_, svs_, messages);
	connect(report_settings_.report_config.data(), SIGNAL(variantsChanged()), this, SLOT(storeReportConfig()));
	if (!messages.isEmpty())
	{
		QMessageBox::warning(this, "Report configuration", "The following problems were encountered while loading the report configuration:\n" + messages.join("\n"));
	}

	//updateGUI
	refreshVariantTable();
}

void MainWindow::loadSomaticReportConfig()
{
	if(filename_ == "") return;

	NGSD db;

	//Determine processed sample ids
	QString ps_tumor = variants_.mainSampleName();
	QString ps_tumor_id = db.processedSampleId(ps_tumor, false);
	if(ps_tumor_id == "") return;
	QString ps_normal = normalSampleName();
	if(ps_normal.isEmpty()) return;
	QString ps_normal_id = db.processedSampleId(ps_normal, false);
	if(ps_normal_id == "") return;


	somatic_report_settings_.tumor_ps = ps_tumor;
	somatic_report_settings_.normal_ps = ps_normal;
	somatic_report_settings_.msi_file = GlobalServiceProvider::fileLocationProvider().getSomaticMsiFile().filename;

	try //load normal sample
	{
		somatic_control_tissue_variants_.load(db.processedSamplePath(db.processedSampleId(ps_normal), PathType::GSVAR));
	}
	catch(Exception e)
	{
		QMessageBox::warning(this, "Could not load germline GSvar file", "Could not load germline GSvar file. No germline variants will be parsed for somatic report generation. Message: " + e.message());
	}



	//Continue loading report (only if existing in NGSD)
	if(db.somaticReportConfigId(ps_tumor_id, ps_normal_id) == -1) return;


	QStringList messages;
	somatic_report_settings_.report_config = db.somaticReportConfig(ps_tumor_id, ps_normal_id, variants_, cnvs_, somatic_control_tissue_variants_, messages);


	if(!messages.isEmpty())
	{
		QMessageBox::warning(this, "Somatic report configuration", "The following problems were encountered while loading the som. report configuration:\n" + messages.join("\n"));
	}

	//Preselect target region bed file in NGSD
	if(somatic_report_settings_.report_config.targetRegionName()!="")
	{
		ui_.filters->setTargetRegionByDisplayName(somatic_report_settings_.report_config.targetRegionName());
	}

	//Preselect filter from NGSD som. rep. conf.
	if(somatic_report_settings_.report_config.filter() != "")
	{
		ui_.filters->setFilter( somatic_report_settings_.report_config.filter() );
	}

	somatic_report_settings_.target_region_filter = ui_.filters->targetRegion();

	refreshVariantTable();
}

void MainWindow::storeSomaticReportConfig()
{
	if(filename_ == "") return;
	if(!LoginManager::active()) return;
	if(variants_.type() != SOMATIC_PAIR) return;

	NGSD db;
	QString ps_tumor_id = db.processedSampleId(variants_.mainSampleName(), false);
	QString ps_normal_id = db.processedSampleId(normalSampleName(), false);

	if(ps_tumor_id=="" || ps_normal_id == "")
	{
		QMessageBox::warning(this, "Storing somatic report configuration", "Samples were not found in the NGSD!");
		return;
	}

	int conf_id = db.somaticReportConfigId(ps_tumor_id, ps_normal_id);

	if (conf_id!=-1)
	{
		QStringList messages;
		QSharedPointer<ReportConfiguration> report_config = db.reportConfig(conf_id, variants_, cnvs_, svs_, messages);
		if (report_config->lastUpdatedBy()!="" && report_config->lastUpdatedBy()!=LoginManager::userName())
		{
			if (QMessageBox::question(this, "Storing report configuration", report_config->history() + "\n\nDo you want to override it?")==QMessageBox::No)
			{
				return;
			}
		}
	}

	//store
	try
	{
		db.setSomaticReportConfig(ps_tumor_id, ps_normal_id, somatic_report_settings_.report_config, variants_, cnvs_, somatic_control_tissue_variants_, Helper::userName());
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Storing somatic report configuration", "Error: Could not store the somatic report configuration.\nPlease resolve this error or report it to the administrator:\n\n" + e.message());
	}
}

void MainWindow::storeReportConfig()
{
	//check if applicable
	if (!germlineReportSupported()) return;

	//check sample
	NGSD db;
	QString processed_sample_id = db.processedSampleId(germlineReportSample(), false);
	if (processed_sample_id=="")
	{
		QMessageBox::warning(this, "Storing report configuration", "Sample was not found in the NGSD!");
		return;
	}

	//check if config exists and not edited by other user
	int conf_id = db.reportConfigId(processed_sample_id);
	if (conf_id!=-1)
	{
		QStringList messages;
		QSharedPointer<ReportConfiguration> report_config = db.reportConfig(conf_id, variants_, cnvs_, svs_, messages);
		if (report_config->lastUpdatedBy()!="" && report_config->lastUpdatedBy()!=LoginManager::userName())
		{
			if (QMessageBox::question(this, "Storing report configuration", report_config->history() + "\n\nDo you want to override it?")==QMessageBox::No)
			{
				return;
			}
		}
	}

	//store
	try
	{
		db.setReportConfig(processed_sample_id, report_settings_.report_config, variants_, cnvs_, svs_);
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Storing report configuration", "Error: Could not store the report configuration.\nPlease resolve this error or report it to the administrator:\n\n" + e.message());
	}
}

void MainWindow::generateEvaluationSheet()
{
	//check if applicable
	if (!germlineReportSupported()) return;

	QString base_name = germlineReportSample();

	//make sure free-text phenotype infos are available
	NGSD db;
	QString sample_id = db.sampleId(base_name);
	QList<SampleDiseaseInfo> disease_infos = db.getSampleDiseaseInfo(sample_id, "clinical phenotype (free text)");
	if (disease_infos.isEmpty() && QMessageBox::question(this, "Evaluation sheet", "No clinical phenotype (free text) is set for the sample!\nIt will be shown on the evaluation sheet!\n\nDo you want to set it?")==QMessageBox::Yes)
	{
		SampleDiseaseInfoWidget* widget = new SampleDiseaseInfoWidget(base_name, this);
		widget->setDiseaseInfo(db.getSampleDiseaseInfo(sample_id));
		auto dlg = GUIHelper::createDialog(widget, "Sample disease details", "", true);
		if (dlg->exec() != QDialog::Accepted) return;

		db.setSampleDiseaseInfo(sample_id, widget->diseaseInfo());
	}

	//try to get VariantListInfo from the NGSD
	QString ps_id = db.processedSampleId(base_name);
	EvaluationSheetData evaluation_sheet_data = db.evaluationSheetData(ps_id, false);
	if (evaluation_sheet_data.ps_id == "") //No db entry found > init
	{
		evaluation_sheet_data.ps_id = db.processedSampleId(base_name);
		evaluation_sheet_data.dna_rna = db.getSampleData(sample_id).name_external;
		// make sure reviewer 1 contains name not user id
		evaluation_sheet_data.reviewer1 = db.userName(db.userId(report_settings_.report_config->createdBy()));
		evaluation_sheet_data.review_date1 = report_settings_.report_config->createdAt().date();
		evaluation_sheet_data.reviewer2 = LoginManager::userName();
		evaluation_sheet_data.review_date2 = QDate::currentDate();
	}

	//Show VaraintSheetEditDialog
	EvaluationSheetEditDialog* edit_dialog = new EvaluationSheetEditDialog(this);
	edit_dialog->importEvaluationSheetData(evaluation_sheet_data);
	if (edit_dialog->exec() != QDialog::Accepted) return;

	//Store updated info in the NGSD
	db.storeEvaluationSheetData(evaluation_sheet_data, true);

	//get filename
	QString folder = Settings::path("gsvar_variantsheet_folder");
	QString filename = QFileDialog::getSaveFileName(this, "Store evaluation sheet",  folder + "/" + base_name + "_variant_sheet_" + QDate::currentDate().toString("yyyyMMdd") + ".html", "HTML files (*.html);;All files(*.*)");
	if (filename.isEmpty()) return;

	//write sheet
	PrsTable prs_table; //not needed
	GermlineReportGeneratorData generator_data(base_name, variants_, cnvs_, svs_, prs_table, report_settings_, ui_.filters->filters(), GSvarHelper::preferredTranscripts());
	GermlineReportGenerator generator(generator_data);
	generator.writeEvaluationSheet(filename, evaluation_sheet_data);

	if (QMessageBox::question(this, "Evaluation sheet", "Evaluation sheet generated successfully!\nDo you want to open it in your browser?")==QMessageBox::Yes)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
	}
}

void MainWindow::transferSomaticData()
{
	try
	{
		if(variants_.type()!=AnalysisType::SOMATIC_PAIR)
		{
			THROW(Exception, "Error: only possible for tumor-normal pair!");
		}

		SomaticDataTransferWidget data_transfer(somatic_report_settings_.tumor_ps, somatic_report_settings_.normal_ps, this);
		data_transfer.exec();
	}
	catch(Exception e)
	{
		QMessageBox::warning(this, "Transfer somatic data to MTB", e.message());
	}
}

void MainWindow::showReportConfigInfo()
{
	//check if applicable
	if (!germlineReportSupported() && !somaticReportSupported()) return;

	QString ps = germlineReportSupported() ? germlineReportSample() : variants_.mainSampleName();
	QString title = "Report configuration information of " + ps;

	//check sample exists
	NGSD db;
	QString processed_sample_id = db.processedSampleId(ps, false);
	if (processed_sample_id=="")
	{
		QMessageBox::warning(this, title, "Sample was not found in the NGSD!");
		return;
	}

	//check config exists
	if(germlineReportSupported())
	{
		int conf_id = db.reportConfigId(processed_sample_id);
		if (conf_id==-1)
		{
			QMessageBox::warning(this, title , "No germline report configuration found in the NGSD!");
			return;
		}


		QStringList messages;
		QSharedPointer<ReportConfiguration> report_config = db.reportConfig(conf_id, variants_, cnvs_, svs_, messages);

		QMessageBox::information(this, title, report_config->history() + "\n\n" + report_config->variantSummary());
	}
	else if(somaticReportSupported())
	{
		QString ps_normal_id = db.processedSampleId(normalSampleName(), false);
		int conf_id = db.somaticReportConfigId(processed_sample_id, ps_normal_id);
		if(conf_id==-1)
		{
			QMessageBox::warning(this, title, "No somatic report configuration found in the NGSD!");
			return;
		}
		QMessageBox::information(this, title, db.somaticReportConfigData(conf_id).history());
	}
}

void MainWindow::finalizeReportConfig()
{
	//check if applicable
	if(!germlineReportSupported()) return;

	QString dialog_title = "Finalize report configuration of " + germlineReportSample();

	//check sample exists
	NGSD db;
	QString processed_sample_id = db.processedSampleId(germlineReportSample(), false);
	if (processed_sample_id=="")
	{
		QMessageBox::warning(this, dialog_title, "Sample was not found in the NGSD!");
		return;
	}


	//check config exists
	int conf_id = db.reportConfigId(processed_sample_id);
	if (conf_id==-1)
	{
		QMessageBox::warning(this, dialog_title, "No report configuration for this sample found in the NGSD!");
		return;
	}

	//make sure the user knows what he does
	int button = QMessageBox::question(this, dialog_title, "Do you really want to finalize the report configuration?\nIt cannot be modified or deleted when finalized!");
	if (button!=QMessageBox::Yes) return;

	//finalize
	try
	{
		db.finalizeReportConfig(conf_id, LoginManager::userId());
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, dialog_title, "Finalizing report config failed:\"n" + e.message());
		return;
	}

	//update report settings data structure
	QStringList messages;
	report_settings_.report_config = db.reportConfig(conf_id, variants_, cnvs_, svs_, messages);
	connect(report_settings_.report_config.data(), SIGNAL(variantsChanged()), this, SLOT(storeReportConfig()));
}

void MainWindow::generateReport()
{
	if (filename_=="") return;

	//check if this is a germline or somatic
	if (somaticReportSupported())
	{
		generateReportSomaticRTF();
	}
	else if (tumoronlyReportSupported())
	{
		generateReportTumorOnly();
	}
	else if (germlineReportSupported())
	{
		generateReportGermline();
	}
	else
	{
		QMessageBox::information(this, "Report error", "Report not supported for this type of analysis!");
	}
}


void MainWindow::generateReportTumorOnly()
{
	try
	{
		TumorOnlyReportWorker::checkAnnotation(variants_);
	}
	catch(FileParseException e)
	{
		QMessageBox::warning(this, "Invalid tumor only file" + filename_, "Could not find all neccessary annotations in tumor-only variant list. Aborting creation of report. " + e.message());
		return;
	}
	QString ps = variants_.mainSampleName();

	//get report settings
	TumorOnlyReportWorkerConfig config;
	config.ps = ps;
	config.roi = ui_.filters->targetRegion();
	config.low_coverage_file = GlobalServiceProvider::fileLocationProvider().getSomaticLowCoverageFile().filename;
	config.bam_file = GlobalServiceProvider::fileLocationProvider().getBamFiles(true).at(0).filename;
	config.filter_result = filter_result_;
	config.preferred_transcripts = GSvarHelper::preferredTranscripts();

	TumorOnlyReportDialog dlg(variants_, config, this);
	if(!dlg.exec()) return;

	//get RTF file name
	QString destination_path = last_report_path_ + "/" + ps + "_DNA_tumor_only_" + QDate::currentDate().toString("yyyyMMdd") + ".rtf";
	QString file_rep = QFileDialog::getSaveFileName(this, "Store report file", destination_path, "RTF files (*.rtf);;All files(*.*)");
	if (file_rep=="") return;

	//Generate RTF
	QApplication::setOverrideCursor(Qt::BusyCursor);
	try
	{
		TumorOnlyReportWorker worker(variants_, config);
		QByteArray temp_filename = Helper::tempFileName(".rtf").toUtf8();
		worker.writeRtf(temp_filename);

		ReportWorker::moveReport(temp_filename, file_rep);
	}
	catch(Exception e)
	{
		QMessageBox::warning(this, "Could not create tumor only report", "Could not write tumor-only report. Error message: " + e.message());
	}

	QApplication::restoreOverrideCursor();

	//open report
	if (QMessageBox::question(this, "DNA tumor-only report", "report generated successfully!\nDo you want to open the report in your default RTF viewer?")==QMessageBox::Yes)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(file_rep) );
	}
}


void MainWindow::generateReportSomaticRTF()
{
	if(!LoginManager::active()) return;

	NGSD db;
	QString ps_tumor = variants_.mainSampleName();
	QString ps_tumor_id = db.processedSampleId(ps_tumor);
	QString ps_normal = normalSampleName();
	QString ps_normal_id = db.processedSampleId(ps_normal);

	//Set data in somatic report settings
	somatic_report_settings_.report_config.setTargetRegionName(ui_.filters->targetRegion().name);

	somatic_report_settings_.report_config.setFilter((ui_.filters->filterName() != "[none]" ? ui_.filters->filterName() : "") ); //filter name -> goes to NGSD som. rep. conf.
	somatic_report_settings_.filters = ui_.filters->filters(); //filter cascase -> goes to report helper

	somatic_report_settings_.tumor_ps = ps_tumor;
	somatic_report_settings_.normal_ps = ps_normal;

	somatic_report_settings_.preferred_transcripts = GSvarHelper::preferredTranscripts();
	somatic_report_settings_.processing_system_roi = GlobalServiceProvider::database().processingSystemRegions( db.processingSystemIdFromProcessedSample(ps_tumor) );
	somatic_report_settings_.processing_system_genes = db.genesToApproved( GlobalServiceProvider::database().processingSystemGenes(db.processingSystemIdFromProcessedSample(ps_tumor)), true );

	somatic_report_settings_.target_region_filter = ui_.filters->targetRegion();


	//Preselect report settings if not already exists to most common values
	if(db.somaticReportConfigId(ps_tumor_id, ps_normal_id) == -1)
	{
		somatic_report_settings_.report_config.setTumContentByMaxSNV(true);
		somatic_report_settings_.report_config.setTumContentByClonality(true);
		somatic_report_settings_.report_config.setTumContentByHistological(true);
		somatic_report_settings_.report_config.setMsiStatus(true);
		somatic_report_settings_.report_config.setCnvBurden(true);
		somatic_report_settings_.report_config.setHrdScore(0);
	}

	SomaticReportDialog dlg(somatic_report_settings_, cnvs_, somatic_control_tissue_variants_, this); //widget for settings

	if(SomaticRnaReport::checkRequiredSNVAnnotations(variants_))
	{
		dlg.enableChoiceReportType(true);
		//get RNA ids annotated to GSvar file
		QStringList rna_ids;
		for(const auto& an : variants_.annotations())
		{
			if(an.name().contains("_rna_tpm")) rna_ids << QString(an.name()).replace("_rna_tpm", "");
		}
		dlg.setRNAids(rna_ids);
	}

	if(!dlg.exec())
	{
		return;
	}

	dlg.writeBackSettings();


	//store somatic report config in NGSD
	if(!dlg.skipNGSD())
	{		
		db.setSomaticReportConfig(ps_tumor_id, ps_normal_id, somatic_report_settings_.report_config, variants_, cnvs_, somatic_control_tissue_variants_, Helper::userName());
	}

	QString destination_path; //path to rtf file
	if(dlg.getReportType() == SomaticReportDialog::report_type::DNA)
	{		
		destination_path = last_report_path_ + "/" + ps_tumor + "_DNA_report_somatic_" + QDate::currentDate().toString("yyyyMMdd") + ".rtf";
	}
	else
	{
		destination_path = last_report_path_ + "/" + dlg.getRNAid() + "-" + ps_tumor + "_RNA_report_somatic_" + QDate::currentDate().toString("yyyyMMdd") + ".rtf";
	}

	//get RTF file name
	QString file_rep = QFileDialog::getSaveFileName(this, "Store report file", destination_path, "RTF files (*.rtf);;All files(*.*)");
	if (file_rep=="") return;

	QApplication::setOverrideCursor(Qt::BusyCursor);

	if(dlg.getReportType() == SomaticReportDialog::report_type::DNA)
	{
		//generate somatic DNA report
		try
		{
			if(!SomaticReportHelper::checkGermlineSNVFile(somatic_control_tissue_variants_))
			{
				QApplication::restoreOverrideCursor();
				QMessageBox::warning(this, "Somatic report", "DNA report cannot be created because germline GSVar-file is invalid. Please check control tissue variant file.");
				return;
			}

			SomaticReportHelper report(variants_, cnvs_, somatic_control_tissue_variants_, somatic_report_settings_);

			//Store XML file with the same somatic report configuration settings
			QString gsvar_xml_folder = Settings::path("gsvar_xml_folder");

			try
			{
				report.storeXML(gsvar_xml_folder + "\\" + somatic_report_settings_.tumor_ps + "-" + somatic_report_settings_.normal_ps + ".xml");
			}
			catch(Exception e)
			{
				QMessageBox::warning(this, "creation of XML file failed", e.message());
			}

			//Generate RTF
			QByteArray temp_filename = Helper::tempFileName(".rtf").toUtf8();

			report.storeRtf(temp_filename);

			ReportWorker::moveReport(temp_filename, file_rep);

			//Generate files for QBIC upload
			QString base_dir = Settings::path("qbic_data_path", true);
			if (!base_dir.isEmpty())
			{
				QString path = base_dir + ps_tumor + "-" + ps_normal + QDir::separator();
				report.storeQbicData(path);
			}

			QApplication::restoreOverrideCursor();
		}
		catch(Exception& error)
		{
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, "Error while creating report", error.message());
			return;
		}
		catch(...)
		{
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, "Error while creating report", "No error message!");
			return;
		}

		//open report
		if (QMessageBox::question(this, "DNA report", "DNA report generated successfully!\nDo you want to open the report in your default RTF viewer?")==QMessageBox::Yes)
		{
			QDesktopServices::openUrl(QUrl::fromLocalFile(file_rep) );
		}
	}
	else //RNA report
	{
		//Generate RTF
		try
		{
			QByteArray temp_filename = Helper::tempFileName(".rtf").toUtf8();


			SomaticRnaReportData rna_report_data = somatic_report_settings_;
			rna_report_data.rna_ps_name = dlg.getRNAid();
			rna_report_data.rna_fusion_file = db.processedSamplePath(db.processedSampleId(dlg.getRNAid()), PathType::FUSIONS);

			SomaticRnaReport rna_report(variants_, cnvs_, rna_report_data);

			rna_report.checkRefTissueTypeInNGSD(rna_report.refTissueType(variants_),somatic_report_settings_.tumor_ps);

			rna_report.writeRtf(temp_filename);
			ReportWorker::moveReport(temp_filename, file_rep);
			QApplication::restoreOverrideCursor();
		}
		catch(Exception& error)
		{
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, "Error while creating somatic RNA report.", error.message());
			return;
		}
		catch(...)
		{
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, "Error while creating somatic RNA report.", "No error message!");
			return;
		}

		if (QMessageBox::question(this, "RNA report", "RNA report generated successfully!\nDo you want to open the report in your default RTF viewer?")==QMessageBox::Yes)
		{
			QDesktopServices::openUrl(QUrl::fromLocalFile(file_rep));
		}
	}
}

void MainWindow::generateReportGermline()
{
	//check that sample is in NGSD
	NGSD db;
	QString ps_name = germlineReportSample();
	QString sample_id = db.sampleId(ps_name, false);
	QString processed_sample_id = db.processedSampleId(ps_name, false);
	if (sample_id.isEmpty() || processed_sample_id.isEmpty())
	{
		GUIHelper::showMessage("Error", "Sample not found in the NGSD.\nCannot generate a report for samples that are not in the NGSD!");
		return;
	}

	//check if there are unclosed gaps
	QStringList unclosed_gap_ids = db.getValues("SELECT id FROM gaps WHERE processed_sample_id='" + processed_sample_id + "' AND (status='to close' OR status='in progress')");
	if (unclosed_gap_ids.count()>0 && QMessageBox::question(this, "Not all gaps closed", "There are gaps for this sample, which still have to be closed!\nDo you want to continue?")==QMessageBox::No)
	{
		return;
	}

	//show report dialog
	ReportDialog dialog(ps_name, report_settings_, variants_, cnvs_, svs_, ui_.filters->targetRegion(), this);
	if (!dialog.exec()) return;

	//set report type
	report_settings_.report_type = dialog.type();

	//get export file name
	QString trio_suffix = (variants_.type() == GERMLINE_TRIO ? "trio_" : "");
	QString type_suffix = dialog.type().replace(" ", "_") + "s_";
	QString roi_name = ui_.filters->targetRegion().name;
	if (roi_name!="") //remove date and prefix with '_'
	{
		roi_name.remove(QRegExp("_[0-9]{4}_[0-9]{2}_[0-9]{2}"));
		roi_name = "_" + roi_name;
	}
	QString file_rep = QFileDialog::getSaveFileName(this, "Export report file", last_report_path_ + "/" + ps_name + roi_name + "_report_" + trio_suffix + type_suffix + QDate::currentDate().toString("yyyyMMdd") + ".html", "HTML files (*.html);;All files(*.*)");
	if (file_rep=="") return;
	last_report_path_ = QFileInfo(file_rep).absolutePath();

	//prepare report generation data
	PrsTable prs_table;
	FileLocationList prs_files = GlobalServiceProvider::fileLocationProvider().getPrsFiles(false).filterById(ps_name);
	if (prs_files.count()==1) prs_table.load(prs_files[0].filename);

	GermlineReportGeneratorData data(ps_name, variants_, cnvs_, svs_, prs_table, report_settings_, ui_.filters->filters(), GSvarHelper::preferredTranscripts());
	data.processing_system_roi = GlobalServiceProvider::database().processingSystemRegions(db.processingSystemIdFromProcessedSample(ps_name));
	if (ui_.filters->targetRegion().isValid())
	{
		data.roi = ui_.filters->targetRegion();
	}

	//show busy dialog
	busy_dialog_ = new BusyDialog("Report", this);
	busy_dialog_->init("Generating report", false);

	//start worker in new thread
	ReportWorker* worker = new ReportWorker(data, file_rep);
	connect(worker, SIGNAL(finished(bool)), this, SLOT(reportGenerationFinished(bool)));
	worker->start();
}

void MainWindow::reportGenerationFinished(bool success)
{
	delete busy_dialog_;

	//show result info box
	ReportWorker* worker = qobject_cast<ReportWorker*>(sender());
	if (success)
	{
		if (QMessageBox::question(this, "Report", "Report generated successfully!\nDo you want to open the report in your browser?")==QMessageBox::Yes)
		{
			QDesktopServices::openUrl(QUrl::fromLocalFile(worker->getReportFile()));
		}
	}
	else
	{
		QMessageBox::warning(this, "Error", "Report generation failed:\n" + worker->errorMessage());
	}
	//clean
	worker->deleteLater();
}

void MainWindow::openProcessedSampleTabsCurrentAnalysis()
{
	if (filename_=="") return;

	SampleHeaderInfo infos = variants_.getSampleHeader();
	foreach(const SampleInfo& info, infos)
	{
		openProcessedSampleTab(info.id);
	}
}

void MainWindow::on_actionOpenProcessedSampleTabByName_triggered()
{
	ProcessedSampleSelector dlg(this, true);
	if (!dlg.exec()) return;

	QString ps_name = dlg.processedSampleName();
	if (ps_name.isEmpty()) return;

	openProcessedSampleTab(ps_name);
}

void MainWindow::on_actionOpenSequencingRunTabByName_triggered()
{
	//create
	DBSelector* selector = new DBSelector(this);
	NGSD db;
	selector->fill(db.createTable("sequencing_run", "SELECT id, name FROM sequencing_run"));

	//show
	auto dlg = GUIHelper::createDialog(selector, "Select sequencing run", "run:", true);
	if (dlg->exec()==QDialog::Rejected) return ;

	//handle invalid name
	if (selector->getId()=="") return;

	openRunTab(selector->text());
}

QString MainWindow::selectGene()
{
	//create
	DBSelector* selector = new DBSelector(QApplication::activeWindow());
	NGSD db;
	selector->fill(db.createTable("gene", "SELECT id, symbol FROM gene"));

	//show
	auto dlg = GUIHelper::createDialog(selector, "Select gene", "symbol:", true);
	if (dlg->exec()==QDialog::Rejected) return "";

	//handle invalid name
	if (selector->getId()=="") return "";

	return selector->text();
}

void MainWindow::importBatch(QString title, QString text, QString table, QStringList fields)
{
	//show dialog
	QTextEdit* edit = new QTextEdit();
	edit->setAcceptRichText(false);
	auto dlg = GUIHelper::createDialog(edit, title, text, true);
	if (dlg->exec()!=QDialog::Accepted) return;

	// load input text as table
	QList<QStringList> table_content;
	QStringList lines = edit->toPlainText().split("\n");
	foreach(QString line, lines)
	{
		// skip comments and empty lines
		if (line.trimmed().isEmpty() || line[0]=='#') continue;

		QStringList parts = line.split("\t");
		table_content.append(parts);
	}
	NGSD db;

	//special handling of processed sample: add 'process_id' and get the list of all cfDNA processing systems
	QStringList cfdna_processing_systems;
	if (table=="processed_sample")
	{
		fields.append("process_id");

		// get all cfDNA processing system
		cfdna_processing_systems = db.getValues("SELECT name_manufacturer FROM processing_system WHERE type='cfDNA (patient-specific)'");
	}

	// special handling of sample: extract last column containing the corresponding tumor sample id for a cfDNA sample
	QList<QPair<QString,QString>> cfdna_tumor_relation;
	if (table=="sample")
	{
		int name_idx = fields.indexOf("name");
		for (int i = 0; i < table_content.size(); ++i)
		{
			QStringList& row = table_content[i];
			if (row.length() == (fields.length() + 1))
			{
				//store tumor-cfDNA relation
				QString tumor_sample = row.last();
				QString cfdna_sample = row.at(name_idx).trimmed();
				// only import if not empty
				if (!tumor_sample.isEmpty()) cfdna_tumor_relation.append(QPair<QString,QString>(tumor_sample, cfdna_sample));

				// remove last element
				row.removeLast();
			}
		}
	}

	//special handling of sample_relations: add user
	if (table=="sample_relations")
	{
		fields.append("user_id");
	}

	//prepare query
	QString query_str = "INSERT INTO " + table + " (" + fields.join(", ") + ") VALUES (";
	for(int i=0; i<fields.count(); ++i)
	{
		if (i!=0) query_str += ", ";
		query_str += ":" + QString::number(i);
	}
	query_str += ")";

	SqlQuery q_insert = db.getQuery();
	q_insert.prepare(query_str);

	//check and insert
	QString last_processed_line;
	try
	{
		db.transaction();

		int imported = 0;
		QStringList duplicate_samples;
		QStringList missing_cfDNA_relation;
		for (int i = 0; i < table_content.size(); ++i)
		{
			QStringList& row = table_content[i];
			last_processed_line = row.join("\t");

			//special handling of processed sample: add 'process_id' and check tumor relation for cfDNA samples
			if (table=="processed_sample")
			{
				// process id
				QString sample_name = row[0].trimmed();
				QVariant next_ps_number = db.getValue("SELECT MAX(ps.process_id)+1 FROM sample as s, processed_sample as ps WHERE s.id=ps.sample_id AND s.name=:0", true, sample_name);
				row.append(next_ps_number.isNull() ? "1" : next_ps_number.toString());
			}

			//special handling of sample duplicates
			if (table=="sample")
			{
				QString sample_name = row[0].trimmed();
				QString sample_id = db.sampleId(sample_name, false);
				if (sample_id!="")
				{
					duplicate_samples << sample_name;
					continue;
				}
			}

			//special handling of sample_relations: add user
			if (table=="sample_relations")
			{
				row.append(LoginManager::userName());
			}

			//check tab-separated parts count
			if (row.count()!=fields.count()) THROW(ArgumentException, "Error: line with more/less than " + QString::number(fields.count()) + " tab-separated parts.");
			//check and bind
			for(int i=0; i<fields.count(); ++i)
			{
				QString value = row[i].trimmed();

				//check for cfDNA samples if a corresponding tumor sample exists
				if ((table=="processed_sample") && (fields[i] == "processing_system_id") && (cfdna_processing_systems.contains(value)))
				{
					// get tumor relation
					QString sample_id = db.sampleId(row[0].trimmed());
					QStringList tumor_samples = db.relatedSamples(sample_id, "tumor-cfDNA");

					if (tumor_samples.size() < 1)
					{
						//No corresponding tumor found!
						missing_cfDNA_relation.append(row[0].trimmed());
					}
				}

				const TableFieldInfo& field_info = db.tableInfo(table).fieldInfo(fields[i]);


				//FK: name to id
				if (field_info.type==TableFieldInfo::FK && !value.isEmpty())
				{
					QString name_field = field_info.fk_name_sql;
					if (name_field.startsWith("CONCAT(name")) //some FK-fields show additional information after the name > use only the name
					{
						name_field = name_field.left(name_field.indexOf(','));
						name_field = name_field.mid(7);
					}
					value = db.getValue("SELECT " + field_info.fk_field + " FROM " + field_info.fk_table + " WHERE " + name_field + "=:0", false, value).toString();
				}

				//accept German dates as well
				if (field_info.type==TableFieldInfo::DATE && !value.isEmpty())
				{
					QDate date_german = QDate::fromString(value, "dd.MM.yyyy");
					if (date_german.isValid())
					{
						value = date_german.toString(Qt::ISODate);
					}
				}

				//decimal point for numbers
				if (field_info.type==TableFieldInfo::FLOAT && value.contains(','))
				{
					value = value.replace(',', '.');
				}

				//check errors
				QStringList errors = db.checkValue(table, fields[i], value, true);
				if (errors.count()>0)
				{
					THROW(ArgumentException, "Error: Invalid value '" + value + "' for field '" + fields[i] + "':\n" + errors.join("\n"));
				}

				q_insert.bindValue(i, value.isEmpty() && field_info.is_nullable ? QVariant() : value);
			}

			// abort if cfDNA
			//insert
			q_insert.exec();
			++imported;
		}

		//handle duplicate samples
		if (duplicate_samples.count()>0)
		{
			int button = QMessageBox::question(this, title, QString::number(duplicate_samples.count()) +" samples are already present in the NGSD:\n" + duplicate_samples.join(" ") + "\n\n Do you want to skip these samples and continue?", QMessageBox::Ok, QMessageBox::Abort);
			if (button==QMessageBox::Abort)
			{
				db.rollback();
				return;
			}
		}

		//import tumor-cfDNA relations
		int imported_relations = 0;
		for (int i = 0; i < cfdna_tumor_relation.size(); ++i)
		{
			const QPair<QString, QString>& relation = cfdna_tumor_relation.at(i);
			// check for valid input
			if (db.getSampleData(db.sampleId(relation.first)).is_tumor)
			{
				// add relation
				db.addSampleRelation(SampleRelation{relation.first.toLatin1(), "tumor-cfDNA", relation.second.toLatin1()}, true);
				imported_relations++;
			}
			else
			{
				THROW(DatabaseException, "Sample " + relation.first + " is not a tumor! Can't import relation.");
			}
		}

		// abort if cfDNA-tumor relations are missing
		if (missing_cfDNA_relation.size() > 0)
		{
			THROW(ArgumentException, "The NGSD does not contain a cfDNA-tumor relation for the following cfDNA samples:\n" + missing_cfDNA_relation.join(", ") + "\nAborting import.");
		}

		db.commit();

		QString message = "Imported " + QString::number(imported) + " table rows.";
		if (duplicate_samples.count()>0) message += "\n\nSkipped " + QString::number(duplicate_samples.count()) + " table rows.";
		if (imported_relations > 0) message += "\n\nImported " + QString::number(imported_relations) + " tumor-cfDNA relations.";
		QMessageBox::information(this, title,  message);
	}
	catch (Exception& e)
	{
		db.rollback();
		QMessageBox::warning(this, title + " - failed", "Import failed - no data was imported!\n\nLine:\n"+ last_processed_line.trimmed() + "\n\nError message:\n" + e.message());
	}
}

int MainWindow::igvPort() const
{
	int port = Settings::integer("igv_port");

	//if NGSD is enabled, add the user ID (like that, several users can work on one server)
	if (LoginManager::active())
	{
		port += LoginManager::userId();
	}

	//if manual override is set, use it
	if (igv_port_manual>0) port = igv_port_manual;

	return port;
}

void MainWindow::on_actionOpenGeneTabByName_triggered()
{
	QString symbol = selectGene();
	if (symbol=="") return;

	openGeneTab(symbol);
}


void MainWindow::on_actionOpenVariantTab_triggered()
{
	//get user input
	bool ok;
	QString text = QInputDialog::getText(this, "Enter variant", "genomic coordinates (GSvar format):", QLineEdit::Normal, "", &ok);
	if (!ok) return;

	//parse variant
	Variant v;
	try
	{
		 v = Variant::fromString(text);
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Invalid variant text", e.message());
		return;
	}

	//show sample overview for variant
	openVariantTab(v);
}

void MainWindow::on_actionOpenProcessingSystemTab_triggered()
{
	//create
	DBSelector* selector = new DBSelector(this);
	NGSD db;
	selector->fill(db.createTable("processing_system", "SELECT id, CONCAT(name_manufacturer, ' (', name_short, ')') FROM processing_system"));

	//show
	auto dlg = GUIHelper::createDialog(selector, "Select processing system", "name:", true);
	if (dlg->exec()==QDialog::Rejected) return;

	//handle invalid name
	if (selector->getId()=="") return;

	openProcessingSystemTab(db.getValue("SELECT name_short FROM processing_system WHERE id=" + selector->getId()).toString());
}

void MainWindow::on_actionOpenProjectTab_triggered()
{
	//create
	DBSelector* selector = new DBSelector(this);
	NGSD db;
	selector->fill(db.createTable("project", "SELECT id, name FROM project"));

	//show
	auto dlg = GUIHelper::createDialog(selector, "Select project", "project:", true);
	if (dlg->exec()==QDialog::Rejected) return ;

	//handle invalid name
	if (selector->getId()=="") return;

	openProjectTab(selector->text());
}

void MainWindow::on_actionStatistics_triggered()
{
	try
	{
		LoginManager::checkRoleIn(QStringList() << "admin");
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Permissions error", e.message());
		return;
	}

	QApplication::setOverrideCursor(Qt::BusyCursor);

	NGSD db;
	TsvFile table;
	bool human_only = true;

	//table header
	table.addHeader("month");
	QStringList sys_types = QStringList() << "WGS" << "WES" << "Panel" << "RNA";
	QStringList pro_types = QStringList() << "diagnostic" << "research";
	foreach(QString pro, pro_types)
	{
		foreach(QString sys, sys_types)
		{
			table.addHeader(sys + " " + pro);
		}
	}

	//table rows
	QSet<QString> comments;
	QDate start = QDate::currentDate();
	start = start.addDays(1-start.day());
	QDate end = start.addMonths(1);
	while(start.year()>=2015)
	{
		QVector<int> counts(table.headers().count(), 0);

		//select runs of current month
		SqlQuery q_run_ids = db.getQuery();
		q_run_ids.exec("SELECT id FROM sequencing_run WHERE end_date >='" + start.toString(Qt::ISODate) + "' AND end_date < '" + end.toString(Qt::ISODate) + "' AND status!='analysis_not_possible' AND status!='run_aborted' AND status!='n/a' AND quality!='bad'");
		while(q_run_ids.next())
		{
			//select samples
			SqlQuery q_sample_data = db.getQuery();
			q_sample_data.exec("SELECT sys.type, p.type FROM sample s, processed_sample ps, processing_system sys, project p WHERE " + QString(human_only ? " s.species_id=(SELECT id FROM species WHERE name='human') AND " : "") + " ps.processing_system_id=sys.id AND ps.sample_id=s.id AND ps.project_id=p.id AND ps.sequencing_run_id=" + q_run_ids.value(0).toString() + " AND ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples)");

			//count
			while(q_sample_data.next())
			{
				QString sys_type = q_sample_data.value(0).toString();
				if (sys_type.contains("Panel")) sys_type = "Panel";
				if (!sys_types.contains(sys_type))
				{
					comments << "##Skipped processing system type '" + sys_type + "'";
					continue;
				}
				QString pro_type = q_sample_data.value(1).toString();
				if (!pro_types.contains(pro_type))
				{
					comments << "##Skipped project type '" + pro_type + "'";
					continue;
				}

				int index = table.headers().indexOf(sys_type + " " + pro_type);
				++counts[index];
			}
		}

		//create row
		QStringList row;
		for(int i=0; i<counts.count(); ++i)
		{
			if (i==0)
			{
				row << start.toString("yyyy/MM");
			}
			else
			{
				row << QString::number(counts[i]);
			}
		}
		table.addRow(row);

		//next month
		start = start.addMonths(-1);
		end = end.addMonths(-1);
	}

	//comments
	foreach(QString comment, comments)
	{
		table.addComment(comment);
	}

	QApplication::restoreOverrideCursor();

	//show dialog
	TsvTableWidget* widget = new TsvTableWidget(table);
	widget->setMinimumWidth(850);
	auto dlg = GUIHelper::createDialog(widget, "Statistics", "Sequencing statistics grouped by month (human)");
	dlg->exec();
}

void MainWindow::on_actionDevice_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("device");
	auto dlg = GUIHelper::createDialog(widget, "Device administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionGenome_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("genome");
	auto dlg = GUIHelper::createDialog(widget, "Genome administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionMID_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("mid");
	auto dlg = GUIHelper::createDialog(widget, "MID administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionProcessingSystem_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("processing_system");
	connect(widget, SIGNAL(openProcessingSystemTab(QString)), this, SLOT(openProcessingSystemTab(QString)));
	auto dlg = GUIHelper::createDialog(widget, "Processing system administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionProject_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("project");
	connect(widget, SIGNAL(openProjectTab(QString)), this, SLOT(openProjectTab(QString)));
	auto dlg = GUIHelper::createDialog(widget, "Project administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSample_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("sample");
	auto dlg = GUIHelper::createDialog(widget, "Sample administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSampleGroup_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("sample_group");
	auto dlg = GUIHelper::createDialog(widget, "Sample group administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSender_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("sender");
	auto dlg = GUIHelper::createDialog(widget, "Sender administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSpecies_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("species");
	auto dlg = GUIHelper::createDialog(widget, "Species administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionUsers_triggered()
{
	try
	{
		LoginManager::checkRoleIn(QStringList() << "admin");
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Permissions error", e.message());
		return;
	}

	//show user table
	DBTableAdministration* widget = new DBTableAdministration("user");
	auto dlg = GUIHelper::createDialog(widget, "User administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionImportMids_triggered()
{
	importBatch("Import MIDs",
				"Batch import of MIDs. Please enter MIDs as tab-delimited text.<br>Example:<br><br>illumina 1 → CGTGAT<br>illumina 2 → AGATA<br>illumina 3 → GTCATG",
				 "mid",
				QStringList() << "name" << "sequence"
				);
}

void MainWindow::on_actionImportStudy_triggered()
{
	importBatch("Import study",
				"Batch import of stamples to studies. Please enter study, processed sample and study-specific name of sample:<br>Example:<br><br>SomeStudy → NA12345_01 → NameOfSampleInStudy",
				 "study_sample",
				QStringList() << "study_id" << "processed_sample_id" << "study_sample_idendifier"
				);
}
void MainWindow::on_actionImportSamples_triggered()
{
	importBatch("Import samples",
				"Batch import of samples. Must contain the following tab-separated fields:<br><b>name</b>, name external, <b>sender</b>, received, received by, <b>sample type</b>, <b>tumor</b>, <b>ffpe</b>, <b>species</b>, concentration [ng/ul], volume, 260/280, 260/230, RIN/DIN, <b>gender</b>, <b>quality</b>, comment <br> (For cfDNA Samples a additional column which defines the corresponding tumor sample can be given.) ",
				"sample",
				QStringList() << "name" << "name_external" << "sender_id" << "received" << "receiver_id" << "sample_type" << "tumor" << "ffpe" << "species_id" << "concentration" << "volume" << "od_260_280" << "od_260_230" << "integrity_number" << "gender" << "quality" << "comment"
				);
}

void MainWindow::on_actionImportProcessedSamples_triggered()
{
	importBatch("Import processed samples",
				"Batch import of processed samples. Must contain the following tab-separated fields:<br><b>sample</b>, <b>project</b>, <b>run name</b>, <b>lane</b>, mid1 name, mid2 name, operator, <b>processing system</b>, processing input [ng], molarity [nM], comment, normal processed sample",
				"processed_sample",
				QStringList() << "sample_id" << "project_id" << "sequencing_run_id" << "lane" << "mid1_i7" << "mid2_i5" << "operator_id" << "processing_system_id" << "processing_input" << "molarity" << "comment" << "normal_id"
				);
}

void MainWindow::on_actionImportSampleRelations_triggered()
{
	importBatch("Import sample relations",
				"Batch import of sample relations. Must contain the following tab-separated fields:<br><b>sample1</b>, <b>relation</b>, <b>sample2</b>",
				"sample_relations",
				QStringList() << "sample1_id" << "relation" << "sample2_id"
				);
}

void MainWindow::on_actionMidClashDetection_triggered()
{
	MidCheckWidget* widget = new MidCheckWidget();
	auto dlg = GUIHelper::createDialog(widget, "MID clash detection");
	dlg->exec();
}

void MainWindow::on_actionVariantValidation_triggered()
{
	VariantValidationWidget* widget = new VariantValidationWidget();
	auto dlg = GUIHelper::createDialog(widget, "Variant validation");
	dlg->exec();
}

void MainWindow::on_actionChangePassword_triggered()
{
	PasswordDialog dlg(this);
	if(dlg.exec()==QDialog::Accepted)
	{
		NGSD db;
		db.setPassword(LoginManager::userId(), dlg.password());
	}
}

void MainWindow::on_actionStudy_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("study");
	auto dlg = GUIHelper::createDialog(widget, "Study administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionGaps_triggered()
{
	GapClosingDialog dlg(this);
	dlg.exec();
}

void MainWindow::on_actionGenderXY_triggered()
{
	ExternalToolDialog dialog("Determine gender", "xy", this);
	dialog.exec();
}

void MainWindow::on_actionGenderHet_triggered()
{
	ExternalToolDialog dialog("Determine gender", "hetx", this);
	dialog.exec();
}

void MainWindow::on_actionGenderSRY_triggered()
{
	ExternalToolDialog dialog("Determine gender", "sry", this);
	dialog.exec();
}

void MainWindow::on_actionStatisticsBED_triggered()
{
	ExternalToolDialog dialog("BED file information", "", this);
	dialog.exec();
}

void MainWindow::on_actionSampleSimilarityGSvar_triggered()
{
	ExternalToolDialog dialog("Sample similarity", "gsvar", this);
	dialog.exec();
}

void MainWindow::on_actionSampleSimilarityVCF_triggered()
{
	ExternalToolDialog dialog("Sample similarity", "vcf", this);
	dialog.exec();
}

void MainWindow::on_actionSampleSimilarityBAM_triggered()
{
	ExternalToolDialog dialog("Sample similarity", "bam", this);
	dialog.exec();
}

void MainWindow::on_actionSampleAncestry_triggered()
{
	ExternalToolDialog dialog("Sample ancestry", "", this);
	dialog.exec();
}

void MainWindow::on_actionAnalysisStatus_triggered()
{
	//check if alread open
	for (int t=0; t<ui_.tabs->count(); ++t)
	{
		if (ui_.tabs->tabText(t)=="Analysis status")
		{
			ui_.tabs->setCurrentIndex(t);
			return;
		}
	}

	//open new
	AnalysisStatusWidget* widget = new AnalysisStatusWidget(this);
	connect(widget, SIGNAL(openProcessedSampleTab(QString)), this, SLOT(openProcessedSampleTab(QString)));
	connect(widget, SIGNAL(openRunTab(QString)), this, SLOT(openRunTab(QString)));
	connect(widget, SIGNAL(loadFile(QString)), this, SLOT(loadFile(QString)));
	openTab(QIcon(":/Icons/Server.png"), "Analysis status", widget);
}

void MainWindow::on_actionGapsLookup_triggered()
{
	if (filename_=="") return;

	AnalysisType type = variants_.type();
	if (type!=GERMLINE_SINGLESAMPLE && type!=GERMLINE_TRIO && type!=GERMLINE_MULTISAMPLE) return;

	QString ps_name = germlineReportSample();

	//check low-coverage file exists
	QStringList low_cov_files = GlobalServiceProvider::fileLocationProvider().getLowCoverageFiles(false).filterById(ps_name).asStringList();
	if (low_cov_files.isEmpty())
	{
		QMessageBox::warning(this, "Gap lookup", "No look-up of gaps is possible!\nCould not find a low-coverage file for sample " + ps_name + ".");
		return;
	}

	//get gene name from user
	QString gene = QInputDialog::getText(this, "Display gaps", "Gene:").trimmed();
	if (gene=="") return;

	//check if gene is in target region
	if (LoginManager::active())
	{
		NGSD db;
		QString ps_id = db.processedSampleId(variants_.mainSampleName());
		if (ps_id!="")
		{
			int sys_id = db.getValue("SELECT processing_system_id FROM processed_sample WHERE id=:0", true, ps_id).toInt();
			BedFile sys_regions = GlobalServiceProvider::database().processingSystemRegions(sys_id);
			if (!sys_regions.isEmpty())
			{
				BedFile region = db.geneToRegions(gene.toLatin1(), Transcript::ENSEMBL, "gene");
				region.merge();
				if (region.count()==0)
				{
					QMessageBox::warning(this, "Precalculated gaps for gene", "Error:\nCould not convert gene symbol '" + gene + "' to a genomic region.\nIs this a HGNC-approved gene name with associated transcripts?");
					return;
				}

				region.intersect(sys_regions);
				if (region.count()==0)
				{
					QMessageBox::warning(this, "Precalculated gaps for gene", "Error:\nGene '" + gene + "' locus does not overlap with sample target region!");
					return;
				}
			}
		}
	}

	//look up data in report
	QStringList output;
	QStringList lines = Helper::loadTextFile(low_cov_files[0], true);
	foreach(QString line, lines)
	{
		QStringList parts = line.split('\t');
		if(parts.count()==4 && parts[3].contains(gene, Qt::CaseInsensitive))
		{
			output.append(line);
		}
	}

	//show output
	QTextEdit* edit = new QTextEdit();
	edit->setText(output.join("\n"));
	edit->setMinimumWidth(500);
	edit->setWordWrapMode(QTextOption::NoWrap);
	edit->setReadOnly(true);
	auto dlg = GUIHelper::createDialog(edit, "Gaps of gene '" + gene + "' from low-coverage BED file for sample " + ps_name);
	dlg->exec();
}

void MainWindow::on_actionGapsRecalculate_triggered()
{
	if (filename_=="") return;

	//check for BAM file
	QString ps = germlineReportSample();
	QStringList bams = GlobalServiceProvider::fileLocationProvider().getBamFiles(false).filterById(ps).asStringList();
	if (bams.empty())
	{
		QMessageBox::warning(this, "Gaps error", "No BAM file found for sample " + ps + "!");
		return;
	}

	//determine ROI name, ROI and gene list
	BedFile roi;
	GeneSet genes;

	//check for ROI file
	if (ui_.filters->targetRegion().isValid())
	{
		roi = ui_.filters->targetRegion().regions;
		genes = ui_.filters->targetRegion().genes;
	}
	else if (LoginManager::active())
	{
		QMessageBox::StandardButton btn = QMessageBox::information(this, "Gaps error", "No target region filter set!<br>Do you want to look up gaps for a specific gene?", QMessageBox::Yes, QMessageBox::No);
		if (btn!=QMessageBox::Yes) return;

		QByteArray symbol = selectGene().toUtf8();
		if (symbol=="") return;

		QApplication::setOverrideCursor(Qt::BusyCursor);

		roi = NGSD().geneToRegions(symbol, Transcript::ENSEMBL, "exon", true, false);
		roi.extend(20);
		roi.merge();

		genes << symbol;

		QApplication::restoreOverrideCursor();
	}
	else
	{
		QMessageBox::warning(this, "Gaps error", "No target region filter set!");
		return;
	}

	//show dialog
	QStringList low_covs = GlobalServiceProvider::fileLocationProvider().getLowCoverageFiles(false).filterById(ps).asStringList();
	low_covs << ""; //add empty string in case there is no low-coverage file > this case is handled inside the dialog
	GapDialog dlg(this, ps, bams[0], low_covs[0], roi, genes);
	connect(&dlg, SIGNAL(openRegionInIGV(QString)), this, SLOT(openInIGV(QString)));
	dlg.exec();
}

void MainWindow::exportVCF()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		//generate GSvar with variants passing filter only
		VariantList selected_variants;
		selected_variants.copyMetaData(variants_);
		for(int i=0; i<variants_.count(); ++i)
		{
			if (!filter_result_.passing(i)) continue;
			selected_variants.append(variants_[i]);
		}

		//convert to VCF
		QString ref_genome = Settings::string("reference_genome", false);
		VcfFile vcf_file = VcfFile::convertGSvarToVcf(selected_variants, ref_genome);

		//store
		QString folder = Settings::path("gsvar_variant_export_folder", true);
		QString file_name = folder + QDir::separator() + QFileInfo(filename_).baseName() + "_export_" + QDate::currentDate().toString("yyyyMMdd") + "_" + Helper::userName() + ".vcf";

		file_name = QFileDialog::getSaveFileName(this, "Export VCF", file_name, "VCF (*.vcf);;All files (*.*)");
		if (file_name!="")
		{
			vcf_file.store(file_name);
			QApplication::restoreOverrideCursor();
			QMessageBox::information(this, "VCF export", "Exported VCF file with " + QString::number(vcf_file.count()) + " variants.");
		}
		else
		{
			QApplication::restoreOverrideCursor();
		}
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();

		QMessageBox::warning(this, "VCF export error", e.message());
	}
}

void MainWindow::exportGSvar()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		//create new GSvar file with passing variants
		VariantList output;
		output.copyMetaData(variants_);
		for(int i=0; i<variants_.count(); ++i)
		{
			if (filter_result_.passing(i))
			{
				output.append(variants_[i]);
			}
		}

		//store
		QString folder = Settings::path("gsvar_variant_export_folder", true);
		QString file_name = folder + QDir::separator() + QFileInfo(filename_).baseName() + "_export_" + QDate::currentDate().toString("yyyyMMdd") + "_" + Helper::userName() + ".GSvar";

		file_name = QFileDialog::getSaveFileName(this, "Export GSvar", file_name, "VCF (*.GSvar);;All files (*.*)");
		if (file_name!="")
		{
			output.store(file_name);
			QApplication::restoreOverrideCursor();
			QMessageBox::information(this, "GSvar export", "Exported GSvar file with " + QString::number(output.count()) + " variants.");
		}
		else
		{
			QApplication::restoreOverrideCursor();
		}

	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();

		QMessageBox::warning(this, "GSvar export error", e.message());
	}
}

void MainWindow::on_actionPreferredTranscripts_triggered()
{
	PreferredTranscriptsWidget* widget = new PreferredTranscriptsWidget();
	auto dlg = GUIHelper::createDialog(widget, "Preferred transcripts");
	dlg->exec();

	//re-load preferred transcripts from NGSD
	GSvarHelper::preferredTranscripts(true);
}

void MainWindow::on_actionEditSomaticGeneRoles_triggered()
{
	DBTableAdministration* table = new DBTableAdministration("somatic_gene_role");
	auto dlg = GUIHelper::createDialog(table, "Somatic Gene Roles");
	addModelessDialog(dlg);
}

void MainWindow::on_actionOpenDocumentation_triggered()
{
	QDesktopServices::openUrl(QUrl("https://github.com/imgag/ngs-bits/tree/master/doc/GSvar/index.md"));
}

void MainWindow::on_actionConvertHgnc_triggered()
{
	ApprovedGenesDialog dlg(this);
	dlg.exec();
}

void MainWindow::on_actionPhenoToGenes_triggered()
{
	try
	{
		PhenoToGenesDialog dlg(this);
		dlg.exec();
	}
	catch (DatabaseException& e)
	{
		QMessageBox::warning(this, "Database error", e.message());
	}
}

void MainWindow::on_actionGenesToRegions_triggered()
{
	GenesToRegionsDialog dlg(this);
	dlg.exec();
}

void MainWindow::openSubpanelDesignDialog(const GeneSet& genes)
{
	SubpanelDesignDialog dlg(this);
	dlg.setGenes(genes);

	dlg.exec();

	if (dlg.lastCreatedSubPanel()!="")
	{
		//update target region list
		ui_.filters->loadTargetRegions();

		//optinally use sub-panel as target regions
		if (QMessageBox::question(this, "Use sub-panel?", "Do you want to set the sub-panel as target region?")==QMessageBox::Yes)
		{
			ui_.filters->setTargetRegionByDisplayName(dlg.lastCreatedSubPanel());
		}
	}
}

void MainWindow::on_actionArchiveSubpanel_triggered()
{
	SubpanelArchiveDialog dlg(this);
	dlg.exec();
	if (dlg.changedSubpanels())
	{
		ui_.filters->loadTargetRegions();
	}
}

QString MainWindow::nobr()
{
	return "<p style='white-space:pre; margin:0; padding:0;'>";
}

void MainWindow::uploadtoLovd(int variant_index, int variant_index2)
{
	//(1) prepare data as far as we can (no RefSeq transcript data is available)
	LovdUploadData data;

	//sample name
	data.processed_sample = variants_.mainSampleName();

	//gender
	NGSD db;
	QString sample_id = db.sampleId(data.processed_sample);
	SampleData sample_data = db.getSampleData(sample_id);
	data.gender = sample_data.gender;

	//phenotype(s)
	data.phenos = sample_data.phenotypes;

	//data 1st variant
	const Variant& variant = variants_[variant_index];
	data.variant = variant;
	int genotype_index = variants_.getSampleHeader().infoByID(data.processed_sample).column_index;
	data.genotype = variant.annotations()[genotype_index];
	FastaFileIndex idx(Settings::string("reference_genome"));
	data.hgvs_g = variant.toHGVS(idx);
	int classification_index = variants_.annotationIndexByName("classification");
	data.classification = variant.annotations()[classification_index];
	int i_refseq = variants_.annotationIndexByName("coding_and_splicing_refseq", true, false);
	if (i_refseq!=-1)
	{
		data.trans_data = variant.transcriptAnnotations(i_refseq);
	}

	//data 2nd variant (comp-het)
	if (variant_index2!=-1)
	{
		const Variant& variant2 = variants_[variant_index2];
		data.variant2 = variant2;
		data.genotype2 = variant2.annotations()[genotype_index];
		data.hgvs_g2 = variant2.toHGVS(idx);
		data.classification2 = variant2.annotations()[classification_index];
		if (i_refseq!=-1)
		{
			data.trans_data2 = variant2.transcriptAnnotations(i_refseq);
		}
	}

	// (2) show dialog
	LovdUploadDialog dlg(this);
	dlg.setData(data);
	dlg.exec();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e)
{
	if (!e->mimeData()->hasFormat("text/uri-list")) return;
	if (e->mimeData()->urls().count()!=1) return;
	QUrl url = e->mimeData()->urls().at(0);
	if (!url.isLocalFile()) return;

	QString filename = url.toLocalFile();
	if (QFile::exists(filename) && filename.endsWith(".GSvar"))
	{
		e->acceptProposedAction();
	}
}

void MainWindow::dropEvent(QDropEvent* e)
{
	loadFile(e->mimeData()->urls().first().toLocalFile());
	e->accept();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	//unload the data
	loadFile();

	//here one could cancel closing the window by calling event->ignore()

	event->accept();
}

void MainWindow::refreshVariantTable(bool keep_widths)
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	QTime timer;
	timer.start();

	//apply filters
	applyFilters(false);
	int passing_variants = filter_result_.countPassing();
	QString status = QString::number(passing_variants) + " of " + QString::number(variants_.count()) + " variants passed filters.";
	int max_variants = 10000;
	if (passing_variants>max_variants)
	{
		status += " Displaying " + QString::number(max_variants) + " variants only!";
	}
	ui_.statusBar->showMessage(status);

	Log::perf("Applying all filters took ", timer);
	timer.start();

	//force update of variant details widget
	var_last_ = -1;

	//update variant table
	QList<int> col_widths = ui_.vars->columnWidths();
	AnalysisType type = variants_.type();
	if (type==SOMATIC_SINGLESAMPLE || type==SOMATIC_PAIR)
	{
		ui_.vars->update(variants_, filter_result_, somatic_report_settings_, max_variants);
	}
	else if (type==GERMLINE_SINGLESAMPLE || type==GERMLINE_TRIO || type==GERMLINE_MULTISAMPLE)
	{
		ui_.vars->update(variants_, filter_result_, report_settings_, max_variants);
	}
	else
	{
		THROW(ProgrammingException, "Unsupported analysis type in refreshVariantTable!");
	}

	ui_.vars->adaptRowHeights();
	if (keep_widths)
	{
		ui_.vars->setColumnWidths(col_widths);
	}
	else
	{
		ui_.vars->adaptColumnWidths();
	}
	QApplication::restoreOverrideCursor();

	Log::perf("Updating variant table took ", timer);
}

void MainWindow::varsContextMenu(QPoint pos)
{
	pos = ui_.vars->viewport()->mapToGlobal(pos);

	QList<int> indices = ui_.vars->selectedVariantsIndices();
	if (indices.count()==1)
	{
		contextMenuSingleVariant(pos, indices[0]);
	}
	else if (indices.count()==2)
	{
		contextMenuTwoVariants(pos, indices[0], indices[1]);
	}
}

void MainWindow::varHeaderContextMenu(QPoint pos)
{
	if (!LoginManager::active()) return;

	//get variant index
	int row = ui_.vars->verticalHeader()->visualIndexAt(pos.ry());
	int index = ui_.vars->rowToVariantIndex(row);

	//set up menu
	QMenu menu(ui_.vars->verticalHeader());
	QAction* a_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	QAction* a_delete =menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");

	if(germlineReportSupported())
	{
		a_delete->setEnabled(!report_settings_.report_config->isFinalized() && report_settings_.report_config->exists(VariantType::SNVS_INDELS, index));
	}
	else if(somaticReportSupported())
	{
		 a_delete->setEnabled(somatic_report_settings_.report_config.exists(VariantType::SNVS_INDELS, index));
	}
	else
	{
		a_delete->setEnabled(false);
	}

	//exec menu
	pos = ui_.vars->verticalHeader()->viewport()->mapToGlobal(pos);
	QAction* action = menu.exec(pos);
	if (action==nullptr) return;

	//actions
	if (action==a_edit)
	{
		editVariantReportConfiguration(index);
	}
	else if (action==a_delete)
	{
		if(germlineReportSupported())
		{
			report_settings_.report_config->remove(VariantType::SNVS_INDELS, index);
		}
		else
		{
			somatic_report_settings_.report_config.remove(VariantType::SNVS_INDELS, index);
		}
		updateReportConfigHeaderIcon(index);
	}
}

void MainWindow::contextMenuSingleVariant(QPoint pos, int index)
{
	//init
	bool  ngsd_user_logged_in = LoginManager::active();
	const Variant& variant = variants_[index];
	int i_gene = variants_.annotationIndexByName("gene", true, true);
	GeneSet genes = GeneSet::createFromText(variant.annotations()[i_gene], ',');
	int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, true);
	QList<VariantTranscript> transcripts = variant.transcriptAnnotations(i_co_sp);
	int i_dbsnp = variants_.annotationIndexByName("dbSNP", true, true);
	const QMap<QByteArray, QByteArrayList>& preferred_transcripts = GSvarHelper::preferredTranscripts();

	//create context menu
	QMenu menu(ui_.vars);

	//NGSD report configuration
	QAction* a_report_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	a_report_edit->setEnabled(ngsd_user_logged_in);
	QAction* a_report_del = menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
	a_report_del->setEnabled(ngsd_user_logged_in && ((!report_settings_.report_config->isFinalized() && report_settings_.report_config->exists(VariantType::SNVS_INDELS, index)) || somatic_report_settings_.report_config.exists(VariantType::SNVS_INDELS, index)));
	menu.addSeparator();

	//NGSD variant options
	QAction* a_var_class = menu.addAction("Edit classification");
	a_var_class->setEnabled(ngsd_user_logged_in);
	QAction* a_var_class_somatic = menu.addAction("Edit classification  (somatic)");
	a_var_class_somatic->setEnabled(ngsd_user_logged_in);
	QAction * a_var_interpretation_somatic = menu.addAction("Edit VICC interpretation (somatic)");
	a_var_interpretation_somatic->setEnabled(ngsd_user_logged_in);
	QAction* a_var_comment = menu.addAction("Edit comment");
	a_var_comment->setEnabled(ngsd_user_logged_in);
	QAction* a_var_val = menu.addAction("Perform variant validation");
	a_var_val->setEnabled(ngsd_user_logged_in);
	menu.addSeparator();

	//Google
	QMenu* sub_menu = menu.addMenu(QIcon("://Icons/Google.png"), "Google");
	foreach(const VariantTranscript& trans, transcripts)
	{
		QAction* action = sub_menu->addAction(trans.gene + " " + trans.idWithoutVersion() + " " + trans.hgvs_c + " " + trans.hgvs_p);
		if (preferred_transcripts.value(trans.gene).contains(trans.idWithoutVersion()))
		{
			QFont font = action->font();
			font.setBold(true);
			action->setFont(font);
		}
	}

	//Alamut
	if (Settings::string("Alamut")!="")
	{
		sub_menu = menu.addMenu(QIcon("://Icons/Alamut.png"), "Alamut");

		//BAM
		if (variants_.type()==GERMLINE_SINGLESAMPLE)
		{
			sub_menu->addAction("BAM");
		}

		//genomic location
		QString loc = variant.chr().str() + ":" + QByteArray::number(variant.start());
		loc.replace("chrMT", "chrM");
		sub_menu->addAction(loc);
		sub_menu->addAction(loc + variant.ref() + ">" + variant.obs());

		//genes
		foreach(const QByteArray& g, genes)
		{
			sub_menu->addAction(g);
		}
		sub_menu->addSeparator();

		//transcripts
		foreach(const VariantTranscript& transcript, transcripts)
		{
			if  (transcript.id!="" && transcript.hgvs_c!="")
			{
				QAction* action = sub_menu->addAction(transcript.idWithoutVersion() + ":" + transcript.hgvs_c + " (" + transcript.gene + ")");

				//highlight preferred transcripts
				if (preferred_transcripts.value(transcript.gene).contains(transcript.idWithoutVersion()))
				{
					QFont font = action->font();
					font.setBold(true);
					action->setFont(font);
				}
			}
		}
	}

	//UCSC
	QAction* a_ucsc = menu.addAction(QIcon("://Icons/UCSC.png"), "Open in UCSC browser");

	//LOVD upload
	sub_menu = menu.addMenu(QIcon("://Icons/LOVD.png"), "LOVD");
	QAction* a_lovd_find = sub_menu->addAction("Find in LOVD");
	QAction* a_lovd_pub = sub_menu->addAction("Publish in LOVD");
	a_lovd_pub->setEnabled(ngsd_user_logged_in);

	//MitoMap
	QAction* a_mitomap = menu.addAction(QIcon("://Icons/MitoMap.png"), "Open in MitoMap");
	a_mitomap->setEnabled(variant.chr().isM());

	//varsome
	QAction* a_varsome =  menu.addAction(QIcon("://Icons/VarSome.png"), "VarSome");

	//PubMed
	sub_menu = menu.addMenu(QIcon("://Icons/PubMed.png"), "PubMed");
	if (ngsd_user_logged_in)
	{
		NGSD db;
		QString ps = germlineReportSupported() ? germlineReportSample() : variants_.mainSampleName();
		QString sample_id = db.sampleId(ps, false);
		if (sample_id!="")
		{
			//get disease list (HPO and CGI cancer type)
			QByteArrayList diseases;
			QList<SampleDiseaseInfo> infos = db.getSampleDiseaseInfo(sample_id);
			foreach(const SampleDiseaseInfo& info, infos)
			{
				if (info.type=="HPO term id")
				{
					int id = db.phenotypeIdByAccession(info.disease_info.toLatin1(), false);
					if (id!=-1)
					{
						QByteArray disease = db.phenotype(id).name().trimmed();
						if (!diseases.contains(disease))
						{
							diseases << disease;
						}
					}
				}
				else if (info.type=="CGI cancer type")
				{
					TSVFileStream stream("://Resources/cancer_types.tsv");
					int idx_id = stream.colIndex("ID",true);
					int idx_name = stream.colIndex("NAME",true);
					while(!stream.atEnd())
					{
						QByteArrayList line = stream.readLine();
						if (line.at(idx_id)==info.disease_info)
						{
							QByteArray disease = line.at(idx_name).trimmed();
							if (!diseases.contains(disease) && !disease.isEmpty())
							{
								diseases << disease;
							}
						}
					}
				}
			}

			//create links for each gene/disease
			foreach(const QByteArray& g, genes)
			{
				foreach(const QByteArray& d, diseases)
				{
					sub_menu->addAction(g + " AND \"" + d + "\"");
				}
			}
		}
	}
	else
	{
		sub_menu->setEnabled(ngsd_user_logged_in);
	}

	//add gene databases
	if (!genes.isEmpty())
	{
		menu.addSeparator();
		foreach(const QByteArray& g, genes)
		{
			sub_menu = menu.addMenu(g);
			sub_menu->addAction(QIcon("://Icons/NGSD_gene.png"), "Gene tab")->setEnabled(ngsd_user_logged_in);
			sub_menu->addAction(QIcon("://Icons/Google.png"), "Google");
			foreach(const GeneDB& db, GeneInfoDBs::all())
			{
				sub_menu->addAction(db.icon, db.name);
			}
		}
	}

	//add custom entries
	QString custom_menu_small_variants = Settings::string("custom_menu_small_variants", true).trimmed();
	if (!custom_menu_small_variants.isEmpty())
	{
		sub_menu = menu.addMenu("Custom");
		QStringList custom_entries = custom_menu_small_variants.split("\t");
		foreach(QString custom_entry, custom_entries)
		{
			QStringList parts = custom_entry.split("|");
			if (parts.count()==2)
			{
				sub_menu->addAction(parts[0]);
			}
		}
	}

	//execute menu
	QAction* action = menu.exec(pos);
	if (!action) return;

	//perform actions
	QByteArray text = action->text().toLatin1();
	QMenu* parent_menu = qobject_cast<QMenu*>(action->parent());

	if (action==a_var_class)
	{
		editVariantClassification(variants_, index);
	}
	else if (action==a_var_class_somatic)
	{
		editVariantClassification(variants_, index, true);
	}
	else if (action==a_var_interpretation_somatic)
	{
		editSomaticVariantInterpretation(variants_, index);
	}
	else if (action==a_var_comment)
	{
		editVariantComment(index);
	}
	else if (action==a_var_val)
	{
		editVariantValidation(index);
	}
	else if (action==a_ucsc)
	{
		QDesktopServices::openUrl(QUrl("https://genome.ucsc.edu/cgi-bin/hgTracks?db=hg19&position=" + variant.chr().str()+":"+QString::number(variant.start()-20)+"-"+QString::number(variant.end()+20)));
	}
	else if (action==a_lovd_find)
	{
		int pos = variant.start();
		if (variant.ref()=="-") pos += 1;
		QDesktopServices::openUrl(QUrl("https://databases.lovd.nl/shared/variants#search_chromosome=" + variant.chr().strNormalized(false)+"&search_VariantOnGenome/DNA=g." + QString::number(pos)));
	}
	else if (action==a_mitomap)
	{
		QDesktopServices::openUrl(QUrl("https://www.mitomap.org/cgi-bin/search_allele?starting="+QString::number(variant.start())+"&ending="+QString::number(variant.end())));
	}
	else if (action==a_lovd_pub)
	{
		try
		{
			uploadtoLovd(index);
		}
		catch (Exception& e)
		{
			GUIHelper::showMessage("LOVD upload error", "Error while uploading variant to LOVD: " + e.message());
			return;
		}
	}
	else if (parent_menu && parent_menu->title()=="Alamut")
	{
		//documentation of the alamut API:
		// - http://www.interactive-biosoftware.com/doc/alamut-visual/2.9/accessing.html
		// - http://www.interactive-biosoftware.com/doc/alamut-visual/2.11/Alamut-HTTP.html
		// - http://www.interactive-biosoftware.com/doc/alamut-visual/2.9/programmatic-access.html
		QStringList parts = action->text().split(" ");
		if (parts.count()>=1)
		{
			QString value = parts[0];
			if (value=="BAM")
			{
				QStringList bams = GlobalServiceProvider::fileLocationProvider().getBamFiles(false).filterById(germlineReportSample()).asStringList();
				if (bams.empty()) return;
				value = "BAM<" + bams[0];
			}

			try
			{
				HttpHandler(HttpRequestHandler::NONE).get(Settings::string("Alamut")+"/show?request="+value);
			}
			catch (Exception& e)
			{
				QMessageBox::warning(this, "Communication with Alamut failed!", e.message());
			}
		}
	}
	else if (parent_menu && parent_menu->title()=="Google")
	{
		QByteArray query;
		QByteArrayList parts = text.split(' ');
		QByteArray gene = parts[0].trimmed();
		QByteArray hgvs_c = parts[2].trimmed();
		QByteArray hgvs_p = parts[3].trimmed();
		query = gene + " AND (\"" + hgvs_c.mid(2) + "\" OR \"" + hgvs_c.mid(2).replace(">", "->") + "\" OR \"" + hgvs_c.mid(2).replace(">", "-->") + "\" OR \"" + hgvs_c.mid(2).replace(">", "/") + "\"";
		if (hgvs_p!="")
		{
			query += " OR \"" + hgvs_p.mid(2) + "\"";
		}
		QByteArray dbsnp = variant.annotations()[i_dbsnp].trimmed();
		if (dbsnp!="")
		{
			query += " OR \"" + dbsnp + "\"";
		}
		query += ")";

		QDesktopServices::openUrl(QUrl("https://www.google.com/search?q=" + query.replace("+", "%2B").replace(' ', '+')));
	}
	else if (action==a_varsome)
	{
		QString ref = variant.ref();
		ref.replace("-", "");
		QString obs = variant.obs();
		obs.replace("-", "");
		QString var = variant.chr().str() + "-" + QString::number(variant.start()) + "-" +  ref + "-" + obs;
		QString genome = variant.chr().isM() ? "hg38" : "hg19";
		QDesktopServices::openUrl(QUrl("https://varsome.com/variant/" + genome + "/" + var));
	}
	else if (action==a_report_edit)
	{
		editVariantReportConfiguration(index);
	}
	else if (action==a_report_del)
	{
		if(germlineReportSupported())
		{
			report_settings_.report_config->remove(VariantType::SNVS_INDELS, index);
		}
		else if(somaticReportSupported())
		{
			somatic_report_settings_.report_config.remove(VariantType::SNVS_INDELS, index);
			storeSomaticReportConfig();
		}

		updateReportConfigHeaderIcon(index);
	}
	else if (parent_menu && parent_menu->title()=="PubMed")
	{
		QDesktopServices::openUrl(QUrl("https://pubmed.ncbi.nlm.nih.gov/?term=" + text));
	}
	else if (parent_menu && parent_menu->title()=="Custom")
	{
		QStringList custom_entries = Settings::string("custom_menu_small_variants", true).trimmed().split("\t");
		foreach(QString custom_entry, custom_entries)
		{
			QStringList parts = custom_entry.split("|");
			if (parts.count()==2 && parts[0]==text)
			{
				QString url = parts[1];
				url.replace("[chr]", variant.chr().strNormalized(true));
				url.replace("[start]", QString::number(variant.start()));
				url.replace("[end]", QString::number(variant.end()));
				url.replace("[ref]", variant.ref());
				url.replace("[obs]", variant.obs());
				QDesktopServices::openUrl(QUrl(url));
			}
		}
	}
	else if (parent_menu) //gene menus
	{
		QString gene = parent_menu->title();

		if (text=="Gene tab")
		{
			openGeneTab(gene);
		}
		else if (text=="Google")
		{
			QString query = gene + " AND (mutation";
			foreach(const Phenotype& pheno, ui_.filters->phenotypes())
			{
				query += " OR \"" + pheno.name() + "\"";
			}
			query += ")";

			QDesktopServices::openUrl(QUrl("https://www.google.com/search?q=" + query.replace("+", "%2B").replace(' ', '+')));
		}
		else //other databases
		{
			GeneInfoDBs::openUrl(text, gene);
		}
	}
}

void MainWindow::contextMenuTwoVariants(QPoint pos, int index1, int index2)
{
	//create context menu
	QMenu menu(ui_.vars);
	QAction* a_lovd = menu.addAction(QIcon("://Icons/LOVD.png"), "Publish in LOVD (comp-het)");

	//execute
	QAction* action = menu.exec(pos);
	if (!action) return;

	//react
	if (action==a_lovd)
	{
		try
		{
			uploadtoLovd(index1, index2);
		}
		catch (Exception& e)
		{
			GUIHelper::showMessage("LOVD upload error", "Error while uploading variant to LOVD: " + e.message());
			return;
		}
	}
}

void MainWindow::editVariantClassification(VariantList& variants, int index, bool is_somatic)
{
	try
	{
		Variant& variant = variants[index];

		//execute dialog
		ClassificationDialog dlg(this, variant, is_somatic);
		if (dlg.exec()!=QDialog::Accepted) return;

		//update NGSD
		NGSD db;

		ClassificationInfo class_info = dlg.classificationInfo();
		if(is_somatic)
		{
			db.setSomaticClassification(variant, class_info);

			//update variant list classification
			int i_som_class = variants.annotationIndexByName("somatic_classification");
			QString new_class = class_info.classification.replace("n/a", "");
			variant.annotations()[i_som_class] = new_class.toLatin1();

			markVariantListChanged(variant, "somatic_classification", new_class);

			//update variant list classification comment
			int i_som_class_comment = variants.annotationIndexByName("somatic_classification_comment");
			variant.annotations()[i_som_class_comment] = class_info.comments.toLatin1();

			markVariantListChanged(variant, "somatic_classification_comment", class_info.comments);

		}
		else //germline variants
		{
			db.setClassification(variant, variants_, class_info);

			//update variant list classification
			int i_class = variants.annotationIndexByName("classification");
			QString new_class = class_info.classification.replace("n/a", "");
			variant.annotations()[i_class] = new_class.toLatin1();

			markVariantListChanged(variant, "classification", new_class);

			//update variant list classification comment
			int i_class_comment = variants.annotationIndexByName("classification_comment");
			variant.annotations()[i_class_comment] = class_info.comments.toLatin1();

			markVariantListChanged(variant, "classification_comment", class_info.comments);
		}

		//update details widget and filtering
		ui_.variant_details->updateVariant(variants, index);
		refreshVariantTable();

	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
		return;
	}
}

void MainWindow::editSomaticVariantInterpretation(const VariantList &vl, int index)
{
	SomaticVariantInterpreterWidget* interpreter = new SomaticVariantInterpreterWidget(index, vl, this);
	auto dlg = GUIHelper::createDialog(interpreter, "Somatic Variant Interpretation");
	connect(interpreter, SIGNAL(stored(int, QString, QString)), this, SLOT(updateSomaticVariantInterpretationAnno(int, QString, QString)) );

	dlg->exec();
}

void MainWindow::updateSomaticVariantInterpretationAnno(int index, QString vicc_interpretation, QString vicc_comment)
{
	int i_vicc = variants_.annotationIndexByName("NGSD_som_vicc_interpretation");
	variants_[index].annotations()[i_vicc] = vicc_interpretation.toUtf8();

	markVariantListChanged(variants_[index], "NGSD_som_vicc_interpretation", vicc_interpretation);

	int i_vicc_comment = variants_.annotationIndexByName("NGSD_som_vicc_comment");
	variants_[index].annotations()[i_vicc_comment] = vicc_comment.toUtf8();

	markVariantListChanged(variants_[index], "NGSD_som_vicc_comment", vicc_comment);

	//update details widget and filtering
	ui_.variant_details->updateVariant(variants_, index);
	refreshVariantTable();
}

void MainWindow::on_actionAnnotateSomaticVariantInterpretation_triggered()
{
	if (filename_.isEmpty()) return;
	if (!LoginManager::active()) return;

	int i_vicc = variants_.annotationIndexByName("NGSD_som_vicc_interpretation");
	int i_vicc_comment = variants_.annotationIndexByName("NGSD_som_vicc_comment");

	NGSD db;
	for(int i=0; i<variants_.count(); ++i)
	{
		//skip variants without VICC infos in NGSD
		SomaticViccData vicc_data = db.getSomaticViccData(variants_[i], false);
		if (vicc_data.created_by.isEmpty()) continue;

		//update score
		QByteArray vicc_score = SomaticVariantInterpreter::viccScoreAsString(vicc_data).toUtf8();
		if (vicc_score!=variants_[i].annotations()[i_vicc])
		{
			variants_[i].annotations()[i_vicc] = vicc_score;
			markVariantListChanged(variants_[i], "NGSD_som_vicc_interpretation", vicc_score);
		}

		//update comment
		QByteArray vicc_comment = vicc_data.comment.toUtf8();
		if (variants_[i].annotations()[i_vicc_comment]!=vicc_comment)
		{
			variants_[i].annotations()[i_vicc_comment]= vicc_comment;
			markVariantListChanged(variants_[i], "NGSD_som_vicc_comment", vicc_comment);
		}
	}

	//update details widget and filtering
	refreshVariantTable();
}

bool MainWindow::germlineReportSupported(bool require_ngsd)
{
	//no file loaded
	if (filename_.isEmpty()) return false;

	//user has to be logged in
	if (require_ngsd && !LoginManager::active()) return false;

	//single and trio (~one affected)
	AnalysisType type = variants_.type();
	if (type==GERMLINE_SINGLESAMPLE || type==GERMLINE_TRIO) return true;

	//multi-sample only with at least one affected
	if (type==GERMLINE_MULTISAMPLE && variants_.getSampleHeader().sampleColumns(true).count()>=1) return true;

	return false;}

QString MainWindow::germlineReportSample()
{
	if (!germlineReportSupported(false))
	{
		THROW(ProgrammingException, "germlineReportSample() cannot be used if germline report is not supported!");
	}

	//set sample for report
	while (germline_report_ps_.isEmpty())
	{
		//determine affected sample names
		QStringList affected_ps;
		foreach(const SampleInfo& info, variants_.getSampleHeader())
		{
			if(info.isAffected())
			{
				affected_ps << info.id.trimmed();
			}
		}

		if (affected_ps.isEmpty()) //no affected => error
		{
			THROW(ProgrammingException, "germlineReportSample() cannot be used if there is no affected sample!");
		}
		else if (affected_ps.count()==1) //one affected => auto-select
		{
			germline_report_ps_ = affected_ps[0];
		}
		else //several affected => let user select
		{
			bool ok = false;
			QString selected = QInputDialog::getItem(this, "Report sample", "processed sample used for report:", affected_ps, 0, false, &ok);
			if (ok)
			{
				germline_report_ps_ = selected;
			}
		}
	}

	return germline_report_ps_;
}

bool MainWindow::somaticReportSupported()
{
	return variants_.type()==SOMATIC_PAIR;
}

bool MainWindow::tumoronlyReportSupported()
{
	return variants_.type()==SOMATIC_SINGLESAMPLE;
}

void MainWindow::updateVariantDetails()
{
	int var_current = ui_.vars->selectedVariantIndex();
	if (var_current==-1) //no several variant => clear
	{
		ui_.variant_details->clear();
	}
	else if (var_current!=var_last_) //update variant details (if changed)
	{
		ui_.variant_details->updateVariant(variants_, var_current);
	}

	var_last_ = var_current;
}

void MainWindow::executeIGVCommands(QStringList commands, bool init_if_not_done)
{
	bool debug = false;

	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		//connect
		QAbstractSocket socket(QAbstractSocket::UnknownSocketType, this);
		QString igv_host = Settings::string("igv_host");
		int igv_port = igvPort();
		if (debug) qDebug() << QDateTime::currentDateTime() << "CONNECTING:" << igv_host << igv_port;
		socket.connectToHost(igv_host, igv_port);
		if (!socket.waitForConnected(1000))
		{
			//show message to user
			QApplication::restoreOverrideCursor();
			if (QMessageBox::information(this, "IGV not running", "IGV is not running on port " + QString::number(igv_port) + ".\nIt will be started now!", QMessageBox::Ok|QMessageBox::Default, QMessageBox::Cancel|QMessageBox::Escape)!=QMessageBox::Ok) return;
			QApplication::setOverrideCursor(Qt::BusyCursor);

			if (debug) qDebug() << QDateTime::currentDateTime() << "FAILED - TRYING TO START IGV";
			igv_initialized_ = false;

			//try to start IGV
			QString igv_app = Settings::path("igv_app").trimmed();
			if (igv_app.isEmpty())
			{
				THROW(Exception, "Could not start IGV: No settings entry for 'igv_app' found!");
			}
			if (!QFile::exists(igv_app))
			{
				THROW(Exception, "Could not start IGV: IGV application '" + igv_app + "' does not exist!");
			}
			bool started = QProcess::startDetached(igv_app + " --port " + QString::number(igv_port));
			if (!started)
			{
				THROW(Exception, "Could not start IGV: IGV application '" + igv_app + "' did not start!");
			}
			if (debug) qDebug() << QDateTime::currentDateTime() << "STARTED IGV - WAITING UNTIL CONNECTING TO THE PORT WORKS";

			//wait for IGV to respond after start
			bool connected = false;
			QDateTime max_wait = QDateTime::currentDateTime().addSecs(40);
			while (QDateTime::currentDateTime() < max_wait)
			{
				socket.connectToHost(igv_host, igv_port);
				if (socket.waitForConnected(1000))
				{
					if (debug) qDebug() << QDateTime::currentDateTime() << "CONNECTING TO THE PORT WORKS";
					connected = true;
					break;
				}
			}
			if (!connected)
			{
				THROW(Exception, "Could not start IGV: IGV application '" + igv_app + "' started, but does not respond!");
			}
		}
		QApplication::restoreOverrideCursor();

		//init if necessary
		if (!igv_initialized_ && init_if_not_done)
		{
			if (debug) qDebug() << QDateTime::currentDateTime() << "INITIALIZING IGV FOR CURRENT SAMPLE!";
			if (!initializeIGV(socket)) return;
		}

		//execute commands
		QApplication::setOverrideCursor(Qt::BusyCursor);
		foreach(QString command, commands)
		{
			if (debug) qDebug() << QDateTime::currentDateTime() << "EXECUTING:" << command;
			socket.write((command + "\n").toLatin1());
			bool ok = socket.waitForReadyRead(180000); // 3 min timeout (trios can be slow)
			QString answer = socket.readAll().trimmed();
			if (!ok || answer!="OK")
			{
				if (debug) qDebug() << QDateTime::currentDateTime() << "FAILED: answer:" << answer << " socket error:" << socket.errorString();
				THROW(Exception, "Could not execute IGV command '" + command + "'.\nAnswer: " + answer + "\nSocket error:" + socket.errorString());
			}
			else
			{
				if (debug) qDebug() << QDateTime::currentDateTime() << "DONE";
			}
		}

		//disconnect
		socket.disconnectFromHost();
		QApplication::restoreOverrideCursor();
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();

		QMessageBox::warning(this, "Error while sending command to IGV", e.message());
	}
}

void MainWindow::editVariantReportConfiguration(int index)
{
	if (!germlineReportSupported() && !somaticReportSupported())
	{
		QMessageBox::information(this, "Report configuration error", "Report configuration not supported for this type of analysis!");
		return;
	}

	NGSD db;

	if(germlineReportSupported()) //germline report configuration
	{
		//init/get config
		ReportVariantConfiguration var_config;
		bool report_settings_exist = report_settings_.report_config->exists(VariantType::SNVS_INDELS, index);
		if (report_settings_exist)
		{
			var_config = report_settings_.report_config->get(VariantType::SNVS_INDELS, index);
		}
		else
		{
			var_config.variant_index = index;
		}

		//get inheritance mode by gene
		const Variant& variant = variants_[index];
		QList<KeyValuePair> inheritance_by_gene;
		int i_genes = variants_.annotationIndexByName("gene", true, false);

		if (i_genes!=-1)
		{
			QByteArrayList genes = variant.annotations()[i_genes].split(',');
			foreach(QByteArray gene, genes)
			{
				GeneInfo gene_info = db.geneInfo(gene);
				inheritance_by_gene << KeyValuePair{gene, gene_info.inheritance};
			}
		}

		//exec dialog
		ReportVariantDialog dlg(variant.toString(false, 10), inheritance_by_gene, var_config, this);
		dlg.setEnabled(!report_settings_.report_config->isFinalized());
		if (dlg.exec()!=QDialog::Accepted) return;


		//update config, GUI and NGSD
		report_settings_.report_config->set(var_config);
		updateReportConfigHeaderIcon(index);

		//force classification of causal variants
		if(var_config.causal)
		{
			const Variant& variant = variants_[index];
			ClassificationInfo classification_info = db.getClassification(variant);
			if (classification_info.classification=="" || classification_info.classification=="n/a")
			{
				QMessageBox::warning(this, "Variant classification required!", "Causal variants should have a classification!", QMessageBox::Ok, QMessageBox::NoButton);
				editVariantClassification(variants_, index);
			}
		}
	}
	else if(somaticReportSupported()) //somatic report variant configuration
	{
		SomaticReportVariantConfiguration var_config;
		bool settings_exists = somatic_report_settings_.report_config.exists(VariantType::SNVS_INDELS, index);
		if(settings_exists)
		{
			var_config = somatic_report_settings_.report_config.get(VariantType::SNVS_INDELS, index);
		}
		else
		{
			var_config.variant_index = index;
		}

		SomaticReportVariantDialog* dlg = new SomaticReportVariantDialog(variants_[index].toString(), var_config, this);

		if(dlg->exec() != QDialog::Accepted) return;
		somatic_report_settings_.report_config.set(var_config);

		storeSomaticReportConfig();
		updateReportConfigHeaderIcon(index);
	}
}

void MainWindow::updateReportConfigHeaderIcon(int index)
{
	if(germlineReportSupported())
	{
		//report config-based filter is on => update whole variant list
		if (ui_.filters->reportConfigurationFilter()!=ReportConfigFilter::NONE)
		{
			refreshVariantTable();
		}
		else //no filter => refresh icon only
		{
			ui_.vars->updateVariantHeaderIcon(report_settings_, index);
		}
	}
	else if(somaticReportSupported())
	{
		if(!ui_.filters->targetRegion().isValid() || ui_.filters->filters().count() > 0)
		{
			refreshVariantTable();
		}
		else
		{
			ui_.vars->updateVariantHeaderIcon(somatic_report_settings_, index);
		}
	}
}

void MainWindow::markVariantListChanged(const Variant& variant, QString column, QString text)
{
	variants_changed_ << VariantListChange{variant, column, text};
}

void MainWindow::storeCurrentVariantList()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	if (GlobalServiceProvider::fileLocationProvider().isLocal())
	{
		try
		{
			//store to temporary file
			QString tmp = filename_ + ".tmp";
			variants_.store(tmp);

			//copy temp
			QFile::remove(filename_);
			QFile::rename(tmp, filename_);

			variants_changed_.clear();
		}
		catch(Exception& e)
		{
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, "Error storing GSvar file", "The GSvar file could not be stored:\n" + e.message());
		}
	}
	else
	{
		//TODO GSvarServer: add a end-point to update the GSvar file on the server - use data from variants_changed_
	}

	QApplication::restoreOverrideCursor();
}

void MainWindow::checkPendingVariantValidations()
{
	if (!LoginManager::active()) return;

	NGSD db;
	QStringList vv_pending = db.getValues("SELECT id FROM variant_validation WHERE status='for reporting' AND user_id='" + LoginManager::userIdAsString() + "'");
	if (vv_pending.isEmpty()) return;

	showNotification("Variant validation: " + QString::number(vv_pending.count()) + " pending variants 'for reporing'!");
}

void MainWindow::showNotification(QString text)
{
	text = text.trimmed();

	//update tooltip
	QStringList tooltips = notification_label_->toolTip().split("\n", QString::SkipEmptyParts);
	if (!tooltips.contains(text)) tooltips.prepend(text);
	notification_label_->setToolTip(tooltips.join("<br>"));

	//show popup
	notification_label_->show();
	QPoint pos = ui_.statusBar->mapToGlobal(notification_label_->pos()) + QPoint(8,8);
	QToolTip::showText(pos, text);
}

void MainWindow::variantRanking()
{
	if (filename_.isEmpty()) return;
	if (!LoginManager::active()) return;

	QApplication::setOverrideCursor(Qt::BusyCursor);	
	QString ps_name = germlineReportSample();
	try
	{
		NGSD db;

		//create phenotype list
		QHash<Phenotype, BedFile> phenotype_rois;
		QString sample_id = db.sampleId(ps_name);
		PhenotypeList phenotypes = ui_.filters->phenotypes();
		if (phenotypes.isEmpty())
		{
			phenotypes = db.getSampleData(sample_id).phenotypes;
		}
		foreach(const Phenotype& pheno, phenotypes)
		{
			//pheno > genes
			GeneSet genes = db.phenotypeToGenes(db.phenotypeIdByAccession(pheno.accession()), true);

			//genes > roi
			BedFile roi;
			foreach(const QByteArray& gene, genes)
			{
				if (!gene2region_cache_.contains(gene))
				{
					BedFile tmp = db.geneToRegions(gene, Transcript::ENSEMBL, "gene", true);
					tmp.clearAnnotations();
					tmp.extend(5000);
					tmp.merge();
					gene2region_cache_[gene] = tmp;
				}
				roi.add(gene2region_cache_[gene]);
			}
			roi.merge();

			phenotype_rois[pheno] = roi;
		}

		//score
		VariantScores::Result result = VariantScores::score("GSvar_v1", variants_, phenotype_rois, VariantScores::blacklist());

		//update variant list
		VariantScores::annotate(variants_, result, true);
		ui_.filters->reset(true);
		ui_.filters->setFilter("GSvar score/rank");

		QApplication::restoreOverrideCursor();

		//show warnings
		if (result.warnings.count()>0)
		{
			QMessageBox::warning(this, "Variant ranking", "Please note the following warnings:\n" + result.warnings.join("\n"));
		}
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, "Ranking variants", "An error occurred:\n" + e.message());
	}
}

void MainWindow::clearSomaticReportSettings(QString ps_id_in_other_widget)
{
	if(!LoginManager::active()) return;
	QString this_ps_id = NGSD().processedSampleId(variants_.mainSampleName(),false);

	if(this_ps_id == "") return;

	if(this_ps_id != ps_id_in_other_widget) return; //skip if ps id of file is different than in other widget
	somatic_report_settings_ = SomaticReportSettings();
	refreshVariantTable();
}

void MainWindow::applyFilters(bool debug_time)
{
	try
	{
		//apply main filter
		QTime timer;
		timer.start();

		const FilterCascade& filter_cascade = ui_.filters->filters();

		filter_result_ = filter_cascade.apply(variants_, false, debug_time);

		ui_.filters->markFailedFilters();

		if (debug_time)
		{
			Log::perf("Applying annotation filters took ", timer);
			timer.start();
		}

		//roi filter
		if (ui_.filters->targetRegion().isValid())
		{
			FilterRegions::apply(variants_, ui_.filters->targetRegion().regions, filter_result_);

			if (debug_time)
			{
				Log::perf("Applying target region filter took ", timer);
				timer.start();
			}
		}

		//gene filter
		GeneSet genes_filter = ui_.filters->genes();
		if (!genes_filter.isEmpty())
		{
			FilterGenes filter;
			filter.setStringList("genes", genes_filter.toStringList());
			filter.apply(variants_, filter_result_);

			if (debug_time)
			{
				Log::perf("Applying gene filter took ", timer);
				timer.start();
			}
		}

		//text filter
		QByteArray text = ui_.filters->text();
		if (!text.isEmpty())
		{
			FilterAnnotationText filter;
			filter.setString("term", text);
			filter.setString("action", "FILTER");
			filter.apply(variants_, filter_result_);

			if (debug_time)
			{

				Log::perf("Applying text filter took ", timer);
				timer.start();
			}
		}

		//target region filter
		QString region_text = ui_.filters->region();
		BedLine region = BedLine::fromString(region_text);
		if (!region.isValid()) //check if valid chr
		{
			Chromosome chr(region_text);
			if (chr.isNonSpecial())
			{
				region.setChr(chr);
				region.setStart(1);
				region.setEnd(999999999);
			}
		}
		if (region.isValid()) //valid region (chr,start, end or only chr)
		{
			BedFile tmp;
			tmp.append(region);
			FilterRegions::apply(variants_, tmp, filter_result_);

			if (debug_time)
			{
				Log::perf("Applying region filter took ", timer);
				timer.start();
			}
		}

		//phenotype selection changed => update ROI
		const PhenotypeList& phenos = ui_.filters->phenotypes();
		if (phenos!=last_phenos_)
		{
			last_phenos_ = phenos;

			//convert phenotypes to genes
			NGSD db;
			GeneSet pheno_genes;
			foreach(const Phenotype& pheno, phenos)
			{
				pheno_genes << db.phenotypeToGenes(db.phenotypeIdByAccession(pheno.accession()), true);
			}

			//convert genes to ROI (using a cache to speed up repeating queries)
			last_phenos_roi_.clear();
			foreach(const QByteArray& gene, pheno_genes)
			{
				if (!gene2region_cache_.contains(gene))
				{
					BedFile tmp = db.geneToRegions(gene, Transcript::ENSEMBL, "gene", true);
					tmp.clearAnnotations();
					tmp.extend(5000);
					tmp.merge();
					gene2region_cache_[gene] = tmp;
				}
				last_phenos_roi_.add(gene2region_cache_[gene]);
			}
			last_phenos_roi_.merge();

			if (debug_time)
			{
				Log::perf("Updating phenotype filter took ", timer);
				timer.start();
			}
		}

		//phenotype filter
		if (!last_phenos_.isEmpty())
		{
			FilterRegions::apply(variants_, last_phenos_roi_, filter_result_);

			if (debug_time)
			{
				Log::perf("Applying phenotype filter took ", timer);
				timer.start();
			}
		}

		//report configuration filter (show only variants with report configuration)
		ReportConfigFilter rc_filter = ui_.filters->reportConfigurationFilter();
		if (germlineReportSupported() && rc_filter!=ReportConfigFilter::NONE)
		{
			QSet<int> report_variant_indices = report_settings_.report_config->variantIndices(VariantType::SNVS_INDELS, false).toSet();
			for(int i=0; i<variants_.count(); ++i)
			{
				if (!filter_result_.flags()[i]) continue;

				if (rc_filter==ReportConfigFilter::HAS_RC)
				{
					filter_result_.flags()[i] = report_variant_indices.contains(i);
				}
				else if (rc_filter==ReportConfigFilter::NO_RC)
				{
					filter_result_.flags()[i] = !report_variant_indices.contains(i);
				}
			}
		}
		else if( somaticReportSupported() && rc_filter != ReportConfigFilter::NONE) //somatic report configuration filter (show only variants with report configuration)
		{
			QSet<int> report_variant_indices = somatic_report_settings_.report_config.variantIndices(VariantType::SNVS_INDELS, false).toSet();
			for(int i=0; i<variants_.count(); ++i)
			{
				if ( !filter_result_.flags()[i] ) continue;

				if (rc_filter==ReportConfigFilter::HAS_RC)
				{
					filter_result_.flags()[i] = report_variant_indices.contains(i);
				}
				else if (rc_filter==ReportConfigFilter::NO_RC)
				{
					filter_result_.flags()[i] = !report_variant_indices.contains(i);
				}
			}
		}

		//keep somatic variants that are marked with "include" in report settings (overrides possible filtering for that variant)
		if( somaticReportSupported() && rc_filter != ReportConfigFilter::NO_RC)
		{
			for(int index : somatic_report_settings_.report_config.variantIndices(VariantType::SNVS_INDELS, false))
			{
				filter_result_.flags()[index] = filter_result_.flags()[index] || somatic_report_settings_.report_config.variantConfig(index, VariantType::SNVS_INDELS).showInReport();
			}
		}
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Filtering error", e.message() + "\nA possible reason for this error is an outdated variant list.\nPlease re-run the annotation steps for the analysis!");

		filter_result_ = FilterResult(variants_.count(), false);
	}
}

void MainWindow::addToRecentFiles(QString filename)
{
	//update settings
	QStringList recent_files = Settings::stringList("recent_files", true);
	recent_files.removeAll(filename);
	if (QFile::exists(filename))
	{
		recent_files.prepend(filename);
	}
	while (recent_files.size() > 10)
	{
		recent_files.removeLast();
	}
	Settings::setStringList("recent_files", recent_files);

	//update GUI
	updateRecentFilesMenu();
}


void MainWindow::updateRecentFilesMenu()
{
	QStringList recent_files = Settings::stringList("recent_files", true);

	QMenu* menu = new QMenu();
	foreach(const QString& file, recent_files)
	{
		menu->addAction(file, this, SLOT(openRecentFile()));
	}
	ui_.actionRecent->setMenu(menu);
}

void MainWindow::updateIGVMenu()
{
	QStringList entries = Settings::stringList("igv_menu");
	if (entries.count()==0)
	{
		ui_.menuTrackDefaults->addAction("No custom entries in INI file!");

		ui_.menuOpenCustomTrack->addAction("No custom entries in INI file!");
	}
	else
	{
		foreach(QString entry, entries)
		{
			QStringList parts = entry.trimmed().split("\t");
			if(parts.count()!=3) continue;
			QAction* action = ui_.menuTrackDefaults->addAction("custom track: " + parts[0]);
			action->setCheckable(true);
			action->setChecked(parts[1]=="1");
			action->setToolTip(parts[2]);

			ui_.menuOpenCustomTrack->addAction(parts[0], this, SLOT(openCustomIgvTrack()));
		}
	}
}

void MainWindow::updateNGSDSupport()
{
	//init
	bool ngsd_user_logged_in = LoginManager::active();

	//toolbar
	ui_.report_btn->setEnabled(ngsd_user_logged_in);
	ui_.actionAnalysisStatus->setEnabled(ngsd_user_logged_in);
	ui_.actionReanalyze->setEnabled(ngsd_user_logged_in);
	ui_.actionGapsRecalculate->setEnabled(ngsd_user_logged_in);
	ui_.actionGeneSelector->setEnabled(ngsd_user_logged_in);
	ui_.actionSampleSearch->setEnabled(ngsd_user_logged_in);
	ui_.actionRunOverview->setEnabled(ngsd_user_logged_in);
	ui_.actionConvertHgvsToGSvar->setEnabled(ngsd_user_logged_in);
	ui_.actionGapsRecalculate->setEnabled(ngsd_user_logged_in);
	ui_.actionExpressionData->setEnabled(ngsd_user_logged_in);

	//toolbar - NGSD search menu
	QToolButton* ngsd_search_btn = ui_.tools->findChild<QToolButton*>("ngsd_search_btn");
	ngsd_search_btn->setEnabled(ngsd_user_logged_in);

	//NGSD menu
	ui_.menuNGSD->setEnabled(ngsd_user_logged_in);
	ui_.actionDesignSubpanel->setEnabled(ngsd_user_logged_in);

	//other actions
	ui_.actionOpenByName->setEnabled(ngsd_user_logged_in);
	ui_.ps_details->setEnabled(ngsd_user_logged_in);
	ui_.vars_ranking->setEnabled(ngsd_user_logged_in);

	ui_.filters->updateNGSDSupport();
}

void MainWindow::openRecentFile()
{
	QAction* action = qobject_cast<QAction*>(sender());
	loadFile(action->text());
}

QString MainWindow::normalSampleName()
{
	if(variants_.type() != AnalysisType::SOMATIC_PAIR) return "";

	foreach(const SampleInfo& info, variants_.getSampleHeader())
	{
		if (!info.isTumor()) return info.id;
	}

	return "";
}

