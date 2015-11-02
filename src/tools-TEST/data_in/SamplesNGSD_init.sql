
-- device
INSERT INTO device (id, type, name) VALUES (1, 'HiSeq', 'Morpheus');

-- sequencing_run
INSERT INTO sequencing_run (id, name, fcid, device_id, recipe, quality) VALUES (1, 'First run', 'ABC', 1, '100+8+8+100', 'good');
INSERT INTO sequencing_run (id, name, fcid, device_id, recipe, quality) VALUES (2, 'Second run', 'XYZ', 1, '100+8+100', 'good');

-- user
INSERT INTO user (id, user_id, password, user_role, name, email, created, active) VALUES (2, 'ahuser', 's2d12kjg234hla0830t6hp9h3tt3t3tsdfg', 'user', 'The user', 'u@s.er', CURDATE(), '1');

-- sender
INSERT INTO sender (id, name) VALUES (1, 'sender');

-- project
INSERT INTO project (id, name, type, internal_coordinator_id, analysis) VALUES (1, 'First project', 'research', 1, 'annotation');
INSERT INTO project (id, name, type, internal_coordinator_id, analysis) VALUES (2, 'Second project', 'diagnostic', 1, 'annotation');

-- processing_system
INSERT INTO processing_system (id, name, shotgun, name_short, genome_id) VALUES (1, 'HaloPlex System', '1', 'hpSYSv1', 1);
INSERT INTO processing_system (id, name, shotgun, name_short, genome_id) VALUES (2, 'SureSelect Human All Exon v5', '1', 'ssHAEv5', 1);

-- sample
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality) VALUES (1, 'NA12878', 'DNA', 1, 'female', '0', '0', 1, 'good');

-- processed_sample
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id, last_analysis) VALUES (1, 1, 1, 1, 1, 2, 1, 1, '2015-10-30');
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id) VALUES (2, 1, 2, 2, 1, 2, 2, 2);
