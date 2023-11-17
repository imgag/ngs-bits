
-- device
INSERT INTO device (id, type, name) VALUES 
(1, 'HiSeq2500', 'Morpheus');

-- sequencing_run
INSERT INTO sequencing_run (id, name, fcid, device_id, recipe, quality, start_date) VALUES 
(1, 'run1', 'ABC', 1, '100+8+8+100', 'good', "2021-01-01"),
(2, 'run2', 'XYZ', 1, '100+8+100', 'good', "2023-01-01");

-- user
INSERT INTO user (id, user_id, password, user_role, name, email, created, active) VALUES 
(99, 'ahuser', 's2d12kjg234hla0830t6hp9h3tt3t3tsdfg', 'user', 'The user', 'u@s.er', NOW(), '1');

-- sender
INSERT INTO sender (id, name) VALUES 
(1, 'Klaus-Erhard');

-- project
INSERT INTO project (id, name, type, internal_coordinator_id, analysis, archived) VALUES 
(1, 'First_project', 'research', 1, 'variants', 0),
(2, 'Second_project', 'diagnostic', 1, 'variants', 1),
(3, 'Third_project', 'diagnostic', 1, 'variants', 0);

-- processing_system
INSERT INTO processing_system (id, name_manufacturer, shotgun, name_short, genome_id, type) VALUES
(1, 'HaloPlex System', '1', 'hpSYSv1', 1, 'Panel'),
(2, 'SureSelect Human All Exon v5', '1', 'ssHAEv5', 1, 'WES'),
(3, 'TruSeq DNA PCR-free', '1', 'TruSeq', 1, 'WGS'),
(4, 'Nanopore v14', '1', 'SQK-114', 1, 'lrGS'),
(5, 'RNA ps', '1', 'rna', 1, 'RNA');

-- sample
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality, disease_group, disease_status, tissue, received, year_of_birth, patient_identifier) VALUES
(1, 'NA12878', 'DNA', 1, 'female', '0', '0', 1, 'good', 'Neoplasms', 'Affected', 'blood', '2023-07-13', 1977, "pat1"),
(2, 'NA12880', 'DNA', 1, 'female', '1', '0', 1, 'good', 'n/a', 'n/a', 'skin', NULL, NULL, NULL),
(3, 'NA12881', 'DNA', 1, 'female', '1', '0', 1, 'good', 'n/a', 'n/a', 'skin', NULL, NULL, NULL),
(4, 'lrGS12882', 'DNA', 1, 'female', '1', '0', 1, 'good', 'n/a', 'n/a', 'skin', NULL, NULL, "pat1"),
(5, 'RNA12883', 'RNA', 1, 'female', '1', '0', 1, 'good', 'n/a', 'n/a', 'skin', NULL, NULL, NULL);

-- processed_sample
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id, quality, normal_id) VALUES
(1, 1, 1, 1, 1, 2, 1, 1, 'bad', NULL),
(2, 1, 2, 2, 1, 2, 2, 2, 'n/a', NULL),
(3, 2, 1, 2, 1, 2, 2, 2, 'n/a', 2),
(4, 2, 2, 2, 1, 2, 2, 3, 'n/a', NULL),
(5, 3, 45, 1, 1, 1, 3, 1, 'good', NULL),
(6, 3, 46, 1, 1, 1, 3, 1, 'good', NULL),
(8, 4, 23, 2, 1, 1, 4, 1, 'good', NULL),
(9, 4, 28, 2, 1, 1, 4, 1, 'good', NULL),
(10, 5, 2, 1, 1, 1, 5, 1, 'good', NULL);

INSERT INTO `sample_relations`(`sample1_id`, `relation`, `sample2_id`) VALUES
(2, 'same sample', 3),
(3, 'same patient', 4),
(4, 'same sample', 5);