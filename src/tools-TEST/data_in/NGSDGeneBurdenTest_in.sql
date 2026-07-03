
INSERT INTO `user`(`id`, `user_id`, `password`, `user_role`, `name`, `email`, `created`, `active`) VALUES (99, 'ahmustm1', '', 'user', 'Max Mustermann', '', '2016-07-05', 1);

INSERT INTO `device` (`id`, `type`, `name`) VALUES (1, 'MiSeq', 'Neo');

INSERT INTO `sender` (`id`, `name`) VALUES (1, 'Coriell');

INSERT INTO `project` (`id`, `name`, `type`, `internal_coordinator_id`, `analysis`) VALUES (1, 'KontrollDNACoriell', 'test', 1, 'variants');

INSERT INTO `sequencing_run` (`id`, `name`, `fcid`, `device_id`, `recipe`, `status`) VALUES (1, '#00372', 'AB2J9', 1, '158+8+158', 'analysis_finished');

INSERT INTO `sample` (`id`, `name`, `sample_type`, `species_id`, `gender`, `quality`, `tumor`, `ffpe`, `sender_id`) VALUES 
(100, 'case00', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(101, 'case01', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(102, 'case02', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(103, 'case03', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(104, 'case04', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(105, 'case05', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(106, 'case06', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(107, 'case07', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(108, 'case08', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(109, 'case09', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(200, 'control00', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(201, 'control01', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(202, 'control02', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(203, 'control03', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(204, 'control04', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(205, 'control05', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(206, 'control06', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(207, 'control07', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(208, 'control08', 'DNA', 1, 'n/a', 'good', 0 ,0, 1),
(209, 'control09', 'DNA', 1, 'n/a', 'good', 0 ,0, 1);

INSERT INTO `processing_system` (`id`, `name_short`, `name_manufacturer`, `adapter1_p5`, `adapter2_p7`, `type`, `shotgun`, `target_file`, `genome_id`) VALUES (1, 'hpHBOCv5', 'HaloPlex HBOC v5', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC ', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel HaloPlex', 0, '/mnt/share/data/enrichment/hpHBOCv5_2014_10_27.bed', 1);

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`) VALUES 
(100, 100, 2, 1, '1', 1, 1),
(101, 101, 2, 1, '1', 1, 1),
(102, 102, 2, 1, '1', 1, 1),
(103, 103, 2, 1, '1', 1, 1),
(104, 104, 2, 1, '1', 1, 1),
(105, 105, 2, 1, '1', 1, 1),
(106, 106, 2, 1, '1', 1, 1),
(107, 107, 2, 1, '1', 1, 1),
(108, 108, 2, 1, '1', 1, 1),
(109, 109, 2, 1, '1', 1, 1),
(200, 200, 2, 1, '1', 1, 1),
(201, 201, 2, 1, '1', 1, 1),
(202, 202, 2, 1, '1', 1, 1),
(203, 203, 2, 1, '1', 1, 1),
(204, 204, 2, 1, '1', 1, 1),
(205, 205, 2, 1, '1', 1, 1),
(206, 206, 2, 1, '1', 1, 1),
(207, 207, 2, 1, '1', 1, 1),
(208, 208, 2, 1, '1', 1, 1),
(209, 209, 2, 1, '1', 1, 1);

INSERT INTO `gene` (`id`, `hgnc_id`, `symbol`, `name`, `type`, `ensembl_id`, `ncbi_id`) VALUES
(704635, 14872, 'ASPN', 'asporin', 'protein-coding gene', 'ENSG00000106819', 54829),
(699917, 29198, 'KIAA1143', 'KIAA1143', 'protein-coding gene', 'ENSG00000163807', 57456),
(700013, 6332, 'KIR2DL4', 'killer cell immunoglobulin like receptor, two Ig domains and long cytoplasmic tail 4', 'protein-coding gene', 'ENSG00000189013', 3805),
(701402, 49024, 'LINC01036', 'long intergenic non-protein coding RNA 1036', 'non-coding RNA', 'ENSG00000230426', 104169671),
(707961, 27814, 'MROH6', 'maestro heat like repeat family member 6', 'protein-coding gene', 'ENSG00000204839', 642475),
(713911, 9347, 'PRDM2', 'PR/SET domain 2', 'protein-coding gene', 'ENSG00000116731', 7799),
(723167, 10700, 'SEC22B', 'SEC22 homolog B, vesicle trafficking protein', 'protein-coding gene', 'ENSG00000265808', 9554),
(723645, 30422, 'SHROOM3', 'shroom family member 3', 'protein-coding gene', 'ENSG00000138771', 57619),
(727223, 18641, 'SUGP2', 'SURP and G-patch domain containing 2', 'protein-coding gene', 'ENSG00000064607', 10147),
(730912, 30600, 'UBXN11', 'UBX domain protein 11', 'protein-coding gene', 'ENSG00000158062', 91544),
(736658, 33879, 'DNLZ', 'DNL-type zinc finger', 'protein-coding gene', 'ENSG00000213221', 728489),
(723170, 53892, 'SEC22B4P', 'SEC22 homolog B4, pseudogene', 'pseudogene', 'ENSG00000277406', 102724364);


-- HIGH impact variants
-- KIR2DL4
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(1001, 'chr19', 54813219, 54813219, '-', 'A', 0.0001, 1.0, 0.25, 15, 5, 3);    
-- MROH6
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(1002, 'chr8', 143567372, 143567372, 'C', '-', 0.0001, 1.0, 0.25, 10, 5, 3); 

-- MODERATE impact variants
-- SEC22B
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(801, 'chr1', 120157112, 120157112, 'T', 'G', 0.0001, 1.0, 0.25, 10, 5, 3);
-- PRDM2
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(802, 'chr1', 13778644, 13778644, 'T', 'A', 0.0001, 32.1, 0.25, 10, 5, 3),    
(803, 'chr1', 13779899, 13779899, '-', 'CTC', 0.0001, 1.0, 0.25, 10, 5, 3);
-- UBXN11
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(804, 'chr1', 26282321, 26282398, 'CCAGGACAGGGACTGGGGCCGGGACCGGGACCGGGACTGGGGCCGGGACCGGGACCGGGACTGGGGCCGGGACCGGGA', '-', 0.0001, 1.0, 0.25, 10, 5, 3), 
(805, 'chr1', 26282329, 26282352, 'GGGACTGGGGCCGGGACCGGGACC', '-', 0.0001, 1.0, 0.25, 10, 5, 3), 
(806, 'chr1', 26284400, 26284400, 'A', 'C', 0.0001, 1.0, 0.25, 10, 5, 3);
-- MROH6
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(807, 'chr8', 143567798, 143567798, 'C', 'T', 0.0001, 1.0, 0.5, 10, 5, 3),
(808, 'chr8', 143572079, 143572079, 'G', 'A', 0.0001, 30.0, 0.25, 10, 8, 3),
(809, 'chr8', 143572085, 143572085, 'A', 'G', 0.0001, 1.0, 0.25, 10, 5, 3),
(810, 'chr8', 143572424, 143572424, 'G', 'T', 0.0001, 1.0, 0.25, 10, 5, 3);
-- ASPN
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(811, 'chr9', 92474742, 92474742, '-', 'TCA', 0.0001, 42.3, 0.5, 10, 5, 3);

-- LOW impact variants
-- SEC22B
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(501, 'chr1', 120168929, 120168929, 'T', 'C', 0.0001, 1.0, 0.25, 10, 5, 3);
-- PRDM2
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(502, 'chr1', 13732900, 13732900, 'T', 'C', 0.0001, 1.0, 0.25, 10, 5, 3),
(503, 'chr1', 13778803, 13778803, 'G', 'A', 0.0001, 1.0, 0.25, 10, 5, 3),
(504, 'chr1', 13782619, 13782619, 'A', 'G', 0.0001, 1.0, 0.25, 10, 5, 3),
(505, 'chr1', 13816508, 13816508, 'A', 'G', 0.0001, 1.0, 0.25, 10, 5, 3);
-- UBXN11
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(506, 'chr1', 26294355, 26294355, '-', 'A', 0.0001, 1.0, 0.25, 10, 5, 3);
-- MROH6
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(507, 'chr8', 143567380, 143567380, 'A', 'G', 0.0001, 1.0, 0.25, 10, 5, 3),
(508, 'chr8', 143570223, 143570223, '-', 'CCCTCCTCA', 0.0001, 1.0, 0.25, 10, 5, 3),
(509, 'chr8', 143572416, 143572416, 'G', 'C', 0.0001, 1.0, 0.25, 10, 5, 3);
-- ASPN
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(510, 'chr9', 92457315, 92457315, 'G', 'A', 0.0001, 1.0, 0.25, 10, 5, 3),
(511, 'chr9', 92474940, 92474940, 'G', 'T', 0.0001, 1.0, 0.25, 10, 5, 3);

-- MODIFIER impact variants
-- SEC22B
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(102, 'chr1', 120157248, 120157248, 'A', 'T', 0.0001, 1.0, 0.25, 10, 5, 3),
(103, 'chr1', 120163401, 120163401, 'A', 'G', 0.0001, 1.0, 0.25, 10, 5, 3);
-- PRDM2
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(109, 'chr1', 13823106, 13823106, 'C', 'T', 0.0001, 1.0, 0.25, 10, 5, 3),
(110, 'chr1', 13823127, 13823127, 'T', 'C', 0.0001, 1.0, 0.25, 10, 5, 3),
(111, 'chr1', 13830065, 13830065, 'C', 'T', 0.0001, 1.0, 0.25, 10, 5, 3);
-- LINC01036
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(112, 'chr1', 146325275, 146325275, 'G', 'T', 0.0001, 1.0, 0.25, 10, 5, 3),
(113, 'chr1', 187563312, 187563312, '-', 'GTGGGCTC', 0.0001, 1.0, 0.25, 10, 5, 3),
(114, 'chr1', 187563443, 187563443, 'C', 'T', 0.0001, 1.0, 0.25, 10, 5, 3);
-- UBXN11
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(115, 'chr1', 26282291, 26282291, 'C', 'T', 0.0001, 1.0, 0.25, 10, 5, 3),
(120, 'chr1', 26320235, 26320235, 'A', 'G', 0.0001, 1.0, 0.25, 10, 5, 3);
-- SUGP2
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(121, 'chr19', 18994322, 18994322, 'G', 'A', 0.0001, 1.0, 0.25, 10, 5, 3);
-- KIAA1143
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
-- (122, 'chr19', 54813219, 54813219, '-', 'A', 0.0001, 1.0, 0.25, 10, 5, 3),
(123, 'chr3', 44761617, 44761623, 'AAGACAG', '-', 0.0001, 1.0, 0.25, 10, 5, 3);
-- SHROOM3
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(124, 'chr4', 76759610, 76759610, 'C', 'T', 0.0001, 1.0, 0.25, 10, 5, 3);
-- MROH6
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(125, 'chr8', 143563095, 143563095, '-', 'CTGG', 0.0001, 1.0, 0.25, 10, 5, 3),
(130, 'chr8', 143570841, 143570841, 'C', 'T', 0.0001, 1.0, 0.25, 10, 5, 3),
(131, 'chr8', 143571048, 143571048, 'A', 'G', 0.0001, 1.0, 0.25, 10, 5, 3);
-- DNLZ
INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `gnomAD`, `cadd`, `spliceai`, `germline_het`, `germline_hom`, `germline_mosaic`) VALUES
(136, 'chr9', 136363734, 136363734, '-', 'GCCCTGCCCCG', 0.0001, 1.0, 0.25, 10, 5, 3); 


-- Insert detected variants
-- KIR2DL4
INSERT INTO `detected_variant` (`processed_sample_id`, `variant_id`, `genotype`) VALUES
(100, 1001, 'hom'),
(101, 1001, 'het'),
(102, 1001, 'het'),
(103, 1001, 'hom'),
(104, 1001, 'het'),
(105, 1001, 'hom'),
(106, 1001, 'het'),
(107, 1001, 'het'),
(108, 1001, 'het'),
(109, 1001, 'hom'),
(200, 1001, 'hom'),
(201, 1001, 'het'),
(202, 1001, 'het'),
(203, 1001, 'het'),
(204, 1001, 'het');
-- MROH6
INSERT INTO `detected_variant` (`processed_sample_id`, `variant_id`, `genotype`) VALUES
(100, 1002, 'hom'),
(101, 807, 'het'),
(101, 808, 'het'),
(103, 507, 'hom'),
(104, 508, 'het'),
(104, 509, 'het'),
(105, 125, 'hom'),
(106, 1002, 'het'),
(107, 507, 'het'),
(108, 807, 'het'),
(109, 508, 'hom'),
(200, 807, 'hom'),
(201, 130, 'het'),
(202, 131, 'het'),
(203, 808, 'het'),
(204, 509, 'het');
-- ASPN
INSERT INTO `detected_variant` (`processed_sample_id`, `variant_id`, `genotype`) VALUES
(200, 811, 'hom'),
(201, 811, 'het'),
(202, 811, 'het'),
(203, 510, 'hom'),
(204, 510, 'het'),
(205, 511, 'hom'),
(206, 511, 'het'),
(207, 511, 'het'),
(208, 511, 'het'),
(209, 511, 'hom'),
(100, 811, 'hom'),
(101, 811, 'het'),
(102, 811, 'het'),
(103, 811, 'het'),
(104, 811, 'het');
-- SEC22B
INSERT INTO `detected_variant` (`processed_sample_id`, `variant_id`, `genotype`) VALUES
(100, 801, 'hom'),
(101, 501, 'het'),
(101, 102, 'het'),
(103, 102, 'hom');
-- SUGP2
INSERT INTO `detected_variant` (`processed_sample_id`, `variant_id`, `genotype`) VALUES
(100, 121, 'hom'),
(101, 121, 'het'),
(201, 121, 'het'),
(203, 121, 'hom');
-- PRDM2
INSERT INTO `detected_variant` (`processed_sample_id`, `variant_id`, `genotype`) VALUES
(100, 802, 'hom'),
(101, 803, 'het'),
(102, 502, 'het'),
(103, 503, 'hom'),
(104, 504, 'het'),
(105, 505, 'hom'),
(106, 109, 'het'),
(107, 110, 'het'),
(108, 503, 'het'),
(109, 803, 'hom'),
(200, 504, 'hom'),
(201, 109, 'het'),
(202, 110, 'het'),
(203, 111, 'het');

--CNVs`
INSERT INTO `cnv_callset` (`id`, `processed_sample_id`, `caller`, `caller_version`, `quality_metrics`) VALUES 
(1, 109, 'ClinCNV', '1.16.2', '{"mean correlation to reference samples":"0.91"}');

INSERT INTO `cnv` (`id`, `cnv_callset_id`, `chr`, `start`, `end`, `cn`, `quality_metrics`) VALUES
(1, 1, 'chr19', 54810000, 54820000, 0, '{"loglikelihood":"150","regions":"10"}'),
(2, 1, 'chr8', 143560000, 143566000, 1, '{"loglikelihood":"50","regions":"3"}'),
(3, 1, 'chr9', 92474000, 92479000, 0, '{"loglikelihood":"140","regions":"10"}'),
(4, 1, 'chr1', 120140000, 120180000, 1, '{"loglikelihood":"180","regions":"5"}');

