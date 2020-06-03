
-- device
INSERT INTO device (id, type, name) VALUES 
(1, 'HiSeq2500', 'Morpheus');

-- sequencing_run
INSERT INTO sequencing_run (id, name, fcid, device_id, recipe, quality) VALUES 
(1, 'run1', 'ABC', 1, '100+8+8+100', 'good');

-- user
INSERT INTO user (id, user_id, password, user_role, name, email, created, active) VALUES 
(99, 'ahuser', 's2d12kjg234hla0830t6hp9h3tt3t3tsdfg', 'user', 'The user', 'u@s.er', NOW(), '1');

-- sender
INSERT INTO sender (id, name) VALUES 
(1, 'sender');

-- project
INSERT INTO project (id, name, type, internal_coordinator_id, analysis) VALUES 
(1, 'First_project', 'research', 1, 'variants');

-- processing_system
INSERT INTO processing_system (id, name_manufacturer, shotgun, name_short, genome_id) VALUES
(1, 'SureSelect Human All Exon v5', '1', 'ssHAEv7', 1);

-- sample
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality, disease_group, disease_status) VALUES
(1, 'NA12878', 'DNA', 1, 'female', '0', '0', 1, 'good', 'Neoplasms', 'Affected');

-- processed_sample
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id, quality) VALUES
(1, 1, 1, '1', 1, 1, 1, 1, 'good'),
(2, 1, 2, '1', 1, 1, 1, 1, 'good'),
(3, 1, 3, '1', 1, 1, 1, 1, 'bad'),
(4, 1, 4, '1', 1, 1, 1, 1, 'good'),
(5, 1, 5, '1', 1, 1, 1, 1, 'good');

-- qc_terms
INSERT INTO `qc_terms` (`id`, `qcml_id`, `name`, `description`, `type`, `obsolete`) VALUES
(47, 'QC:2000025', 'target region read depth', 'Average sequencing depth in target region.', 'float', 0);

-- processed_sample_qc
INSERT INTO `processed_sample_qc` (`processed_sample_id`, `qc_terms_id`, `value`) VALUES
(1, 47, '100.28'),
(2, 47, '20.28'),
(3, 47, '100.28'),
(4, 47, '100.28'),
(5, 47, '100.28');

-- cnv_callset
INSERT INTO `cnv_callset` (`id`, `processed_sample_id`, `caller`, `caller_version`, `call_date`, `quality_metrics`, `quality`) VALUES
(1, 1, 'ClinCNV', 'v 1.16.1', '2019-10-20T09:55:01', null, 'good'),
(2, 2, 'ClinCNV', 'v 1.16.1', '2019-10-20T09:55:01', null, 'good'),
(3, 3, 'ClinCNV', 'v 1.16.1', '2019-10-20T09:55:01', null, 'good'),
(4, 4, 'ClinCNV', 'v 1.16.1', '2019-10-20T09:55:01', null, 'bad'),
(5, 5, 'ClinCNV', 'v 1.16.1', '2019-10-20T09:55:01', null, 'good');

-- cnv
INSERT INTO `cnv` (`cnv_callset_id`, `chr`, `start`, `end`, `cn`) VALUES
(1, 'chr1', 1000, 2000, 1),
(1, 'chr1', 3000, 4000, 1),
(1, 'chr2', 10000, 40000, 1),

(2, 'chr1', 2000, 3000, 2),
(2, 'chr2', 10000, 20000, 2),
(2, 'chr2', 30000, 40000, 2),

(3, 'chr1', 1000, 4000, 2),

(4, 'chr1', 1000, 4000, 2),


(5, 'chr1', 1000, 3000, 5),
(5, 'chr2', 20000, 40000, 5),
(5, 'chr3', 100000, 200000, 5);
