
INSERT INTO `user`(`id`, `user_id`, `password`, `user_role`, `name`, `email`, `created`, `active`) VALUES (99, 'ahmustm1', '', 'user', 'Max Mustermann', '', '2016-07-05', 1);

INSERT INTO `device` (`id`, `type`, `name`) VALUES (1, 'MiSeq', 'Neo');

INSERT INTO `sender` (`id`, `name`) VALUES (1, 'Coriell');

INSERT INTO `project` (`id`, `name`, `type`, `internal_coordinator_id`, `analysis`) VALUES (1, 'KontrollDNACoriell', 'test', 1, 'variants');

INSERT INTO `sequencing_run` (`id`, `name`, `fcid`, `device_id`, `recipe`, `status`) VALUES (1, '#00372', 'AB2J9', 1, '158+8+158', 'analysis_finished');

INSERT INTO `sample` (`id`, `name`, `sample_type`, `species_id`, `gender`, `quality`, `tumor`, `ffpe`, `sender_id`) VALUES 
(1, 'NORMAL', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(2, 'TUMOR', 'DNA', 1, 'n/a', 'good', 1 ,1, 1),
(3, 'BADQUAL', 'DNA', 1, 'n/a', 'good', 0 ,0, 1);

INSERT INTO `processing_system` (`id`, `name_short`, `name_manufacturer`, `adapter1_p5`, `adapter2_p7`, `type`, `shotgun`, `target_file`, `genome_id`) VALUES (1, 'hpHBOCv5', 'HaloPlex HBOC v5', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC ', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel HaloPlex', 0, '/mnt/share/data/enrichment/hpHBOCv5_2014_10_27.bed', 1);

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`, `quality`) VALUES 
(1, 1, 1, 1, '1', 1, 1, 'good'),
(2, 2, 1, 1, '1', 1, 1, 'good'),
(3, 3, 1, 1, '1', 1, 1, 'bad');

INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `coding`) VALUES
(6, 'chr10', 43613843, 43613843, 'G', 'T', 0.7653, '');

INSERT INTO `detected_variant` (`processed_sample_id`, `variant_id`, `genotype`) VALUES
(1, 6, 'hom'),
(2, 6, 'hom'),
(3, 6, 'het');


INSERT INTO `qc_terms` (`id`, `qcml_id`, `name`, `description`, `type`, `obsolete`) VALUES
(4, 'QC:2000023', 'insert size', 'Average insert size (for paired-end reads only).', 'float', 0),
(5, 'QC:2000020', 'mapped read percentage', 'Percentage of reads that could be mapped to the reference genome.', 'float', 0),
(16, 'QC:2000005', 'read count', 'Total number of reads (one cluster in a paired-end experiment generates two reads).', 'float', 0);

INSERT INTO `processed_sample_qc` (`id`, `processed_sample_id`, `qc_terms_id`, `value`) VALUES
(991747, 1, 4, '176.57'),
(991744, 2, 5, '99.77'),
(991735, 3, 16, '1059810');

INSERT INTO `gene` (`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES
(81674, 1100, 'BRCA1', 'BRCA1, DNA repair associated', 'protein-coding gene'),
(79279, 37133, 'A1BG-AS1', 'A1BG antisense RNA 1', 'non-coding RNA');

INSERT INTO `gene_alias` (`gene_id`, `symbol`, `type`) VALUES
(79279, 'NCRNA00181', 'previous'),
(79279, 'A1BGAS', 'previous'),
(79279, 'A1BG-AS', 'previous'),
(79279, 'FLJ23569', 'synonym');

INSERT INTO `geneinfo_germline` (`symbol`, `inheritance`, `gnomad_oe_mis`, `gnomad_oe_syn`, `gnomad_oe_lof`, `comments`) VALUES
('BRCA1', 'AD', 0.11, 0.22, 0.33, '28.04.2016 ahmustm1]\nHPO: Autosomal dominant inheritance'),
('A1BG-AS', 'AD', 0.11, 0.22, 0.33, '28.04.2016 ahmustm1]\nHPO: Autosomal dominant inheritance');

INSERT INTO `hpo_term` (`id`, `hpo_id`, `name`, `definition`, `synonyms`) VALUES
(11495, 'HP:0000003', 'Multicystic kidney dysplasia', '\"Multicystic dysplasia of the kidney is characterized by multiple cysts of varying size in the kidney and the absence of a normal pelvocaliceal system. The condition is associated with ureteral or ureteropelvic atresia, and the affected kidney is nonfunctional.', '');

INSERT INTO `hpo_genes` (`hpo_term_id`, `gene`, `details`, `evidence`) VALUES
(11495, 'BRCA1', '(OMIM,(3), high)', 'high'),
(11495, 'A1BG-AS', '(OMIM,(1),low)', 'low');

INSERT INTO `omim_gene` (`id`, `gene`, `mim`) VALUES
(95435, 'BRCA1', '113705'),
(99999, 'A1BG-AS', '999999');

INSERT INTO `sample_disease_info` (`id`, `sample_id`, `disease_info`, `type`, `user_id`, `date`) VALUES
(12, 1, 'HP:0000003', 'HPO term id', 99, '2018-11-21 18:26:26'),
(13, 1, 'C34.9', 'ICD10 code', 99, '2018-11-21 18:26:26'),
(14, 1, '60', 'tumor fraction', 99, '2018-11-21 18:26:26'),
(15, 2, 'G11.9', 'ICD10 code', 99, '2018-11-21 18:35:57'),
(16, 2, 'HP:0001272', 'HPO term id', 99, '2018-11-21 18:35:57');
