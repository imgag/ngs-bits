
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
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality) VALUES (2, 'NA12879', 'DNA', 1, 'male', '0', '0', 1, 'good');
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality) VALUES (3, 'NA12880', 'DNA', 1, 'female', '0', '0', 1, 'good');
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality) VALUES (4, 'DUMMY', 'DNA', 1, 'male', '0', '0', 1, 'good');

-- processed_sample
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id) VALUES (1, 1, 1, 1, 1, 2, 1, 1);
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id) VALUES (2, 2, 2, 2, 1, 2, 2, 2);
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id) VALUES (3, 3, 3, 2, 1, 2, 2, 2);
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id) VALUES (4, 4, 1, 2, 1, 2, 2, 2);

--variant
INSERT INTO variant (id, chr, start, end, ref, obs, genome_id) VALUES (1, 'chr1', 62713224, 62713224, 'C', 'G', 1);
INSERT INTO variant (id, chr, start, end, ref, obs, genome_id) VALUES (2, 'chr1', 62713246, 62713246, 'G', 'A', 1);
INSERT INTO variant (id, chr, start, end, ref, obs, genome_id) VALUES (3, 'chr1', 62728784, 62728784, 'A', 'G', 1);
INSERT INTO variant (id, chr, start, end, ref, obs, genome_id) VALUES (4, 'chr1', 62728838, 62728838, 'T', 'C', 1);
INSERT INTO variant (id, chr, start, end, ref, obs, genome_id) VALUES (5, 'chr1', 120539331, 120539331, 'C', 'T', 1);
INSERT INTO variant (id, chr, start, end, ref, obs, genome_id) VALUES (6, 'chr1', 120611964, 120611964, 'G', 'A', 1);
INSERT INTO variant (id, chr, start, end, ref, obs, genome_id) VALUES (7, 'chr1', 120612034, 120612034, 'T', 'G', 1);
INSERT INTO variant (id, chr, start, end, ref, obs, genome_id) VALUES (8, 'chr1', 120612040, 120612040, '-', 'CCTCCTCCG', 1);

--variant_classification
INSERT INTO variant_classification (id, variant_id, class) VALUES (1, 2, '5');
INSERT INTO variant_classification (id, variant_id, class) VALUES (2, 8, '1');

--detected variant NA12878
INSERT INTO detected_variant (id, processed_sample_id, variant_id, genotype, quality) VALUES (1, 1, 1, 'het', '');
INSERT INTO detected_variant (id, processed_sample_id, variant_id, genotype, quality) VALUES (2, 1, 8, 'hom', '');

--detected variant NA12879
INSERT INTO detected_variant (id, processed_sample_id, variant_id, genotype, quality) VALUES (3, 2, 1, 'het', '');
INSERT INTO detected_variant (id, processed_sample_id, variant_id, genotype, quality) VALUES (4, 2, 3, 'het', '');
INSERT INTO detected_variant (id, processed_sample_id, variant_id, genotype, quality) VALUES (5, 2, 4, 'het', '');

--detected variant NA12880
INSERT INTO detected_variant (id, processed_sample_id, variant_id, genotype, quality, comment) VALUES (6, 3, 1, 'hom', '', 'from NA12880');
INSERT INTO detected_variant (id, processed_sample_id, variant_id, genotype, quality) VALUES (7, 3, 4, 'het', '');

--detected variant DUMMY
INSERT INTO detected_variant (id, processed_sample_id, variant_id, genotype, quality) VALUES (8, 4, 1, 'hom', '');
INSERT INTO detected_variant (id, processed_sample_id, variant_id, genotype, quality, comment) VALUES (9, 4, 4, 'het', '', 'from DUMMY');

--variant validation NA12878
INSERT INTO variant_validation (id, sample_id, variant_id, genotype, status, comment) VALUES (1, 1, 8, 'hom', 'false positive', 'val com1');

--variant validation DUMMY
INSERT INTO variant_validation (id, sample_id, variant_id, genotype, status, comment) VALUES (2, 4, 4, 'hom', 'true positive', 'val com2');
