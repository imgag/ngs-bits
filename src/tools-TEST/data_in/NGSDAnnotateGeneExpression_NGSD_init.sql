
INSERT INTO `user`(`id`, `user_id`, `password`, `user_role`, `name`, `email`, `created`, `active`, `salt`) VALUES
(99, 'ahmustm1', '4bc26c3f212efe28ad8a58d655afb3f1dabc8eb9', 'user', 'Max Mustermann', 'no.mail@max.de', '2016-07-05', 1, 'salt123456salt123456salt123456salt123456');


INSERT INTO `device` (`id`, `type`, `name`) VALUES
(1, 'MiSeq', 'Neo');


INSERT INTO `sender` (`id`, `name`) VALUES
(1, 'Coriell');


INSERT INTO `project` (`id`, `name`, `type`, `internal_coordinator_id`, `analysis`) VALUES
(1, 'KontrollDNACoriell', 'test', 1, 'variants'),
(2, 'KontrollDNACoriell2', 'test', 1, 'variants');


INSERT INTO `sequencing_run` (`id`, `name`, `fcid`, `device_id`, `recipe`, `status`) VALUES
(1, '#00372', 'AB2J9', 1, '158+8+158', 'analysis_finished');


INSERT INTO `sample` (`id`, `name`, `name_external`, `sample_type`, `tissue`, `species_id`, `gender`, `quality`, `tumor`, `ffpe`, `sender_id`, `comment`, `disease_group`, `disease_status`) VALUES
(1, 'RX001', 'ex1', 'RNA', 'skin', 1, 'female', 'good', 0 ,0, 1, 'comment_s1', 'n/a', 'Affected'),
(2, 'RX002', 'ex2', 'RNA', 'skin', 1, 'female', 'good', 0 ,0, 1, 'comment_s2', 'n/a', 'Affected'),
(3, 'RX003', 'ex3', 'RNA', 'skin', 1, 'male', 'good', 0 ,0, 1, 'comment_s3', 'n/a', 'Affected'),
(4, 'RX004', 'ex4', 'RNA', 'skin', 1, 'male', 'good', 0 ,0, 1, 'comment_s4', 'n/a', 'Affected'),
(5, 'RX005', 'ex5', 'RNA', 'blood', 1, 'female', 'good', 0 ,0, 1, 'comment_s5', 'n/a', 'Affected'),
(6, 'RX006', 'ex6', 'RNA', 'blood', 1, 'female', 'good', 0 ,0, 1, 'comment_s6', 'n/a', 'Affected'),
(7, 'RX007', 'ex7', 'RNA', 'blood', 1, 'male', 'good', 0 ,0, 1, 'comment_s7', 'n/a', 'Affected'),
(8, 'RX008', 'ex8', 'RNA', 'blood', 1, 'male', 'good', 0 ,0, 1, 'comment_s8', 'n/a', 'Affected'),
(11, 'DX001', 'ex1', 'DNA', 'skin', 1, 'female', 'good', 0 ,0, 1, 'comment_s1', 'n/a', 'Affected'),
(12, 'DX002', 'ex2', 'DNA', 'skin', 1, 'female', 'good', 0 ,0, 1, 'comment_s2', 'n/a', 'Affected'),
(13, 'DX003', 'ex3', 'DNA', 'skin', 1, 'male', 'good', 0 ,0, 1, 'comment_s3', 'n/a', 'Affected'),
(14, 'DX004', 'ex4', 'DNA', 'skin', 1, 'male', 'good', 0 ,0, 1, 'comment_s4', 'n/a', 'Affected'),
(15, 'DX005', 'ex5', 'DNA', 'blood', 1, 'female', 'good', 0 ,0, 1, 'comment_s5', 'n/a', 'Affected'),
(16, 'DX006', 'ex6', 'DNA', 'blood', 1, 'female', 'good', 0 ,0, 1, 'comment_s6', 'n/a', 'Affected'),
(17, 'DX007', 'ex7', 'DNA', 'blood', 1, 'male', 'good', 0 ,0, 1, 'comment_s7', 'n/a', 'Affected'),
(18, 'DX008', 'ex8', 'DNA', 'blood', 1, 'male', 'good', 0 ,0, 1, 'comment_s8', 'n/a', 'Affected');


INSERT INTO `sample_relations`(`sample1_id`, `relation`, `sample2_id`) VALUES
(1, 'same sample', 11),
(2, 'same sample', 12),
(3, 'same sample', 13),
(4, 'same sample', 14),
(5, 'same sample', 15),
(6, 'same sample', 16),
(7, 'same sample', 17),
(8, 'same sample', 18);


INSERT INTO `processing_system` (`id`, `name_short`, `name_manufacturer`, `adapter1_p5`, `adapter2_p7`, `type`, `shotgun`, `target_file`, `genome_id`) VALUES
(1, 'nebRNAU2_mrna', 'NEBNext Ultra II Directional RNA mRNA', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'RNA', 1, 'nebRNAU2_mrna_2021_12_13.bed', 1);


INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`, `quality`, `comment`, `normal_id`) VALUES
(5001, 1, 1, 1, '1', 1, 1, 'medium', 'comment_ps1', null),
(5002, 2, 1, 1, '1', 1, 2, 'good', 'comment_ps2', null),
(5003, 3, 1, 1, '1', 1, 1, 'medium', 'comment_ps3', null),
(5004, 4, 1, 1, '1', 1, 2, 'good', 'comment_ps4', null),
(5005, 5, 1, 1, '1', 1, 1, 'medium', 'comment_ps5', null),
(5006, 6, 1, 1, '1', 1, 2, 'good', 'comment_ps6', null),
(5007, 7, 1, 1, '1', 1, 1, 'medium', 'comment_ps7', null),
(5008, 8, 1, 1, '1', 1, 2, 'good', 'comment_ps8', null),
(6001, 11, 1, 1, '1', 1, 1, 'medium', 'comment_ps1', null),
(6002, 12, 1, 1, '1', 1, 2, 'good', 'comment_ps2', null),
(6003, 13, 1, 1, '1', 1, 1, 'medium', 'comment_ps3', null),
(6004, 14, 1, 1, '1', 1, 2, 'good', 'comment_ps4', null),
(6005, 15, 1, 1, '1', 1, 1, 'medium', 'comment_ps5', null),
(6006, 16, 1, 1, '1', 1, 2, 'good', 'comment_ps6', null),
(6007, 17, 1, 1, '1', 1, 1, 'medium', 'comment_ps7', null),
(6008, 18, 1, 1, '1', 1, 2, 'good', 'comment_ps8', null);


INSERT INTO `sample_disease_info` (`id`, `sample_id`, `disease_info`, `type`, `user_id`) VALUES
(1, 1, 'HPO:0123456', 'HPO term id', 99),
(2, 1, 'ICD10 test code 001', 'ICD10 code', 99),
(3, 2, 'HPO:6543210', 'HPO term id', 99),
(4, 2, 'ICD10 test code 002', 'ICD10 code', 99),
(5, 3, 'HPO:0123456', 'HPO term id', 99),
(6, 4, 'ICD10 test code 002', 'ICD10 code', 99),
(7, 15, 'ICD10 test code 001', 'ICD10 code', 99),
(8, 16, 'HPO:6543210', 'HPO term id', 99),
(9, 17, 'ICD10 test code 001', 'ICD10 code', 99),
(10, 18, 'ICD10 test code 002', 'ICD10 code', 99);


INSERT INTO `gene` (`id`, `hgnc_id`, `symbol`, `name`, `type`, `ensembl_id`) VALUES
(1, 32038, 'AADACL4', 'arylacetamide deacetylase like 4', 'protein-coding gene', 'ENSG00000204518'),
(2, 16754, 'ACAP3', 'ArfGAP with coiled-coil, ankyrin repeat and PH domains 3', 'protein-coding gene', 'ENSG00000131584'),
(3, 24157, 'ACOT7', 'acyl-CoA thioesterase 7', 'protein-coding gene', 'ENSG00000097021'),
(4, 24078, 'ANGPTL7', 'angiopoietin like 7', 'protein-coding gene', 'ENSG00000171819'),
(5, 26604, 'ARHGEF19', 'Rho guanine nucleotide exchange factor 19', 'protein-coding gene', 'ENSG00000142632'),
(6, 51528, 'BRWD1P1', 'bromodomain and WD repeat domain containing 1 pseudogene 1', 'pseudogene', 'ENSG00000215909'),
(7, 27915, 'C1ORF174', 'chromosome 1 open reading frame 174', 'protein-coding gene', 'ENSG00000198912'),
(8, 1245, 'C1QC', 'complement C1q C chain', 'protein-coding gene', 'ENSG00000159189'),
(9, 32308, 'C1QTNF12', 'C1q and TNF related 12', 'protein-coding gene', 'ENSG00000184163'),
(10, 24190, 'CAMK2N1', 'calcium/calmodulin dependent protein kinase II inhibitor 1', 'protein-coding gene', 'ENSG00000162545'),
(11, 18806, 'CAMTA1', 'calmodulin binding transcription activator 1', 'protein-coding gene', 'ENSG00000171735'),
(12, 40855, 'CAMTA1-AS2', 'CAMTA1 antisense RNA 2', 'non-coding RNA', 'ENSG00000237728'),
(13, 1511, 'CASP9', 'caspase 9', 'protein-coding gene', 'ENSG00000132906'),
(14, 1736, 'CDC42', 'cell division cycle 42', 'protein-coding gene', 'ENSG00000070831'),
(15, 15944, 'CELA3A', 'chymotrypsin like elastase 3A', 'protein-coding gene', 'ENSG00000142789'),
(16, 29368, 'CFAP74', 'cilia and flagella associated protein 74', 'protein-coding gene', 'ENSG00000142609'),
(17, 37742, 'CICP3', 'capicua transcriptional repressor pseudogene 3', 'pseudogene', 'ENSG00000229376'),
(18, 37756, 'CICP7', 'capicua transcriptional repressor pseudogene 7', 'pseudogene', 'ENSG00000233653'),
(19, 2024, 'CLCN6', 'chloride voltage-gated channel 6', 'protein-coding gene', 'ENSG00000011021'),
(20, 17447, 'CLSTN1', 'calsyntenin 1', 'protein-coding gene', 'ENSG00000171603'),
(21, 55080, 'DDX11L17', 'DEAD/H-box helicase 11 like 17 (pseudogene)', 'pseudogene', 'ENSG00000279928'),
(22, 25054, 'DRAXIN', 'dorsal inhibitory axon guidance protein', 'protein-coding gene', 'ENSG00000162490'),
(23, 55801, 'EFHD2-AS1', 'EFHD2 antisense RNA 1', 'non-coding RNA', 'ENSG00000228140'),
(24, 28957, 'EMC1', 'ER membrane protein complex subunit 1', 'protein-coding gene', 'ENSG00000127463'),
(25, 27635, 'FAM41C', 'family with sequence similarity 41 member C', 'non-coding RNA', 'ENSG00000230368'),
(26, 26717, 'FAM131C', 'family with sequence similarity 131 member C', 'protein-coding gene', 'ENSG00000185519'),
(27, 29249, 'FBXO42', 'F-box protein 42', 'protein-coding gene', 'ENSG00000037637'),
(28, 39381, 'HMGN2P17', 'high mobility group nucleosomal binding domain 2 pseudogene 17', 'pseudogene', 'ENSG00000238249'),
(29, 6229, 'KCNAB2', 'potassium voltage-gated channel subfamily A regulatory beta subunit 2', 'protein-coding gene', 'ENSG00000069424'),
(30, 28513, 'KIAA2013', 'KIAA2013', 'protein-coding gene', 'ENSG00000116685'),
(31, 52433, 'LINC01646', 'long intergenic non-protein coding RNA 1646', 'non-coding RNA', 'ENSG00000232596'),
(32, 52501, 'LINC01714', 'long intergenic non-protein coding RNA 1714', 'non-coding RNA', 'ENSG00000227634'),
(33, 52562, 'LINC01772', 'long intergenic non-protein coding RNA 1772', 'non-coding RNA', 'ENSG00000226029'),
(34, 54047, 'LINC02606', 'long intergenic non-protein coding RNA 2606', 'non-coding RNA', 'ENSG00000284693'),
(35, 54286, 'LINC02766', 'long intergenic non-protein coding RNA 2766', 'non-coding RNA', 'ENSG00000229484'),
(36, 54302, 'LINC02782', 'long intergenic non-protein coding RNA 2782', 'non-coding RNA', 'ENSG00000231510'),
(37, 54303, 'LINC02783', 'long intergenic non-protein coding RNA 2783', 'non-coding RNA', 'ENSG00000204362'),
(38, 54342, 'LINC02810', 'long intergenic non-protein coding RNA 2810', 'non-coding RNA', 'ENSG00000236648'),
(39, 29207, 'LRRC47', 'leucine rich repeat containing 47', 'protein-coding gene', 'ENSG00000130764'),
(40, 54537, 'MFFP1', 'MFF pseudogene 1', 'pseudogene', 'ENSG00000215720'),
(41, 48338, 'MICOS10-NBL1', 'MICOS10-NBL1 readthrough', 'other', 'ENSG00000270136'),
(42, 25715, 'MIIP', 'migration and invasion inhibitory protein', 'protein-coding gene', 'ENSG00000116691'),
(43, 31635, 'MIR34A', 'microRNA 34a', 'non-coding RNA', 'ENSG00000284357'),
(44, 31579, 'MIR200B', 'microRNA 200b', 'non-coding RNA', 'ENSG00000207730'),
(45, 35283, 'MIR1290', 'microRNA 1290', 'non-coding RNA', 'ENSG00000221662'),
(46, 52482, 'MIR1302-2HG', 'MIR1302-2 host gene', 'non-coding RNA', 'ENSG00000243485'),
(47, 38371, 'MIR3115', 'microRNA 3115', 'non-coding RNA', 'ENSG00000263793'),
(48, 38370, 'MIR4251', 'microRNA 4251', 'non-coding RNA', 'ENSG00000283572'),
(49, 38231, 'MIR4253', 'microRNA 4253', 'non-coding RNA', 'ENSG00000264014'),
(50, 41593, 'MIR4632', 'microRNA 4632', 'non-coding RNA', 'ENSG00000263676'),
(51, 7170, 'MMP23A', 'matrix metallopeptidase 23A (pseudogene)', 'pseudogene', 'ENSG00000215914'),
(52, 7171, 'MMP23B', 'matrix metallopeptidase 23B', 'protein-coding gene', 'ENSG00000189409'),
(53, 14478, 'MRPL20', 'mitochondrial ribosomal protein L20', 'protein-coding gene', 'ENSG00000242485'),
(54, 44571, 'MTATP8P1', 'MT-ATP8 pseudogene 1', 'pseudogene', 'ENSG00000240409'),
(55, 42092, 'MTND1P23', 'MT-ND1 pseudogene 23', 'pseudogene', 'ENSG00000225972'),
(56, 7542, 'MXRA8', 'matrix remodeling associated 8', 'protein-coding gene', 'ENSG00000162576'),
(57, 29831, 'NADK', 'NAD kinase', 'protein-coding gene', 'ENSG00000008130'),
(58, 25076, 'NBPF3', 'NBPF member 3', 'protein-coding gene', 'ENSG00000142794'),
(59, 7940, 'NPPB', 'natriuretic peptide B', 'protein-coding gene', 'ENSG00000120937'),
(60, 14825, 'OR4F5', 'olfactory receptor family 4 subfamily F member 5', 'protein-coding gene', 'ENSG00000186092'),
(61, 18368, 'PADI4', 'peptidyl arginine deiminase 4', 'protein-coding gene', 'ENSG00000159339'),
(62, 19366, 'PANK4', 'pantothenate kinase 4 (inactive)', 'protein-coding gene', 'ENSG00000157881'),
(63, 55101, 'PDE4DIPP10', 'PDE4DIP pseudogene 10', 'pseudogene', 'ENSG00000228823'),
(64, 14581, 'PINK1', 'PTEN induced kinase 1', 'protein-coding gene', 'ENSG00000158828'),
(65, 9038, 'PLA2G5', 'phospholipase A2 group V', 'protein-coding gene', 'ENSG00000127472'),
(66, 25284, 'PLEKHN1', 'pleckstrin homology domain containing N1', 'protein-coding gene', 'ENSG00000187583'),
(67, 28840, 'PRAMEF1', 'PRAME family member 1', 'protein-coding gene', 'ENSG00000116721'),
(68, 28841, 'PRAMEF2', 'PRAME family member 2', 'protein-coding gene', 'ENSG00000120952'),
(69, 24074, 'PRAMEF8', 'PRAME family member 8', 'protein-coding gene', 'ENSG00000182330'),
(70, 30693, 'PRAMEF18', 'PRAME family member 18', 'protein-coding gene', 'ENSG00000279804'),
(71, 24908, 'PRAMEF19', 'PRAME family member 19', 'protein-coding gene', 'ENSG00000204480'),
(72, 49179, 'PRAMEF25', 'PRAME family member 25', 'protein-coding gene', 'ENSG00000229571'),
(73, 51891, 'PRAMEF29P', 'PRAME family member 29, pseudogene', 'pseudogene', 'ENSG00000234064'),
(74, 9347, 'PRDM2', 'PR/SET domain 2', 'protein-coding gene', 'ENSG00000116731'),
(75, 30309, 'RER1', 'retention in endoplasmic reticulum sorting receptor 1', 'protein-coding gene', 'ENSG00000157916'),
(76, 46293, 'RN7SL277P', 'RNA, 7SL, cytoplasmic 277, pseudogene', 'pseudogene', 'ENSG00000240490'),
(77, 41127, 'RNF186-AS1', 'RNF186 antisense RNA 1', 'non-coding RNA', 'ENSG00000235434'),
(78, 32947, 'RNF207', 'ring finger protein 207', 'protein-coding gene', 'ENSG00000158286'),
(79, 10130, 'RNU1-3', 'RNA, U1 small nuclear 3', 'non-coding RNA', 'ENSG00000207513'),
(80, 34281, 'RNU6-37P', 'RNA, U6 small nuclear 37, pseudogene', 'pseudogene', 'ENSG00000199562'),
(81, 47098, 'RNU6-135P', 'RNA, U6 small nuclear 135, pseudogene', 'pseudogene', 'ENSG00000252578'),
(82, 47740, 'RNU6-777P', 'RNA, U6 small nuclear 777, pseudogene', 'pseudogene', 'ENSG00000201135'),
(83, 45734, 'RNU7-200P', 'RNA, U7 small nuclear 200 pseudogene', 'pseudogene', 'ENSG00000251914'),
(84, 35667, 'RPL7P11', 'ribosomal protein L7 pseudogene 11', 'pseudogene', 'ENSG00000234619'),
(85, 36708, 'RPL23AP19', 'ribosomal protein L23a pseudogene 19', 'pseudogene', 'ENSG00000229716'),
(86, 36176, 'RPL23AP24', 'ribosomal protein L23a pseudogene 24', 'pseudogene', 'ENSG00000236679'),
(87, 10458, 'RSC1A1', 'regulator of solute carriers 1', 'protein-coding gene', 'ENSG00000215695'),
(88, 13445, 'SLC2A7', 'solute carrier family 2 member 7', 'protein-coding gene', 'ENSG00000197241'),
(89, 53623, 'SLC25A34-AS1', 'SLC25A34 and TMEM82 antisense RNA 1', 'non-coding RNA', 'ENSG00000224459'),
(90, 42631, 'TBCAP2', 'tubulin folding cofactor A pseudogene 2', 'pseudogene', 'ENSG00000235829'),
(91, 20855, 'THAP3', 'THAP domain containing 3', 'protein-coding gene', 'ENSG00000041988'),
(92, 37099, 'TMEM88B', 'transmembrane protein 88B', 'protein-coding gene', 'ENSG00000205116'),
(93, 33719, 'TMEM201', 'transmembrane protein 201', 'protein-coding gene', 'ENSG00000188807'),
(94, 53732, 'TMEM274P', 'transmembrane protein 274, pseudogene', 'pseudogene', 'ENSG00000283611'),
(95, 11924, 'TNFRSF9', 'TNF receptor superfamily member 9', 'protein-coding gene', 'ENSG00000049249'),
(96, 40591, 'TP73-AS2', 'TP73 antisense RNA 2', 'non-coding RNA', 'ENSG00000235131'),
(97, 34297, 'TTC34', 'tetratricopeptide repeat domain 34', 'protein-coding gene', 'ENSG00000215912'),
(98, 26693, 'TTLL10', 'tubulin tyrosine ligase like 10', 'protein-coding gene', 'ENSG00000162571'),
(99, 18533, 'USP48', 'ubiquitin specific peptidase 48', 'protein-coding gene', 'ENSG00000090686'),
(100, 12644, 'VAMP3', 'vesicle associated membrane protein 3', 'protein-coding gene', 'ENSG00000049245'),
(101, 12783, 'WNT4', 'Wnt family member 4', 'protein-coding gene', 'ENSG00000162552'),
(102, 29045, 'ZBTB40', 'zinc finger and BTB domain containing 40', 'protein-coding gene', 'ENSG00000184677');


