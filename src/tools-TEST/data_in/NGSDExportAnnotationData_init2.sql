
-- device
INSERT INTO device (id, type, name) VALUES 
(1, 'HiSeq2500', 'Morpheus');

-- sequencing_run
INSERT INTO sequencing_run (id, name, fcid, device_id, recipe, quality) VALUES
(1, 'First run', 'ABC', 1, '100+8+8+100', 'good'),
(2, 'Second run', 'XYZ', 1, '100+8+100', 'good');

-- user
INSERT INTO user (id, user_id, password, user_role, name, email, created, active) VALUES 
(99, 'ahuser', 's2d12kjg234hla0830t6hp9h3tt3t3tsdfg', 'user', 'The user', 'u@s.er', NOW(), '1');

-- sender
INSERT INTO sender (id, name) VALUES 
(1, 'sender');

-- project
INSERT INTO project (id, name, type, internal_coordinator_id, analysis) VALUES 
(1, 'First project', 'research', 1, 'variants'),
(2, 'Second project', 'diagnostic', 1, 'variants');

-- processing_system
INSERT INTO processing_system (id, name_manufacturer, shotgun, name_short, genome_id) VALUES 
(1, 'HaloPlex System', '1', 'hpSYSv1', 1),
(2, 'SureSelect Human All Exon v5', '1', 'ssHAEv5', 1);

-- sample
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality) VALUES 
(1, 'NA12878', 'DNA', 1, 'female', 1, '0', 1, 'good'),
(2, 'NA12879', 'DNA', 1, 'male', 1, '0', 1, 'good'),
(3, 'NA12880', 'DNA', 1, 'female', '0', '0', 1, 'good'),
(4, 'DUMMY', 'DNA', 1, 'male', '0', '0', 1, 'good');

-- processed_sample
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id) VALUES
(1, 1, 1, 1, 1, 2, 1, 1),
(2, 2, 2, 2, 1, 2, 2, 2),
(3, 3, 3, 2, 1, 2, 2, 2),
(4, 4, 1, 2, 1, 2, 2, 2);

--variant
INSERT INTO variant (id, chr, start, end, ref, obs) VALUES
(1, 'chr1', 62263112, 62263112, 'A', 'G'),
(2, 'chr3', 142558733, 142558733, 'A', 'T'),
(3, 'chr2', 47805601, 47805601, '-', 'T');

--somatic_vicc_interpretation
INSERT INTO `somatic_vicc_interpretation` (`id`, `variant_id`, `null_mutation_in_tsg`, `known_oncogenic_aa`, `strong_cancerhotspot`, `located_in_canerhotspot`, `absent_from_controls`, `protein_length_change`, `other_aa_known_oncogenic`, `weak_cancerhotspot`, `computational_evidence`, `mutation_in_gene_with_etiology`, `very_weak_cancerhotspot`, `very_high_maf`, `benign_functional_studies`, `high_maf`, `benign_computational_evidence`, `synonymous_mutation`, `comment`, `created_by`, `created_date`, `last_edit_by`, `last_edit_date`) VALUES
(1, 3, 0, 1, 1, NULL, 1, 0, NULL, 0, 1, NULL, 0, 0, 0, 0, NULL, 0, 'test VICC comment', 99, '2020-12-21 09:59:37', 99, '2020-12-23 09:33:23');

--detected somatic variant
INSERT INTO detected_somatic_variant (id, processed_sample_id_tumor, processed_sample_id_normal, variant_id, variant_frequency, depth) VALUES 
(1, 1, 3, 1, 0.1, 500),
(2, 1, 3, 2, 0.1, 500),
(3, 2, 4, 2, 0.1, 500),
(4, 2, 4, 3, 0.1, 500),
(5, 1, NULL, 3, 0.1, 500),
(6, 2, NULL, 3, 0.1, 500);
