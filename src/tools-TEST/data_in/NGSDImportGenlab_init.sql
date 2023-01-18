INSERT INTO user (user_id, password, user_role, name, email, created, active) VALUES
('ahtest', 'password', 'user', 'Orang Utan Klaus', 'ouk@med.uni-tuebingen.de', NOW(), 1);

INSERT INTO sender (name) VALUES
('Mario Luigi');

INSERT INTO device (type, name) VALUES
('GAIIx', 'test');

INSERT INTO project (name, type, internal_coordinator_id, analysis) VALUES
('SomaticAndTreatment','diagnostic', 2, 'variants');

INSERT INTO processing_system (name_short, name_manufacturer, shotgun, genome_id, type) VALUES
('ssHAEv6', 'SureSelect Human All Exon v6', '1', 1, 'WES');

INSERT INTO sequencing_run (name, fcid, start_date, end_date, device_id, recipe) VALUES
('#00001', 'FCID4711', '2018-02-04', '2018-02-04', 1, '100+8+100'),
('#00002', 'FCID4712', '2018-02-05', '2018-02-05', 1, '100+8+100'),
('#01489', 'FCID4713', '2012-06-27', '2020-06-29', 1, '100+8+100');

INSERT INTO sample (name, sample_type, species_id, gender, tumor, ffpe, sender_id, disease_group) VALUES 
('DXtest01', 'DNA', 1, 'male', '0', '0', 1, 'n/a'),
('DXtest02', 'DNA', 1, 'female', '0', '0', 1, 'n/a');
('DXtest03', 'DNA', 1, 'male', '0', '0', 1, 'n/a'),
('DXtest04', 'DNA', 1, 'female', '0', '0', 1, 'n/a');


INSERT INTO processed_sample (sample_id, process_id, sequencing_run_id, lane, processing_system_id, project_id, normal_id) VALUES
(1,1,1,'1',1,1, NULL),
(2,2,1,'1',1,1, NULL),
(3,1,1,'1',1,1, NULL),
(4,1,1,'1',1,1, NULL),
(5,1,1,'1',1,1, NULL),
(6,1,1,'1',1,1, 5),
(7,1,1,'1',1,1, NULL),
(8,1,1,'1',1,1, 5),
(9,2,3,'1',1,1, NULL),
(10,3,3,'1',1,1, NULL),
(11,4,3,'1',1,1, NULL);

