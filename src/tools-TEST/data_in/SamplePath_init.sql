
-- device
INSERT INTO device (id, type, name) VALUES 
(1, 'HiSeq2500', 'Morpheus');

-- sequencing_run
INSERT INTO sequencing_run (id, name, fcid, device_id, recipe, quality) VALUES 
(1, 'run1', 'ABC', 1, '100+8+8+100', 'good');

-- sender
INSERT INTO sender (id, name) VALUES 
(1, 'Klaus-Erhard');

-- project
INSERT INTO project (id, name, type, internal_coordinator_id, analysis, archived, folder_override) VALUES 
(1, 'First_project', 'research', 1, 'variants', 0, NULL),
(2, 'Second_project', 'research', 1, 'variants', 0, "C:\\or_project\\");


-- processing_system
INSERT INTO processing_system (id, name_manufacturer, shotgun, name_short, genome_id) VALUES
(1, 'SureSelect Human All Exon v5', '1', 'ssHAEv5', 1);

-- sample
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality, disease_group, disease_status, tissue, received, year_of_birth) VALUES
(1, 'NA12878', 'DNA', 1, 'female', '0', '0', 1, 'good', 'Neoplasms', 'Affected', 'blood', '2023-07-13', 1977);

-- processed_sample
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id, quality, folder_override) VALUES
(1, 1, 1, 1, 1, 2, 1, 1, 'bad', NULL),
(2, 1, 2, 1, 1, 2, 1, 2, 'bad', NULL),
(3, 1, 3, 1, 1, 2, 1, 2, 'bad', "C:\\or_sample\\Sample_XYZ\\");
