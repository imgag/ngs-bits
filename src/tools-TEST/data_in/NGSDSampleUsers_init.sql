
-- device
INSERT INTO device (id, type, name) VALUES 
(1, 'HiSeq2500', 'Morpheus');

-- sequencing_run
INSERT INTO sequencing_run (id, name, fcid, device_id, recipe, quality, start_date) VALUES 
(1, 'run1', 'ABC', 1, '100+8+8+100', 'good', "2021-01-01");

-- user
INSERT INTO user (id, user_id, password, user_role, name, email, created, active) VALUES 
(100, 'rc_creator', 's2d12kjg234hla0830t6hp9h3tt3t3tsdfg', 'user', 'RC creator', 'user1@user.de', NOW(), '1'),
(101, 'rc_updater', 's2d12kjg234hla0830t6hp9h3tt3t3tsdfg', 'user', 'RC updater', 'user2@user.de', NOW(), '1'),
(102, 'rc_finalizer', 's2d12kjg234hla0830t6hp9h3tt3t3tsdfg', 'user', 'RC finalizer', 'user3@user.de', NOW(), '1'),
(103, 'ds_setter', 's2d12kjg234hla0830t6hp9h3tt3t3tsdfg', 'user', 'DS setter', 'user4@user.de', NOW(), '1');

-- sender
INSERT INTO sender (id, name) VALUES 
(1, 'Sibira');

-- project
INSERT INTO project (id, name, type, internal_coordinator_id, analysis, archived) VALUES 
(1, 'First_project', 'research', 1, 'variants', 0);

-- processing_system
INSERT INTO processing_system (id, name_manufacturer, shotgun, name_short, genome_id, type) VALUES
(1, 'HaloPlex System', '1', 'hpSYSv1', 1, 'Panel');

-- sample
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality, disease_group, disease_status, tissue, received, year_of_birth, patient_identifier) VALUES
(1, 'NA12877', 'DNA', 1, 'female', '0', '0', 1, 'good', 'Neoplasms', 'Affected', 'blood', '2023-07-13', 1977, "pat1"),
(2, 'NA12878', 'DNA', 1, 'female', '0', '0', 1, 'good', 'n/a', 'n/a', 'skin', NULL, NULL, NULL);

-- processed_sample
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id, quality, normal_id) VALUES
(1, 1, 1, 1, 1, 1, 1, 1, 'good', NULL),
(2, 2, 1, 1, 1, 1, 1, 1, 'good', NULL);

-- report_configuration`
INSERT INTO `report_configuration`(`id`, `processed_sample_id`, `created_by`, `created_date`, `last_edit_by`, `last_edit_date`, `finalized_by`, `finalized_date`) VALUES
(1, 1, 100, NOW(), 101, NOW(), 102, NOW());

-- diag_status`
INSERT INTO `diag_status`(`processed_sample_id`, `status`, `user_id`, `date`, `outcome`, `comment`) VALUES
(1,'done', 103, NOW(),'no significant findings','');