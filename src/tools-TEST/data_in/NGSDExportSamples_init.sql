
-- device
INSERT INTO device (id, type, name) VALUES 
(1, 'HiSeq2500', 'Morpheus');

-- sequencing_run
INSERT INTO sequencing_run (id, name, fcid, device_id, recipe, quality) VALUES 
(1, 'run1', 'ABC', 1, '100+8+8+100', 'good'),
(2, 'run2', 'XYZ', 1, '100+8+100', 'good');

-- user
INSERT INTO user (id, user_id, password, user_role, name, email, created, active) VALUES 
(99, 'ahuser', 's2d12kjg234hla0830t6hp9h3tt3t3tsdfg', 'user', 'The user', 'u@s.er', NOW(), '1');

-- sender
INSERT INTO sender (id, name) VALUES 
(1, 'Klaus-Erhard');

-- project
INSERT INTO project (id, name, type, internal_coordinator_id, analysis) VALUES 
(1, 'First_project', 'research', 1, 'variants'),
(2, 'Second_project', 'diagnostic', 1, 'variants'),
(3, 'Third_project', 'diagnostic', 1, 'variants');

-- processing_system
INSERT INTO processing_system (id, name_manufacturer, shotgun, name_short, genome_id) VALUES
(1, 'HaloPlex System', '1', 'hpSYSv1', 1),
(2, 'SureSelect Human All Exon v5', '1', 'ssHAEv5', 1);

-- sample
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality, disease_group, disease_status) VALUES
(1, 'NA12878', 'DNA', 1, 'female', '0', '0', 1, 'good', 'Neoplasms', 'Affected'),
(2, 'NA12880', 'DNA', 1, 'female', '1', '0', 1, 'good', 'n/a', 'n/a');

-- processed_sample
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id, quality) VALUES
(1, 1, 1, 1, 1, 2, 1, 1, 'bad'),
(2, 1, 2, 2, 1, 2, 2, 2, 'n/a'),
(3, 2, 1, 2, 1, 2, 2, 2, 'n/a'),
(4, 2, 2, 2, 1, 2, 2, 3, 'n/a');

-- merged_processed_samples
INSERT INTO merged_processed_samples (processed_sample_id, merged_into) VALUES (4, 3);

-- qc_terms
INSERT INTO `qc_terms` (`id`, `qcml_id`, `name`, `description`, `type`, `obsolete`) VALUES
(4, 'QC:2000023', 'insert size', 'Average insert size (for paired-end reads only).', 'float', 0),
(47, 'QC:2000025', 'target region read depth', 'Average sequencing depth in target region.', 'float', 0),
(19, 'QC:2000026', 'target region 10x percentage', 'Percentage the target region that is covered at least 10-fold.', 'float', 0),
(31, 'QC:2000027', 'target region 20x percentage', 'Percentage the target region that is covered at least 20-fold.', 'float', 0);

-- processed_sample_qc
INSERT INTO `processed_sample_qc` (`processed_sample_id`, `qc_terms_id`, `value`) VALUES
(1, 4, '157.34'),
(1, 47, '456.33'),
(1, 19, '98.98'),
(1, 31, '97.45'),
(2, 4, '32.34'),
(2, 47, '100.28');

-- analysis job
INSERT INTO `analysis_job`(`type`, `high_priority`, `args`, `sge_id`, `sge_queue`) VALUES
('single sample',1,'','','');

INSERT INTO `analysis_job_sample`(`analysis_job_id`, `processed_sample_id`, `info`) VALUES
(1, 1, '');

INSERT INTO `analysis_job_history`(`analysis_job_id`, `time`, `user_id`, `status`, `output`) VALUES
(1, '2015-10-30T23:59:00', 99, 'queued', ''),
(1, '2015-11-01T00:00:10', null, 'started', ''),
(1, '2015-11-01T04:13:32', null, 'finished', '');

--- disease infos
INSERT INTO `diag_status`(`processed_sample_id`, `status`, `user_id`, `date`, `outcome`, `comment`) VALUES
(3, 'done', 99, '2018-10-23 14:09:21', 'significant findings', 'comment line1\ncomment\tline2');


INSERT INTO `hpo_term` (`id`, `hpo_id`, `name`, `definition`, `synonyms`) VALUES
(11495, 'HP:0000003', 'Multicystic kidney dysplasia', '\"Multicystic dysplasia of the kidney is characterized by multiple cysts of varying size in the kidney and the absence of a normal pelvocaliceal system. The condition is associated with ureteral or ureteropelvic atresia, and the affected kidney is nonfunctional.', ''),
(64350, 'HP:0002862', 'Bladder carcinoma', '\"The presence of a carcinoma of the urinary bladder.', '');

INSERT INTO `sample_disease_info` (`id`, `sample_id`, `disease_info`, `type`, `user_id`, `date`) VALUES
(1, 1, 'HP:0000003', 'HPO term id', 99, '2018-11-21 18:26:26'),
(2, 2, 'HP:0000003', 'HPO term id', 99, '2018-11-21 18:26:26'),
(3, 2, 'HP:0002862', 'HPO term id', 99, '2018-11-21 18:26:26'),
(4, 2, 'C34.9', 'ICD10 code', 99, '2018-11-21 18:26:26'),
(5, 2, '60', 'tumor fraction', 99, '2018-11-21 18:26:26');

--report config
INSERT INTO `report_configuration`(`id`, `processed_sample_id`, `created_by`, `created_date`) VALUES
(1,2,99,'2019-12-11 15:19:26'),
(2,3,99,'2019-12-11 15:19:26');

INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `1000g`, `gnomad`, `gene`, `variant_type`, `coding`, `comment`) VALUES
(1, 'chr1', 115256554, 115256554, 'A', 'G', NULL, NULL, 'NRAS', 'synonymous', 'NRAS:ENST00000369535:synonymous_variant:LOW:exon3/7:c.157T>C:p.Leu53=:PF00071', NULL);

INSERT INTO `report_configuration_variant` (`id`, `report_configuration_id`, `variant_id`, `type`, `causal`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES
(1, 1, 1, 'diagnostic variant', 1, 'n/a', 0, 0, 0, 0, 0, 0, 0, 0, '', '');


INSERT INTO `cnv_callset` (`id`, `processed_sample_id`, `caller`, `caller_version`, `call_date`, `quality_metrics`, `quality`) VALUES
(1, 3, 'ClinCNV', 'v1.16.4', '2019-08-26 14:21:42', '', 'good');

INSERT INTO `cnv` (`id`, `cnv_callset_id`, `chr`, `start`, `end`, `cn`, `quality_metrics`) VALUES
(1, 1, 'chr1', 17248493, 17275441, 1, '');

INSERT INTO `report_configuration_cnv` (`id`, `report_configuration_id`, `cnv_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES
(1, 2, 1, 'diagnostic variant', 0, 'n/a', 'AD', 0, 0, 0, 0, 0, 1, 0, 0, '', '');

--study
INSERT INTO `study`(`id`, `name`, `description`) VALUES
(1, "SomeStudy","");

INSERT INTO `study_sample`(`study_id`, `processed_sample_id`) VALUES
(1, 2);

INSERT INTO `processed_sample_ancestry` (`processed_sample_id`, `num_snps`, `score_afr`, `score_eur`, `score_sas`, `score_eas`, `population`) VALUES
(1, 3123, 0.0469, 0.3401, 0.1803, 0.0349, 'EUR'),
(2, 3151, 0.0412, 0.3028, 0.1796, 0.0926, 'ADMIXED/UNKNOWN');


