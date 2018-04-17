
-- device
INSERT INTO device (id, type, name) VALUES (1, 'HiSeq2500', 'Morpheus');

-- sequencing_run
INSERT INTO sequencing_run (id, name, fcid, device_id, recipe, quality) VALUES (1, 'run1', 'ABC', 1, '100+8+8+100', 'good');
INSERT INTO sequencing_run (id, name, fcid, device_id, recipe, quality) VALUES (2, 'run2', 'XYZ', 1, '100+8+100', 'good');

-- user
INSERT INTO user (id, user_id, password, user_role, name, email, created, active) VALUES (2, 'ahuser', 's2d12kjg234hla0830t6hp9h3tt3t3tsdfg', 'user', 'The user', 'u@s.er', CURDATE(), '1');

-- sender
INSERT INTO sender (id, name) VALUES (1, 'sender');

-- project
INSERT INTO project (id, name, type, internal_coordinator_id, analysis) VALUES (1, 'First project', 'research', 1, 'annotation');
INSERT INTO project (id, name, type, internal_coordinator_id, analysis) VALUES (2, 'Second project', 'diagnostic', 1, 'annotation');

-- processing_system
INSERT INTO processing_system (id, name_manufacturer, shotgun, name_short, genome_id) VALUES (1, 'HaloPlex System', '1', 'hpSYSv1', 1);
INSERT INTO processing_system (id, name_manufacturer, shotgun, name_short, genome_id) VALUES (2, 'SureSelect Human All Exon v5', '1', 'ssHAEv5', 1);

-- sample
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality, disease_group, disease_status) VALUES (1, 'NA12878', 'DNA', 1, 'female', '0', '0', 1, 'good', 'Neoplasms', 'Affected');
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality) VALUES (2, 'NA12880', 'DNA', 1, 'female', '0', '0', 1, 'good');

-- processed_sample
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id, quality) VALUES (1, 1, 1, 1, 1, 2, 1, 1, 'bad');
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id) VALUES (2, 1, 2, 2, 1, 2, 2, 2);
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id) VALUES (3, 2, 1, 2, 1, 2, 2, 2);

-- qc_terms
INSERT INTO `qc_terms` (`id`, `qcml_id`, `name`, `description`) VALUES
(4, 'QC:2000023', 'insert size', 'Average insert size (for paired-end reads only).'),
(47, 'QC:2000025', 'target region read depth', 'Average sequencing depth in target region.'),
(19, 'QC:2000026', 'target region 10x percentage', 'Percentage the target region that is covered at least 10-fold.'),
(31, 'QC:2000027', 'target region 20x percentage', 'Percentage the target region that is covered at least 20-fold.');

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
(1, '2015-10-30T23:59:00', 2, 'queued', ''),
(1, '2015-11-01T00:00:10', null, 'started', ''),
(1, '2015-11-01T04:13:32', null, 'finished', '');

