
-- sender
INSERT INTO sender (id, name) VALUES 
(1, 'SENDER1');

-- device
INSERT INTO device (id, type, name) VALUES 
(1, 'NovaSeq6000', 'HORST');

-- sequencing_run
INSERT INTO sequencing_run (id, name, fcid, device_id, recipe, quality) VALUES 
(1, 'RUN1', 'ABC', 1, '150+8+8+150', 'good');


-- processing_system
INSERT INTO processing_system (id, name_short, name_manufacturer, type, shotgun, genome_id) VALUES
(1, 'TruSeqPCRfree', 'Illumina TruSeq DNA PCR-Free', 'WGS', '1', 1),
(2, 'nebRNAU2_qiaRRNA_umi', 'NEBNext Ultra II Directional RNA + QIAseq FastSelect rRNA removal UMI', 'RNA', '1', 1);

-- sample
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality, disease_group, disease_status, tissue) VALUES
(1, 'SAMPLE1', 'DNA', 1, 'female', '0', '0', 1, 'good', 'Neoplasms', 'Affected', 'blood'),
(2, 'SAMPLE2', 'RNA', 1, 'male', '0', '0', 1, 'good', 'Neoplasms', 'Affected', 'skin');

-- project
INSERT INTO project (id, name, type, internal_coordinator_id, analysis, archived) VALUES 
(1, 'PROJECT1', 'research', 1, 'variants', 0);

-- processed_sample
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id, quality) VALUES
(1, 1, 2, 1, 1, 2, 1, 1, 'good'),
(2, 2, 1, 1, 1, 2, 2, 1, 'good');

--study
INSERT INTO study (id, name, description) VALUES 
(1, 'Study4711', 'Descripion of the study');

-- study_sample
INSERT INTO study_sample (id, study_id, processed_sample_id) VALUES 
(1, 1, 1),
(2, 1, 2);
