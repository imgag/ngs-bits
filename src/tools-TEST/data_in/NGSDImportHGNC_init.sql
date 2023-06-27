INSERT INTO `user`(`id`, `user_id`, `password`, `user_role`, `name`, `email`, `created`, `active`, `salt`) VALUES
(99, 'ahmustm1', '4bc26c3f212efe28ad8a58d655afb3f1dabc8eb9', 'user', 'Max Mustermann', 'no.mail@max.de', '2016-07-05', 1, 'salt123456salt123456salt123456salt123456');


INSERT INTO `device` (`id`, `type`, `name`) VALUES
(1, 'MiSeq', 'Neo');

INSERT INTO `sender` (`id`, `name`) VALUES
(1, 'Coriell');

INSERT INTO `project` (`id`, `name`, `type`, `internal_coordinator_id`, `analysis`) VALUES
(1, 'KontrollDNACoriell', 'test', 1, 'variants');

INSERT INTO `sequencing_run` (`id`, `name`, `fcid`, `start_date`, `device_id`, `recipe`, `status`) VALUES
(1, '#00372', 'AB2J9', '2021-02-19', 1, '158+8+158', 'analysis_finished');

INSERT INTO `sample` (`id`, `name`, `name_external`, `sample_type`, `species_id`, `gender`, `quality`, `tumor`, `ffpe`, `sender_id`, `comment`, `disease_group`, `disease_status`, `tissue`, `patient_identifier`) VALUES
(1, 'NA12878', 'ex1', 'DNA', 1, 'female', 'good', 0 ,0, 1, 'comment_s1', 'Diseases of the blood or blood-forming organs', 'Unaffected', 'blood', 'pat1');

INSERT INTO `processing_system` (`id`, `name_short`, `name_manufacturer`, `adapter1_p5`, `adapter2_p7`, `type`, `shotgun`, `target_file`, `genome_id`) VALUES
(1, 'hpHBOCv5', 'HaloPlex HBOC v5', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel Haloplex', 0, 'hpHBOCv5.bed', 1);

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`, `quality`, `comment`, `normal_id`) VALUES
(3999, 1, 3, 1, '1', 1, 1, 'medium', 'comment_ps1', null),
(4000, 1, 4, 1, '1', 1, 1, 'medium', 'comment_ps2', null);


INSERT INTO `geneinfo_germline`(`symbol`, `inheritance`, `gnomad_oe_mis`, `gnomad_oe_syn`, `gnomad_oe_lof`, `comments`) VALUES
('BRCA1', 'AD', 0.00, 0.00, 0.00, 'valid => keep'),
('RNF53', 'AD', NULL, NULL, NULL, 'old name of BRCA1 => delete'),
('BLABLA', 'AD', NULL, NULL, NULL, 'unknown => delete'),
('FANCD1', 'AD', NULL, NULL, NULL, 'old name of BRCA2 => update');


INSERT INTO `somatic_gene_role` (`symbol`, `gene_role`, `high_evidence`, `comment`) VALUES
('BRCA1', 'loss_of_function', true, 'valid => keep'),
('RNF53', 'loss_of_function', true, 'old name of BRCA1 => delete'),
('BLABLA', 'ambiguous', false, 'unknown => delete'),
('FANCD1', 'loss_of_function', true, 'old name of BRCA2 => update');

-- somatic_pathway
INSERT INTO `somatic_pathway` (`id`, `name`) VALUES
(1, 'DNA Damage Repair'),
(2, 'DNA Damage Repair old'),
(3, 'unknown pathway'),
(4, 'alternative pathway');

INSERT INTO `somatic_pathway_gene` (`symbol`, `pathway_id`) VALUES
('BRCA1', 1),
('RNF53', 2),
('BLABLA', 3),
('FANCD1', 1),
('FANCD1', 4);

INSERT INTO `expression_gene` (`id`, `symbol`) VALUES
(1, 'BRCA1'),
(2, 'RNF53'),
(3, 'BLABLA'),
(4, 'FANCD1');

INSERT INTO `expression` (`symbol_id`, `processed_sample_id`, `tpm`, `raw`) VALUES
(1, 3999, 8.765, 1),
(2, 3999, 9.87654, 3),
(3, 3999, 2.584, 4),
(4, 3999, 2.3456, 7),
(4, 4000, 1.23456, 987);
