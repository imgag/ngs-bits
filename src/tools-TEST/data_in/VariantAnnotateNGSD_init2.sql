
-- device
INSERT INTO device (id, type, name) VALUES (1, 'HiSeq2500', 'Morpheus');

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
INSERT INTO processing_system (id, name_manufacturer, shotgun, name_short, genome_id) VALUES (1, 'HaloPlex System', '1', 'hpSYSv1', 1);
INSERT INTO processing_system (id, name_manufacturer, shotgun, name_short, genome_id) VALUES (2, 'SureSelect Human All Exon v5', '1', 'ssHAEv5', 1);

-- sample
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality) VALUES (1, 'NA12878', 'DNA', 1, 'female', 1, '0', 1, 'good');
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality) VALUES (2, 'NA12879', 'DNA', 1, 'male', 1, '0', 1, 'good');
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality) VALUES (3, 'NA12880', 'DNA', 1, 'female', '0', '0', 1, 'good');
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality) VALUES (4, 'DUMMY', 'DNA', 1, 'male', '0', '0', 1, 'good');

-- processed_sample
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id) VALUES (1, 1, 1, 1, 1, 2, 1, 1);
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id) VALUES (2, 2, 2, 2, 1, 2, 2, 2);
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id) VALUES (3, 3, 3, 2, 1, 2, 2, 2);
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id) VALUES (4, 4, 1, 2, 1, 2, 2, 2);

--variant
INSERT INTO variant (id, chr, start, end, ref, obs) VALUES (1, 'chr1', 62728784, 62728784, 'A', 'G');
INSERT INTO variant (id, chr, start, end, ref, obs) VALUES (2, 'chr3', 142277575, 142277575, 'A', 'T');
INSERT INTO variant (id, chr, start, end, ref, obs) VALUES (3, 'chr2', 48032740, 48032740, '-', 'T');

--detected somatic variant
INSERT INTO detected_somatic_variant (id, processed_sample_id_tumor, processed_sample_id_normal, variant_id, variant_frequency, depth) VALUES (1, 1, 3, 1, 0.1, 500);
INSERT INTO detected_somatic_variant (id, processed_sample_id_tumor, processed_sample_id_normal, variant_id, variant_frequency, depth) VALUES (2, 1, 3, 2, 0.1, 500);
INSERT INTO detected_somatic_variant (id, processed_sample_id_tumor, processed_sample_id_normal, variant_id, variant_frequency, depth) VALUES (3, 2, 4, 2, 0.1, 500);
INSERT INTO detected_somatic_variant (id, processed_sample_id_tumor, processed_sample_id_normal, variant_id, variant_frequency, depth) VALUES (4, 2, 4, 3, 0.1, 500);
