INSERT INTO user (user_id, password, user_role, name, email, created, active) VALUES
('ahtest', 'password', 'user', 'Orang Utan Klaus', 'ouk@med.uni-tuebingen.de', NOW(), 1);

INSERT INTO sender (name) VALUES
('Mario Luigi');

INSERT INTO device (type, name) VALUES
('GAIIx', 'test');

INSERT INTO study (name, description) VALUES
('DISCO-TWIN', 'test'),
('Genome+', 'test');

INSERT INTO project (name, type, internal_coordinator_id, analysis) VALUES
('SomaticAndTreatment','diagnostic', 2, 'variants');

INSERT INTO processing_system (name_short, name_manufacturer, shotgun, genome_id, type) VALUES
('ssHAEv6', 'SureSelect Human All Exon v6', '1', 1, 'WES');

INSERT INTO sequencing_run (name, fcid, start_date, end_date, device_id, recipe) VALUES
('#00001', 'FCID4711', '2018-02-04', '2018-02-04', 1, '100+8+100'),
('#00002', 'FCID4712', '2018-02-05', '2018-02-05', 1, '100+8+100'),
('#01489', 'FCID4713', '2012-06-27', '2020-06-29', 1, '100+8+100');

INSERT INTO sample (name, sample_type, species_id, gender, tumor, ffpe, sender_id, disease_group, disease_status, patient_identifier) VALUES 
('DXtest1', 'DNA', 1, 'female', '0', '0', 1, 'Neoplasms', 'Unaffected', "9999999"),
('DXtest2', 'DNA', 1, 'male',   '0', '0', 1, 'Neoplasms', 'Unaffected', "9999999"),
('DXtest3', 'DNA', 1, 'n/a', '0', '0', 1, 'n/a', 'n/a', ""),
('DXtest4', 'DNA', 1, 'n/a', '0', '0', 1, 'n/a', 'n/a', ""),
('DXtest5', 'DNA', 1, 'n/a', '0', '0', 1, 'n/a', 'n/a', "");


INSERT INTO processed_sample (sample_id, process_id, sequencing_run_id, lane, processing_system_id, project_id, normal_id) VALUES
(1,1,1,'1',1,1, NULL),
(2,1,1,'1',1,1, NULL),
(3,1,1,'1',1,1, NULL),
(4,1,1,'1',1,1, NULL),
(5,1,1,'1',1,1, NULL);

INSERT INTO sample_disease_info (sample_id, disease_info, type, user_id, date) VALUES
(1, "HP:9999999", "HPO term id", 1, "2023-01-18 00:00:00"),
(1, "tissue", "RNA reference tissue", 1, "2023-01-18 00:00:00"),
(1, "ORPHA:999", "Orpha number", 1, "2023-01-18 00:00:00"),
(1, "G99.9", "ICD10 code", 1, "2023-01-18 00:00:00"),
(1, "111", "tumor fraction", 1, "2023-01-18 00:00:00"),
(1, "Is sick", "clinical phenotype (free text)", 1, "2023-01-18 00:00:00"),
(2, "HP:9999999", "HPO term id", 1, "2023-01-18 00:00:00"),
(2, "tissue", "RNA reference tissue", 1, "2023-01-18 00:00:00"),
(2, "ORPHA:999", "Orpha number", 1, "2023-01-18 00:00:00"),
(2, "G99.9", "ICD10 code", 1, "2023-01-18 00:00:00"),
(2, "111", "tumor fraction", 1, "2023-01-18 00:00:00"),
(2, "Is sick", "clinical phenotype (free text)", 1, "2023-01-18 00:00:00");




