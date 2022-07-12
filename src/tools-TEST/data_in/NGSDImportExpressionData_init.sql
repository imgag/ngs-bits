INSERT INTO `user`(`id`, `user_id`, `password`, `user_role`, `name`, `email`, `created`, `active`) 
VALUES (99, 'ahmustm1', '', 'user', 'Max Mustermann', '', '2016-07-05', 1);

INSERT INTO `device` (`id`, `type`, `name`) 
VALUES (1, 'MiSeq', 'Neo');

INSERT INTO `sender` (`id`, `name`) 
VALUES (1, 'Coriell');

INSERT INTO `project` (`id`, `name`, `type`, `internal_coordinator_id`, `analysis`) 
VALUES (1, 'KontrollDNACoriell', 'test', 1, 'variants');

INSERT INTO `sequencing_run` (`id`, `name`, `fcid`, `device_id`, `recipe`, `status`) 
VALUES (1, '#00372', 'AB2J9', 1, '158+8+158', 'analysis_finished');

INSERT INTO `sample` (`id`, `name`, `sample_type`, `species_id`, `gender`, `quality`, `tumor`, `ffpe`, `sender_id`) 
VALUES (1, 'RX123456', 'RNA', 1, 'n/a', 'good', 0 ,0, 1);

INSERT INTO `processing_system` (`id`, `name_short`, `name_manufacturer`, `adapter1_p5`, `adapter2_p7`, `type`, `shotgun`, `target_file`, `genome_id`) 
VALUES (1, 'hpHBOCv5', 'HaloPlex HBOC v5', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC ', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel HaloPlex', 0, '/mnt/share/data/enrichment/hpHBOCv5_2014_10_27.bed', 1);

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`) 
VALUES (3999, 1, 3, 1, '1', 1, 1);