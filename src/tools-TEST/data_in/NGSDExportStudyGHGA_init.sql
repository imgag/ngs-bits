
-- sender
INSERT INTO sender (id, name) VALUES 
(1, 'SENDER1');

-- device
INSERT INTO device (id, type, name) VALUES 
(1, 'NovaSeq6000', 'HORST');

-- sequencing_run
INSERT INTO sequencing_run (id, name, fcid, flowcell_type, device_id, recipe, quality) VALUES 
(1, 'RUN1', 'ABC', 'Illumina NovaSeq S4', 1, '150+8+8+150', 'good');


-- processing_system
INSERT INTO processing_system (id, name_short, name_manufacturer, type, shotgun, genome_id) VALUES
(1, 'TruSeqPCRfree', 'Illumina TruSeq DNA PCR-Free', 'WGS', '1', 1),
(2, 'nebRNAU2_qiaRRNA_umi', 'NEBNext Ultra II Directional RNA + QIAseq FastSelect rRNA removal UMI', 'RNA', '1', 1);

-- sample
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality, disease_group, disease_status, tissue) VALUES
(11, 'SAMPLE1', 'DNA', 1, 'female', '0', '0', 1, 'good', 'Neoplasms', 'Affected', 'blood'),
(22, 'SAMPLE2', 'RNA', 1, 'male', '0', '0', 1, 'good', 'Neoplasms', 'Affected', 'skin');

-- sample_disease_info
INSERT INTO sample_disease_info(id, sample_id, disease_info, type, user_id, date) VALUES
(1, 11, 'HP:0033006' , 'HPO term id', 1 , '2023-01-20 11:41:10'),
(2, 11, 'HP:0040049' , 'HPO term id', 1 , '2023-01-19 12:43:43');

-- hpo_term
INSERT INTO `hpo_term`(`id`, `hpo_id`, `name`, `definition`, `synonyms`) VALUES 
(109927,'HP:0033006', 'Diffuse alveolar damage','Diffuse alveolar damage (DAD) describes a comon hi...', ''),
(111242,'HP:0040049', 'Macular edema','Thickening of the retina that takes place due to a..', 'Macular oedema');

-- project
INSERT INTO project (id, name, type, internal_coordinator_id, analysis, archived) VALUES 
(1, 'PROJECT1', 'research', 1, 'variants', 0);

-- processed_sample
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id, quality) VALUES
(111, 11, 2, 1, 1, 2, 1, 1, 'good'),
(222, 22, 1, 1, 1, 2, 2, 1, 'good');

-- processed_sample_ancestry
INSERT INTO processed_sample_ancestry(processed_sample_id, num_snps, score_afr, score_eur, score_sas, score_eas, population) VALUES
(111, 47,0.0,1.0,0.0,0.0,'EUR');
