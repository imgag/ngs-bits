
INSERT INTO `user`(`id`, `user_id`, `password`, `user_role`, `name`, `email`, `created`, `active`, `salt`) VALUES
(99, 'ahmustm1', '4bc26c3f212efe28ad8a58d655afb3f1dabc8eb9', 'user', 'Max Mustermann', 'no.mail@max.de', '2016-07-05', 1, 'salt123456salt123456salt123456salt123456'),
(101, 'ahkerra1', '', 'user', 'Sarah Kerrigan', 'queen_of_blades@the_swarm.org', '1998-03-31', 0, NULL);


INSERT INTO `device` (`id`, `type`, `name`) VALUES
(1, 'MiSeq', 'Neo');

INSERT INTO `sender` (`id`, `name`) VALUES
(1, 'Coriell');

INSERT INTO `project` (`id`, `name`, `type`, `internal_coordinator_id`, `analysis`) VALUES
(1, 'KontrollDNACoriell', 'test', 1, 'variants');

INSERT INTO `sequencing_run` (`id`, `name`, `fcid`, `start_date`, `device_id`, `recipe`, `status`) VALUES
(1, '#00372', 'AB2J9', '2021-02-19', 1, '158+8+158', 'analysis_finished');

INSERT INTO `sample` (`id`, `name`, `name_external`, `sample_type`, `species_id`, `gender`, `quality`, `tumor`, `ffpe`, `sender_id`, `comment`, `disease_group`, `disease_status`) VALUES
(1, 'NA12878', 'ex1', 'DNA', 1, 'female', 'good', 0 ,0, 1, 'comment_s1', 'Diseases of the blood or blood-forming organs', 'Unaffected'),
(2, 'NA12123', 'ex2', 'DNA', 1, 'female', 'good', 0 ,0, 1, 'comment_s2', 'Neoplasms', 'Affected'),
(3, 'NA12345', 'ex3', 'DNA', 1, 'male', 'bad', 1 ,1, 1, 'comment_s3', 'Diseases of the immune system', 'Affected'),
(4, 'NA12123repeat', 'ex4', 'DNA', 1, 'female', 'good', 0 ,0, 1, 'comment_s4', 'Neoplasms', 'Affected'),
(5, 'DX184894', 'ex5', 'DNA', 1, 'female', 'good', 1, 1, 1, 'comment_s5', 'Neoplasms', 'Affected'),
(6, 'DX184263', 'ex6', 'DNA', 1, 'female', 'good', 0, 0, 1, 'comment_s6', 'Neoplasms', 'Affected');

INSERT INTO `processing_system` (`id`, `name_short`, `name_manufacturer`, `adapter1_p5`, `adapter2_p7`, `type`, `shotgun`, `target_file`, `genome_id`) VALUES
(1, 'hpHBOCv5', 'HaloPlex HBOC v5', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel Haloplex', 0, 'hpHBOCv5.bed', 1),
(2, 'hpHBOCv6', 'HaloPlex HBOC v6', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel Haloplex', 0, 'hpHBOCv6.bed', 1),
(3, 'ssSC_vTEST', 'SureSelect Somatic vTEST', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'Panel', 1, '/mnt/share/data/enrichment/ssSC_test.bed', 1),
(4, 'IDT_xGenPrism', 'IDT xGen Human ID + IDT xGen Prism DNA', 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCA', 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT', 'cfDNA (patient-specific)', 1, 'idt_HumanID.bed', 1);

INSERT INTO `processed_sample`(`id`, `sample_id`, `process_id`, `sequencing_run_id`, `lane`, `processing_system_id`, `project_id`, `quality`, `comment`, `normal_id`) VALUES
(3999, 1, 3, 1, '1', 1, 1, 'medium', 'comment_ps1', null),
(4000, 1, 4, 1, '1', 1, 1, 'medium', 'comment_ps2', null),
(4001, 2, 4, 1, '1', 1, 1, 'medium', 'comment_ps3', null),
(4002, 3, 1, 1, '1', 1, 1, 'good', 'comment_ps4', 3999),
(4003, 4, 1, 1, '1', 1, 1, 'good', 'comment_ps4', null),
(5, 2, 23, 1, '1', 1, 1, 'medium', 'comment_ps5', null),
(6, 3, 44, 1, '1', 1, 1, 'medium', 'comment_ps6', null),
(7, 6, 1, '1', '1,2,3,4', 3, 1, 'good', 'comment_ps7', null),
(8, 5, 1, '1', '1,2,3,4', 3, 1, 'good', 'comment_ps8', 7);

INSERT INTO `diag_status`(`processed_sample_id`, `status`, `user_id`, `date`, `outcome`, `comment`) VALUES
(3999, 'done', 99, '2014-07-29 09:40:49', 'no significant findings', "free text");

INSERT INTO `gene`(`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES
(1,1001, 'BRCA1','Breast cancer associated gene 1', 'protein-coding gene'),
(2,1002, 'BRCA2','Breast cancer associated gene 2', 'protein-coding gene'),
(3,1003, 'NIPA1', 'non imprinted in Prader-Willi/Angelman syndrome 1', 'protein-coding gene'),
(4,1004, 'NON-CODING', 'non-coding RNA', 'non-coding RNA'),
(415153, 3418, 'EPRS', 'glutamyl-prolyl-tRNA synthetase', 'protein-coding gene'),
(427667, 7421, 'MT-CO2', 'mitochondrially encoded cytochrome c oxidase II', 'protein-coding gene'),
(433223, 9605, 'PTGS2', 'prostaglandin-endoperoxide synthase 2', 'protein-coding gene'),
(433401, 9751, 'QARS1', 'glutaminyl-tRNA synthetase 1', 'protein-coding gene'),
(496483, 2682, 'DAZ1', 'deted in azoospermia 1', 'protein-coding gene'),
(498651, 18403, 'FAM9A', 'family with sequence similarity 9 member A', 'protein-coding gene'),
(499568, 3823, 'FOXP1', 'forkhead box P1', 'protein-coding gene'),
(511007, 7436, 'MTHFR', 'methylenetetrahydrofolate reductase', 'protein-coding gene'),
(512576, 7981, 'NR4A2', 'nuclear receptor subfamily 4 group A member 2', 'protein-coding gene'),
(526175, 17575, 'SPEN', 'spen family transcriptional repressor', 'protein-coding gene'),
(622167, 2652, 'CYP7B1', 'cytochrome P450 family 7 subfamily B member 1', 'protein-coding gene'),
(650913, 10985, 'SLC25A15', 'solute carrier family 25 member 15', 'protein-coding gene'),
(652410, 11237, 'SPG7', 'SPG7 matrix AAA peptidase subunit, paraplegin', 'protein-coding gene');

INSERT INTO `gene_alias` (`gene_id`, `symbol`, `type`) VALUES
(427667, 'COX2', 'synonym'),
(433223, 'COX2', 'synonym'),
(415153, 'QARS', 'previous'),
(433401, 'QARS', 'previous');

INSERT INTO `gene_transcript`(`id`, `gene_id`, `name`, `source`, `chromosome`, `start_coding`, `end_coding`, `strand`) VALUES
(1, 1,'BRCA1_TR1','ccds','17',100,200,'+'),
(2, 2,'BRCA2_TR1','ccds','13',100,200,'+'),
(3, 3,'NIPA1_TR1','ensembl','15',100,400,'-'),
(4, 3,'NIPA1_TR2','ensembl','15',150,350,'-'),
(5, 4,'NON-CODING_TR1','ensembl','22',NULL,NULL,'-'),
(1568912, 622167, 'ENST00000310193', 'ensembl', '8', 65509199, 65711144, '-'),
(1503635, 650913, 'ENST00000338625', 'ensembl', '13', 41367363, 41383803, '+'),
(1515928, 652410, 'ENST00000268704', 'ensembl', '16', 89574826, 89623501, '+'),
(1515930, 652410, 'ENST00000341316', 'ensembl', '16', 89574826, 89603318, '+');

INSERT INTO `gene_exon`(`transcript_id`, `start`, `end`) VALUES
(1, 100, 110),
(1, 120, 130),
(1, 160, 170),
(1, 190, 200),
(2, 100, 120),
(2, 180, 200),
(3, 100, 200),
(3, 300, 400),
(4, 80, 90),
(4, 100, 200),
(4, 300, 400),
(4, 410, 420),
(5, 100, 200),
(5, 300, 400),
(1503635, 41363633, 41363799),
(1503635, 41367294, 41367417),
(1503635, 41373193, 41373451),
(1503635, 41379254, 41379391),
(1503635, 41381430, 41381599),
(1503635, 41382574, 41382732),
(1503635, 41383679, 41384247),
(1515928, 89574811, 89575008),
(1515928, 89576898, 89577000),
(1515928, 89579356, 89579445),
(1515928, 89590414, 89590655),
(1515928, 89592737, 89592876),
(1515928, 89595885, 89595987),
(1515928, 89597091, 89597216),
(1515928, 89598312, 89598474),
(1515928, 89598871, 89599044),
(1515928, 89611056, 89611180),
(1515928, 89613066, 89613168),
(1515928, 89614411, 89614521),
(1515928, 89616902, 89617017),
(1515928, 89619387, 89619543),
(1515928, 89620202, 89620368),
(1515928, 89620894, 89620971),
(1515928, 89623295, 89624174),
(1515930, 89574819, 89575008),
(1515930, 89576898, 89577000),
(1515930, 89579356, 89579445),
(1515930, 89590414, 89590655),
(1515930, 89592737, 89592876),
(1515930, 89595885, 89595987),
(1515930, 89597091, 89597216),
(1515930, 89598312, 89598474),
(1515930, 89598871, 89599044),
(1515930, 89603173, 89604129),
(1568912, 65508692, 65509486),
(1568912, 65517239, 65517414),
(1568912, 65527583, 65527789),
(1568912, 65528248, 65528838),
(1568912, 65536960, 65537096),
(1568912, 65711023, 65711318);

INSERT INTO `geneinfo_germline`(`symbol`, `inheritance`, `gnomad_oe_syn`, `gnomad_oe_mis`, `gnomad_oe_lof`, `comments`) VALUES
('BRCA1', 'AD', 0.77, 0.88, 0.99, ''),
('TP53', 'AD', 0.93, 0.92, 0.91, ''),
('NIPA1', 'n/a', NULL, NULL, NULL, '');

INSERT INTO `somatic_gene_role` (`id`, `symbol`, `gene_role`, `high_evidence`, `comment`) VALUES
(1, 'BRCA2', 'loss_of_function', true, "test comment"),
(2, 'PTGS2', 'ambiguous',  false, "comment on gene"),
(3, 'EPRS', 'activating', true, NULL);

INSERT INTO `variant` (`id`, `chr`, `start`, `end`, `ref`, `obs`, `1000g`, `gnomAD`, `gene`, `variant_type`, `coding`) VALUES
(6, 'chr10', 43613843, 43613843, 'G', 'T', 0.7125, 0.7653, 'RET', 'synonymous', 'RET:NM_020975.4:synonymous:LOW:exon13/20:c.2307G>T:p.Leu769Leu,RET:NM_020630.4:synonymous:LOW:exon13/19:c.2307G>T:p.Leu769Leu'),
(405, 'chr17', 7579472, 7579472, 'G', 'C', 0.5429, 0.7452, 'TP53', 'missense,upstream_gene', 'TP53:NM_000546.5:missense:MODERATE:exon4/11:c.215C>G:p.Pro72Arg,TP53:NM_001126112.2:missense:MODERATE:exon4/11:c.215C>G:p.Pro72Arg,TP53:NM_001126113.2:missense:MODERATE:exon4/12:c.215C>G:p.Pro72Arg,TP53:NM_001126114.2:missense:MODERATE:exon4/12:c.215C>G:p.Pro72Arg,TP53:NM_001126118.1:missense:MODERATE:exon3/10:c.98C>G:p.Pro33Arg,TP53:NM_001276695.1:missense:MODERATE:exon4/12:c.98C>G:p.Pro33Arg,TP53:NM_001276696.1:missense:MODERATE:exon4/12:c.98C>G:p.Pro33Arg,TP53:NM_001276760.1:missense:MODERATE:exon4/11:c.98C>G:p.Pro33Arg,TP53:NM_001276761.1:missense:MODERATE:exon4/11:c.98C>G:p.Pro33Arg,TP53:NM_001126115.1:upstream_gene:MODIFIER::c.-939C>G:,TP53:NM_001126116.1:upstream_gene:MODIFIER::c.-939C>G:,TP53:NM_001126117.1:upstream_gene:MODIFIER::c.-939C>G:,TP53:NM_001276697.1:upstream_gene:MODIFIER::c.-1020C>G:,TP53:NM_001276698.1:upstream_gene:MODIFIER::c.-1020C>G:,TP53:NM_001276699.1:upstream_gene:MODIFIER::c.-1020C>G:'),
(447, 'chr5', 112175770, 112175770, 'G', 'A', 0.6655, 0.6167, 'APC', 'synonymous', 'APC:NM_000038.5:synonymous:LOW:exon16/16:c.4479G>A:p.Thr1493Thr,APC:NM_001127511.2:synonymous:LOW:exon14/14:c.4425G>A:p.Thr1475Thr,APC:NM_001127510.2:synonymous:LOW:exon17/17:c.4479G>A:p.Thr1493Thr'),
(199842, 'chr1', 62713224, 62713224, 'C', 'G', 0.9862, 0.9598, 'KANK4', 'missense', 'KANK4:NM_181712.4:missense:MODERATE:exon9/10:c.2803G>C:p.Val935Leu'),
(199843, 'chr1', 62728784, 62728784, 'A', 'G', 0.7159, 0.7338, 'KANK4', 'missense', 'KANK4:NM_181712.4:missense:MODERATE:exon7/10:c.2519T>C:p.Val840Ala'),
(199844, 'chr1', 62728838, 62728838, 'T', 'C', 0.7163, 0.734, 'KANK4', 'missense', 'KANK4:NM_181712.4:missense:MODERATE:exon7/10:c.2465A>G:p.His822Arg'),
(199845, 'chr1', 62728861, 62728861, 'T', 'C', 0.7163, 0.7338, 'KANK4', 'synonymous', 'KANK4:NM_181712.4:synonymous:LOW:exon7/10:c.2442A>G:p.Lys814Lys'),
(199846, 'chr1', 62728918, 62728918, 'G', 'A', 0.7161, 0.7338, 'KANK4', 'synonymous', 'KANK4:NM_181712.4:synonymous:LOW:exon7/10:c.2385C>T:p.Pro795Pro'),
(200567, 'chr1', 120458004, 120458004, 'A', 'T', 0.1925, 0.1038, 'NOTCH2', 'synonymous', 'NOTCH2:NM_024408.3:synonymous:LOW:exon34/34:c.7341T>A:p.Gly2447Gly'),
(200575, 'chr1', 120611964, 120611964, 'G', 'C', 0, 0, 'NOTCH2', 'missense', 'NOTCH2:NM_024408.3:missense:MODERATE:exon1/34:c.57C>G:p.Cys19Trp,NOTCH2:NM_001200001.1:missense:MODERATE:exon1/22:c.57C>G:p.Cys19Trp'),
(203397, 'chr10', 43595968, 43595968, 'A', 'G', 0.7536, 0.7394, 'RET', 'synonymous', 'RET:NM_020975.4:synonymous:LOW:exon2/20:c.135A>G:p.Ala45Ala,RET:NM_020630.4:synonymous:LOW:exon2/19:c.135A>G:p.Ala45Ala'),
(204170, 'chr10', 104264107, 104264107, 'C', 'T', 0.3962,  0.56, 'SUFU,ACTR1A', 'splice_region&intron,upstream_gene', 'SUFU:NM_016169.3:splice_region&intron:LOW:exon1/11:c.182+16C>T:,SUFU:NM_001178133.1:splice_region&intron:LOW:exon1/10:c.182+16C>T:,ACTR1A:NM_005736.3:upstream_gene:MODIFIER::c.-1703G>A:'),
(204172, 'chr10', 104387019, 104387019, 'T', 'C', 0.7196, 0.6521, 'SUFU', 'splice_region&intron', 'SUFU:NM_016169.3:splice_region&intron:LOW:exon11/11:c.1365+19T>C:'),
(204173, 'chr10', 104389932, 104389932, 'T', 'G', 0.9974, 1, 'SUFU', '3''UTR', 'SUFU:NM_016169.3:3''UTR:MODIFIER:exon12/12:c.*20T>G:'),
(206534, 'chr11', 64572018, 64572018, 'T', 'C', 0.8345, 0.9959, 'MEN1,MAP4K2', 'missense,upstream_gene', 'MEN1:NM_000244.3:missense:MODERATE:exon10/10:c.1636A>G:p.Thr546Ala,MEN1:NM_130800.2:missense:MODERATE:exon10/10:c.1636A>G:p.Thr546Ala,MEN1:NM_130801.2:missense:MODERATE:exon10/10:c.1636A>G:p.Thr546Ala,MEN1:NM_130799.2:missense:MODERATE:exon10/10:c.1621A>G:p.Thr541Ala,MEN1:NM_130802.2:missense:MODERATE:exon10/10:c.1636A>G:p.Thr546Ala,MEN1:NM_130803.2:missense:MODERATE:exon10/10:c.1636A>G:p.Thr546Ala,MEN1:NM_130804.2:missense:MODERATE:exon11/11:c.1636A>G:p.Thr546Ala,MAP4K2:NM_004579.4:upstream_gene:MODIFIER::c.-1397A>G:,MAP4K2:NM_001307990.1:upstream_gene:MODIFIER::c.-1397A>G:'),
(206535, 'chr11', 64572557, 64572557, 'A', 'G', 0.976, 0.9997, 'MEN1,MAP4K2', 'synonymous,upstream_gene', 'MEN1:NM_000244.3:synonymous:LOW:exon9/10:c.1314T>C:p.His438His,MEN1:NM_130800.2:synonymous:LOW:exon9/10:c.1314T>C:p.His438His,MEN1:NM_130801.2:synonymous:LOW:exon9/10:c.1314T>C:p.His438His,MEN1:NM_130799.2:synonymous:LOW:exon9/10:c.1299T>C:p.His433His,MEN1:NM_130802.2:synonymous:LOW:exon9/10:c.1314T>C:p.His438His,MEN1:NM_130803.2:synonymous:LOW:exon9/10:c.1314T>C:p.His438His,MEN1:NM_130804.2:synonymous:LOW:exon10/11:c.1314T>C:p.His438His,MAP4K2:NM_004579.4:upstream_gene:MODIFIER::c.-1936T>C:,MAP4K2:NM_001307990.1:upstream_gene:MODIFIER::c.-1936T>C:'),
(207343, 'chr11', 108175462, 108175462, 'G', 'A', 0.0669, 0.1402, 'ATM', 'missense', 'ATM:NM_000051.3:missense:MODERATE:exon37/63:c.5557G>A:p.Asp1853Asn'),
(207345, 'chr11', 108183167, 108183167, 'A', 'G', 1, 0, 'ATM', 'missense', 'ATM:NM_000051.3:missense:MODERATE:exon40/63:c.5948A>G:p.Asn1983Ser'),
(207750, 'chr11', 125525195, 125525195, 'A', 'G', 0.9858, 0.9637, 'CHEK1', 'missense,non_coding_exon', 'CHEK1:NM_001114121.2:missense:MODERATE:exon13/14:c.1411A>G:p.Ile471Val,CHEK1:NM_001114122.2:missense:MODERATE:exon13/13:c.1411A>G:p.Ile471Val,CHEK1:NM_001244846.1:missense:MODERATE:exon12/12:c.1309A>G:p.Ile437Val,CHEK1:NM_001274.5:missense:MODERATE:exon13/13:c.1411A>G:p.Ile471Val,CHEK1:NR_045204.1:non_coding_exon:MODIFIER:exon12/12:n.2084A>G:,CHEK1:NR_045205.1:non_coding_exon:MODIFIER:exon13/13:n.1845A>G:'),
(210578, 'chr13', 32911888, 32911888, 'A', 'G', 0.2668,  0.3074, 'BRCA2', 'synonymous', 'BRCA2:NM_000059.3:synonymous:LOW:exon11/27:c.3396A>G:p.Lys1132Lys'),
(210580, 'chr13', 32913055, 32913055, 'A', 'G', 0.974, 0.9994, 'BRCA2', 'synonymous', 'BRCA2:NM_000059.3:synonymous:LOW:exon11/27:c.4563A>G:p.Leu1521Leu'),
(210582, 'chr13', 32915005, 32915005, 'G', 'C', 0.9736, 0.9994, 'BRCA2', 'synonymous', 'BRCA2:NM_000059.3:synonymous:LOW:exon11/27:c.6513G>C:p.Val2171Val'),
(210584, 'chr13', 32929232, 32929232, 'A', 'G', 0.2326, 0.2133, 'BRCA2', 'synonymous', 'BRCA2:NM_000059.3:synonymous:LOW:exon14/27:c.7242A>G:p.Ser2414Ser'),
(210585, 'chr13', 32929387, 32929387, 'T', 'C', 0.9758, 0.9994, 'BRCA2', 'missense', 'BRCA2:NM_000059.3:missense:MODERATE:exon14/27:c.7397T>C:p.Val2466Ala'),
(213346, 'chr15', 43707808, 43707808, 'A', 'T', 0.1286, 0.19, 'TP53BP1', 'synonymous', 'TP53BP1:NM_001141980.1:synonymous:LOW:exon23/28:c.5073T>A:p.Ser1691Ser,TP53BP1:NM_001141979.1:synonymous:LOW:exon23/28:c.5073T>A:p.Ser1691Ser,TP53BP1:NM_005657.2:synonymous:LOW:exon23/28:c.5058T>A:p.Ser1686Ser'),
(213347, 'chr15', 43724646, 43724646, 'T', 'G', 0.5264, 0.3036, 'TP53BP1', 'missense', 'TP53BP1:NM_001141980.1:missense:MODERATE:exon17/28:c.3421A>C:p.Lys1141Gln,TP53BP1:NM_001141979.1:missense:MODERATE:exon17/28:c.3421A>C:p.Lys1141Gln,TP53BP1:NM_005657.2:missense:MODERATE:exon17/28:c.3406A>C:p.Lys1136Gln'),
(213349, 'chr15', 43748304, 43748304, 'A', 'G', 0.5262, 0.3032, 'TP53BP1', 'synonymous', 'TP53BP1:NM_001141980.1:synonymous:LOW:exon12/28:c.2502T>C:p.Asp834Asp,TP53BP1:NM_001141979.1:synonymous:LOW:exon12/28:c.2502T>C:p.Asp834Asp,TP53BP1:NM_005657.2:synonymous:LOW:exon12/28:c.2487T>C:p.Asp829Asp'),
(213350, 'chr15', 43767774, 43767774, 'G', 'C', 0.5258, 0.3029, 'TP53BP1', 'missense', 'TP53BP1:NM_001141980.1:missense:MODERATE:exon9/28:c.1074C>G:p.Asp358Glu,TP53BP1:NM_001141979.1:missense:MODERATE:exon9/28:c.1074C>G:p.Asp358Glu,TP53BP1:NM_005657.2:missense:MODERATE:exon9/28:c.1059C>G:p.Asp353Glu'),
(214723, 'chr16', 3639139, 3639139, 'A', 'G', 0.7396, 0.5077, 'SLX4', 'synonymous', 'SLX4:NM_032444.2:synonymous:LOW:exon12/15:c.4500T>C:p.Asn1500Asn'),
(215568, 'chr16', 68857441, 68857441, 'T', 'C', 0.7187, 0.6322, 'CDH1', 'synonymous', 'CDH1:NM_004360.4:synonymous:LOW:exon13/16:c.2076T>C:p.Ala692Ala,CDH1:NM_001317184.1:synonymous:LOW:exon12/15:c.1893T>C:p.Ala631Ala,CDH1:NM_001317185.1:synonymous:LOW:exon13/16:c.528T>C:p.Ala176Ala,CDH1:NM_001317186.1:synonymous:LOW:exon12/15:c.111T>C:p.Ala37Ala'),
(216149, 'chr16', 89838078, 89838078, 'A', 'G', 0.5599, 0.3077, 'FANCA', 'splice_region&intron', 'FANCA:NM_000135.2:splice_region&intron:LOW:exon23/42:c.2151+8T>C:,FANCA:NM_001286167.1:splice_region&intron:LOW:exon23/42:c.2151+8T>C:'),
(216155, 'chr16', 89866043, 89866043, 'T', 'C', 0.6905, 0.3937, 'FANCA', 'missense', 'FANCA:NM_000135.2:missense:MODERATE:exon9/43:c.796A>G:p.Thr266Ala,FANCA:NM_001286167.1:missense:MODERATE:exon9/43:c.796A>G:p.Thr266Ala,FANCA:NM_001018112.1:missense:MODERATE:exon9/11:c.796A>G:p.Thr266Ala'),
(217254, 'chr17', 29553485, 29553485, 'G', 'A', 0.4968, 0.2891, 'NF1', 'synonymous,downstream_gene', 'NF1:NM_001042492.2:synonymous:LOW:exon18/58:c.2034G>A:p.Pro678Pro,NF1:NM_000267.3:synonymous:LOW:exon18/57:c.2034G>A:p.Pro678Pro,NF1:NM_001128147.2:downstream_gene:MODIFIER::c.*4477G>A:'),
(218033, 'chr17', 59760996, 59760996, 'A', 'G', 0.6208, 0.5869, 'BRIP1', 'synonymous', 'BRIP1:NM_032043.2:synonymous:LOW:exon20/20:c.3411T>C:p.Tyr1137Tyr'),
(218034, 'chr17', 59763347, 59763347, 'A', 'G', 0.6278, 0.589, 'BRIP1', 'missense', 'BRIP1:NM_032043.2:missense:MODERATE:exon19/20:c.2755T>C:p.Ser919Pro'),
(218035, 'chr17', 59763465, 59763465, 'T', 'C', 0.8151, 0.6555, 'BRIP1', 'synonymous', 'BRIP1:NM_032043.2:synonymous:LOW:exon19/20:c.2637A>G:p.Glu879Glu'),
(222995, 'chr2', 17942775, 17942775, 'T', 'A', 0.8137, 0.6259, 'GEN1', 'missense', 'GEN1:NM_001130009.1:missense:MODERATE:exon3/14:c.274T>A:p.Ser92Thr,GEN1:NM_182625.3:missense:MODERATE:exon3/14:c.274T>A:p.Ser92Thr'),
(222996, 'chr2', 17954027, 17954027, 'G', 'A', 0.977, 0.9998, 'GEN1', 'missense', 'GEN1:NM_001130009.1:missense:MODERATE:exon8/14:c.929G>A:p.Ser310Asn,GEN1:NM_182625.3:missense:MODERATE:exon8/14:c.929G>A:p.Ser310Asn'),
(222997, 'chr2', 17962450, 17962450, 'A', 'G', 0.5739, 0.5184, 'GEN1', 'synonymous', 'GEN1:NM_001130009.1:synonymous:LOW:exon14/14:c.1971A>G:p.Glu657Glu,GEN1:NM_182625.3:synonymous:LOW:exon14/14:c.1971A>G:p.Glu657Glu'),
(225333, 'chr2', 215632255, 215632255, 'C', 'T', 0.3662, 0.2758, 'BARD1', 'missense,intron,non_coding_exon', 'BARD1:NM_000465.3:missense:MODERATE:exon6/11:c.1519G>A:p.Val507Met,BARD1:NM_001282543.1:missense:MODERATE:exon5/10:c.1462G>A:p.Val488Met,BARD1:NM_001282545.1:intron:MODIFIER:exon2/6:c.216-14976G>A:,BARD1:NM_001282548.1:intron:MODIFIER:exon1/5:c.159-14976G>A:,BARD1:NM_001282549.1:intron:MODIFIER:exon3/4:c.364+24766G>A:,BARD1:NR_104212.1:non_coding_exon:MODIFIER:exon5/10:n.1512G>A:,BARD1:NR_104215.1:non_coding_exon:MODIFIER:exon4/9:n.1455G>A:,BARD1:NR_104216.1:non_coding_exon:MODIFIER:exon5/10:n.711G>A:'),
(225334, 'chr2', 215632256, 215632256, 'A', 'G', 0.7706, 0.7806, 'BARD1', 'synonymous,intron,non_coding_exon', 'BARD1:NM_000465.3:synonymous:LOW:exon6/11:c.1518T>C:p.His506His,BARD1:NM_001282543.1:synonymous:LOW:exon5/10:c.1461T>C:p.His487His,BARD1:NM_001282545.1:intron:MODIFIER:exon2/6:c.216-14977T>C:,BARD1:NM_001282548.1:intron:MODIFIER:exon1/5:c.159-14977T>C:,BARD1:NM_001282549.1:intron:MODIFIER:exon3/4:c.364+24765T>C:,BARD1:NR_104212.1:non_coding_exon:MODIFIER:exon5/10:n.1511T>C:,BARD1:NR_104215.1:non_coding_exon:MODIFIER:exon4/9:n.1454T>C:,BARD1:NR_104216.1:non_coding_exon:MODIFIER:exon5/10:n.710T>C:'),
(225336, 'chr2', 215645464, 215645464, 'C', 'G', 0.4593, 0.6008, 'BARD1', 'missense,intron,non_coding_exon', 'BARD1:NM_000465.3:missense:MODERATE:exon4/11:c.1134G>C:p.Arg378Ser,BARD1:NM_001282543.1:missense:MODERATE:exon3/10:c.1077G>C:p.Arg359Ser,BARD1:NM_001282545.1:intron:MODIFIER:exon2/6:c.215+16321G>C:,BARD1:NM_001282548.1:intron:MODIFIER:exon1/5:c.159-28185G>C:,BARD1:NM_001282549.1:intron:MODIFIER:exon3/4:c.364+11557G>C:,BARD1:NR_104216.1:intron:MODIFIER:exon3/9:n.507-11428G>C:,BARD1:NR_104212.1:non_coding_exon:MODIFIER:exon3/10:n.1127G>C:,BARD1:NR_104215.1:non_coding_exon:MODIFIER:exon2/9:n.1070G>C:'),
(225337, 'chr2', 215674224, 215674224, 'G', 'A', 0.3313, 0.3474, 'BARD1,LOC101928103', 'missense,upstream_gene,non_coding_exon', 'BARD1:NM_000465.3:missense:MODERATE:exon1/11:c.70C>T:p.Pro24Ser,BARD1:NM_001282543.1:missense:MODERATE:exon1/10:c.70C>T:p.Pro24Ser,BARD1:NM_001282545.1:missense:MODERATE:exon1/7:c.70C>T:p.Pro24Ser,BARD1:NM_001282548.1:missense:MODERATE:exon1/6:c.70C>T:p.Pro24Ser,BARD1:NM_001282549.1:missense:MODERATE:exon1/5:c.70C>T:p.Pro24Ser,LOC101928103:NR_110292.1:upstream_gene:MODIFIER::n.-729G>A:,BARD1:NR_104212.1:non_coding_exon:MODIFIER:exon1/10:n.212C>T:,BARD1:NR_104215.1:non_coding_exon:MODIFIER:exon1/9:n.212C>T:,BARD1:NR_104216.1:non_coding_exon:MODIFIER:exon1/10:n.212C>T:'),
(225526, 'chr2', 220416942, 220416942, 'G', 'C', 0.3876, 0.4551, 'OBSL1,MIR3132,TMEM198', 'splice_region&intron,upstream_gene,downstream_gene', 'OBSL1:NM_015311.2:splice_region&intron:LOW:exon18/20:c.5309-4C>G:,MIR3132:NR_036082.1:upstream_gene:MODIFIER::n.-3073C>G:,TMEM198:NM_001005209.2:downstream_gene:MODIFIER::c.*2366G>C:,TMEM198:NM_001303098.1:downstream_gene:MODIFIER::c.*2366G>C:,OBSL1:NM_001173431.1:downstream_gene:MODIFIER::c.*3777C>G:'),
(225528, 'chr2', 220417266, 220417266, 'C', 'T', 0.3766, 0.4456, 'OBSL1,MIR3132,TMEM198', 'missense,upstream_gene,downstream_gene', 'OBSL1:NM_015311.2:missense:MODERATE:exon18/21:c.5300G>A:p.Arg1767Gln,MIR3132:NR_036082.1:upstream_gene:MODIFIER::n.-3397G>A:,TMEM198:NM_001005209.2:downstream_gene:MODIFIER::c.*2690C>T:,TMEM198:NM_001303098.1:downstream_gene:MODIFIER::c.*2690C>T:,OBSL1:NM_001173431.1:downstream_gene:MODIFIER::c.*3453G>A:'),
(225529, 'chr2', 220419236, 220419236, 'T', 'C', 0.9852, 0.9996, 'OBSL1,TMEM198', 'synonymous,downstream_gene', 'OBSL1:NM_015311.2:synonymous:LOW:exon15/21:c.4836A>G:p.Thr1612Thr,TMEM198:NM_001005209.2:downstream_gene:MODIFIER::c.*4660T>C:,TMEM198:NM_001303098.1:downstream_gene:MODIFIER::c.*4660T>C:,OBSL1:NM_001173431.1:downstream_gene:MODIFIER::c.*1483A>G:'),
(225530, 'chr2', 220419339, 220419339, 'T', 'C', 0.9611, 0.9925, 'OBSL1,TMEM198', 'missense,downstream_gene', 'OBSL1:NM_015311.2:missense:MODERATE:exon15/21:c.4733A>G:p.Gln1578Arg,TMEM198:NM_001005209.2:downstream_gene:MODIFIER::c.*4763T>C:,TMEM198:NM_001303098.1:downstream_gene:MODIFIER::c.*4763T>C:,OBSL1:NM_001173431.1:downstream_gene:MODIFIER::c.*1380A>G:'),
(225532, 'chr2', 220420956, 220420956, 'A', 'G', 0.9637, 0.9996, 'OBSL1', 'synonymous', 'OBSL1:NM_015311.2:synonymous:LOW:exon14/21:c.4395T>C:p.Asp1465Asp,OBSL1:NM_001173431.1:synonymous:LOW:exon14/14:c.4395T>C:p.Asp1465Asp'),
(225533, 'chr2', 220421417, 220421417, 'C', 'G', 0.7246, 0.7336, 'OBSL1', 'missense', 'OBSL1:NM_015311.2:missense:MODERATE:exon13/21:c.4095G>C:p.Glu1365Asp,OBSL1:NM_001173431.1:missense:MODERATE:exon13/14:c.4095G>C:p.Glu1365Asp'),
(225536, 'chr2', 220422774, 220422774, 'A', 'G', 0.4191, 0.4331, 'OBSL1', 'synonymous,downstream_gene', 'OBSL1:NM_015311.2:synonymous:LOW:exon11/21:c.3561T>C:p.Pro1187Pro,OBSL1:NM_001173431.1:synonymous:LOW:exon11/14:c.3561T>C:p.Pro1187Pro,OBSL1:NM_001173408.1:downstream_gene:MODIFIER::c.*3832T>C:'),
(225541, 'chr2', 220430203, 220430203, 'C', 'T', 0.7927, 0.9043, 'OBSL1', 'missense', 'OBSL1:NM_015311.2:missense:MODERATE:exon6/21:c.2168G>A:p.Arg723Lys,OBSL1:NM_001173431.1:missense:MODERATE:exon6/14:c.2168G>A:p.Arg723Lys,OBSL1:NM_001173408.1:missense:MODERATE:exon6/9:c.2168G>A:p.Arg723Lys'),
(225543, 'chr2', 220435034, 220435034, 'A', 'G', 0.7833, 0.9044, 'OBSL1,INHA', 'synonymous,upstream_gene', 'OBSL1:NM_015311.2:synonymous:LOW:exon1/21:c.921T>C:p.Leu307Leu,OBSL1:NM_001173431.1:synonymous:LOW:exon1/14:c.921T>C:p.Leu307Leu,OBSL1:NM_001173408.1:synonymous:LOW:exon1/9:c.921T>C:p.Leu307Leu,INHA:NM_002191.3:upstream_gene:MODIFIER::c.-2063A>G:'),
(229029, 'chr3', 10089723, 10089723, 'G', 'A', 0, 0, 'FANCD2', 'synonymous', 'FANCD2:NM_033084.3:synonymous:LOW:exon16/43:c.1401G>A:p.Thr467Thr,FANCD2:NM_001018115.1:synonymous:LOW:exon16/44:c.1401G>A:p.Thr467Thr'),
(230800, 'chr3', 142168331, 142168331, 'C', 'T', 0.9163, 0.8614, 'ATR,XRN1', 'synonymous,upstream_gene', 'ATR:NM_001184.3:synonymous:LOW:exon47/47:c.7875G>A:p.Gln2625Gln,XRN1:NM_019001.4:upstream_gene:MODIFIER::c.-1545G>A:,XRN1:NM_001282857.1:upstream_gene:MODIFIER::c.-1545G>A:,XRN1:NM_001282859.1:upstream_gene:MODIFIER::c.-1545G>A:'),
(230801, 'chr3', 142178144, 142178144, 'C', 'T', 0.1022, 0.1578, 'ATR', 'missense', 'ATR:NM_001184.3:missense:MODERATE:exon43/47:c.7274G>A:p.Arg2425Gln'),
(230806, 'chr3', 142277575, 142277575, 'A', 'T', 0.598, 0.595, 'ATR', 'synonymous', 'ATR:NM_001184.3:synonymous:LOW:exon8/47:c.1776T>A:p.Gly592Gly'),
(230807, 'chr3', 142281612, 142281612, 'A', 'G', 0.5974, 0.5955, 'ATR', 'missense', 'ATR:NM_001184.3:missense:MODERATE:exon4/47:c.632T>C:p.Met211Thr'),
(234144, 'chr5', 80149981, 80149981, 'A', 'G', 0.9022, 0.8355, 'MSH3', 'missense', 'MSH3:NM_002439.4:missense:MODERATE:exon21/24:c.2846A>G:p.Gln949Arg'),
(234361, 'chr5', 112162854, 112162854, 'T', 'C', 0.51, 0.5902, 'APC', 'synonymous', 'APC:NM_000038.5:synonymous:LOW:exon12/16:c.1458T>C:p.Tyr486Tyr,APC:NM_001127511.2:synonymous:LOW:exon10/14:c.1404T>C:p.Tyr468Tyr,APC:NM_001127510.2:synonymous:LOW:exon13/17:c.1458T>C:p.Tyr486Tyr'),
(234362, 'chr5', 112164561, 112164561, 'G', 'A', 0.6661, 0.6169, 'APC', 'synonymous', 'APC:NM_000038.5:synonymous:LOW:exon14/16:c.1635G>A:p.Ala545Ala,APC:NM_001127511.2:synonymous:LOW:exon12/14:c.1581G>A:p.Ala527Ala,APC:NM_001127510.2:synonymous:LOW:exon15/17:c.1635G>A:p.Ala545Ala'),
(234365, 'chr5', 112176325, 112176325, 'G', 'A', 0.6667, 0.621, 'APC', 'synonymous', 'APC:NM_000038.5:synonymous:LOW:exon16/16:c.5034G>A:p.Gly1678Gly,APC:NM_001127511.2:synonymous:LOW:exon14/14:c.4980G>A:p.Gly1660Gly,APC:NM_001127510.2:synonymous:LOW:exon17/17:c.5034G>A:p.Gly1678Gly'),
(234366, 'chr5', 112176559, 112176559, 'T', 'G', 0.6669, 0.6186, 'APC', 'synonymous', 'APC:NM_000038.5:synonymous:LOW:exon16/16:c.5268T>G:p.Ser1756Ser,APC:NM_001127511.2:synonymous:LOW:exon14/14:c.5214T>G:p.Ser1738Ser,APC:NM_001127510.2:synonymous:LOW:exon17/17:c.5268T>G:p.Ser1756Ser'),
(234367, 'chr5', 112176756, 112176756, 'T', 'A', 0.8654, 0.765, 'APC', 'missense', 'APC:NM_000038.5:missense:MODERATE:exon16/16:c.5465T>A:p.Val1822Asp,APC:NM_001127511.2:missense:MODERATE:exon14/14:c.5411T>A:p.Val1804Asp,APC:NM_001127510.2:missense:MODERATE:exon17/17:c.5465T>A:p.Val1822Asp'),
(234368, 'chr5', 112177171, 112177171, 'G', 'A', 0.6665, 0.6168, 'APC', 'synonymous', 'APC:NM_000038.5:synonymous:LOW:exon16/16:c.5880G>A:p.Pro1960Pro,APC:NM_001127511.2:synonymous:LOW:exon14/14:c.5826G>A:p.Pro1942Pro,APC:NM_001127510.2:synonymous:LOW:exon17/17:c.5880G>A:p.Pro1960Pro'),
(235808, 'chr6', 35423662, 35423662, 'A', 'C', 0.778, 0.6717, 'FANCE', 'synonymous', 'FANCE:NM_021922.2:synonymous:LOW:exon2/10:c.387A>C:p.Pro129Pro'),
(237622, 'chr7', 6026775, 6026775, 'T', 'C', 0.8832, 0.8457, 'PMS2', 'missense,non_coding_exon', 'PMS2:NM_000535.5:missense:MODERATE:exon11/15:c.1621A>G:p.Lys541Glu,PMS2:NR_003085.2:non_coding_exon:MODIFIER:exon11/15:n.1703A>G:'),
(237626, 'chr7', 6036980, 6036980, 'G', 'C', 0.8313, 0.8148, 'PMS2', 'synonymous,non_coding_exon', 'PMS2:NM_000535.5:synonymous:LOW:exon7/15:c.780C>G:p.Ser260Ser,PMS2:NR_003085.2:non_coding_exon:MODIFIER:exon7/15:n.862C>G:'),
(239025, 'chr7', 128846328, 128846328, 'G', 'C', 0.7562, 0.8391, 'SMO', 'synonymous', 'SMO:NM_005631.4:synonymous:LOW:exon6/12:c.1164G>C:p.Gly388Gly'),
(239327, 'chr7', 142460313, 142460313, 'T', 'C', 0.3966,  0, 'PRSS1', 'synonymous', 'PRSS1:NM_002769.4:synonymous:LOW:exon4/5:c.486T>C:p.Asp162Asp'),
(239334, 'chr7', 142460865, 142460865, 'T', 'C', 0.3938, 0, 'PRSS1', 'synonymous', 'PRSS1:NM_002769.4:synonymous:LOW:exon5/5:c.738T>C:p.Asn246Asn'),
(240595, 'chr8', 90958422, 90958422, 'T', 'C', 0.3528, 0.3132, 'NBN', 'synonymous', 'NBN:NM_002485.4:synonymous:LOW:exon13/16:c.2016A>G:p.Pro672Pro,NBN:NM_001024688.2:synonymous:LOW:exon14/17:c.1770A>G:p.Pro590Pro'),
(240596, 'chr8', 90958530, 90958530, 'T', 'C', 0.379, 0.3144, 'NBN', 'splice_region&intron', 'NBN:NM_002485.4:splice_region&intron:LOW:exon12/15:c.1915-7A>G:,NBN:NM_001024688.2:splice_region&intron:LOW:exon13/16:c.1669-7A>G:'),
(240598, 'chr8', 90967711, 90967711, 'A', 'G', 0.6086, 0.3559, 'NBN', 'synonymous', 'NBN:NM_002485.4:synonymous:LOW:exon10/16:c.1197T>C:p.Asp399Asp,NBN:NM_001024688.2:synonymous:LOW:exon11/17:c.951T>C:p.Asp317Asp'),
(240600, 'chr8', 90990479, 90990479, 'C', 'G', 0.357, 0.3136, 'NBN', 'missense', 'NBN:NM_002485.4:missense:MODERATE:exon5/16:c.553G>C:p.Glu185Gln,NBN:NM_001024688.2:missense:MODERATE:exon6/17:c.307G>C:p.Glu103Gln'),
(240602, 'chr8', 90995019, 90995019, 'C', 'T', 0.3792, 0.3147, 'NBN', 'synonymous,5''UTR', 'NBN:NM_002485.4:synonymous:LOW:exon2/16:c.102G>A:p.Leu34Leu,NBN:NM_001024688.2:5''UTR:MODIFIER:exon2/17:c.-195G>A:'),
(241479, 'chr9', 17342383, 17342383, 'C', 'T', 0.9495, 0.9994, 'CNTLN', 'synonymous', 'CNTLN:NM_017738.3:synonymous:LOW:exon12/26:c.1827C>T:p.Asp609Asp'),
(241663, 'chr9', 35074917, 35074917, 'T', 'C', 0.2202, 0.4993, 'FANCG,VCP', 'splice_region&intron,upstream_gene', 'FANCG:NM_004629.1:splice_region&intron:LOW:exon12/13:c.1636+7A>G:,VCP:NM_007126.3:upstream_gene:MODIFIER::c.-2567A>G:'),
(307040, 'chr1', 45797505, 45797505, 'C', 'G', 0.3135, 0.2492, 'MUTYH,HPDL', 'missense,downstream_gene', 'MUTYH:NM_001128425.1:missense:MODERATE:exon12/16:c.1014G>C:p.Gln338His,MUTYH:NM_001048174.1:missense:MODERATE:exon12/16:c.930G>C:p.Gln310His,MUTYH:NM_001293191.1:missense:MODERATE:exon12/16:c.963G>C:p.Gln321His,MUTYH:NM_001293195.1:missense:MODERATE:exon13/17:c.930G>C:p.Gln310His,MUTYH:NM_001048172.1:missense:MODERATE:exon12/16:c.933G>C:p.Gln311His,MUTYH:NM_001048173.1:missense:MODERATE:exon12/16:c.930G>C:p.Gln310His,MUTYH:NM_001293196.1:missense:MODERATE:exon12/16:c.654G>C:p.Gln218His,MUTYH:NM_001048171.1:missense:MODERATE:exon12/16:c.972G>C:p.Gln324His,MUTYH:NM_001293190.1:missense:MODERATE:exon12/16:c.975G>C:p.Gln325His,MUTYH:NM_001293192.1:missense:MODERATE:exon12/16:c.654G>C:p.Gln218His,MUTYH:NM_012222.2:missense:MODERATE:exon12/16:c.1005G>C:p.Gln335His,HPDL:NM_032756.2:downstream_gene:MODIFIER::c.*3569C>G:'),
(307049, 'chr10', 88635779, 88635779, 'C', 'A', 0.4998, 0.2562, 'BMPR1A', 'missense', 'BMPR1A:NM_004329.2:missense:MODERATE:exon3/13:c.4C>A:p.Pro2Thr'),
(307137, 'chr15', 43762196, 43762196, 'C', 'T', 0.3824, 0.113, 'TP53BP1', 'missense', 'TP53BP1:NM_001141980.1:missense:MODERATE:exon11/28:c.1249G>A:p.Gly417Ser,TP53BP1:NM_001141979.1:missense:MODERATE:exon11/28:c.1249G>A:p.Gly417Ser,TP53BP1:NM_005657.2:missense:MODERATE:exon11/28:c.1234G>A:p.Gly412Ser'),
(307158, 'chr16', 68771372, 68771372, 'C', 'T', 0.7634, 0.9074, 'CDH1', 'splice_region&intron', 'CDH1:NM_004360.4:splice_region&intron:LOW:exon1/15:c.48+6C>T:,CDH1:NM_001317184.1:splice_region&intron:LOW:exon1/14:c.48+6C>T:,CDH1:NM_001317185.1:splice_region&intron:LOW:exon1/15:c.-1568+6C>T:,CDH1:NM_001317186.1:splice_region&intron:LOW:exon1/14:c.-1772+6C>T:'),
(307173, 'chr16', 89836323, 89836323, 'C', 'T', 0.6667, 0.3142, 'FANCA', 'missense', 'FANCA:NM_000135.2:missense:MODERATE:exon26/43:c.2426G>A:p.Gly809Asp,FANCA:NM_001286167.1:missense:MODERATE:exon26/43:c.2426G>A:p.Gly809Asp'),
(307180, 'chr16', 89849480, 89849480, 'C', 'T', 0.624, 0.381, 'FANCA', 'missense', 'FANCA:NM_000135.2:missense:MODERATE:exon16/43:c.1501G>A:p.Gly501Ser,FANCA:NM_001286167.1:missense:MODERATE:exon16/43:c.1501G>A:p.Gly501Ser'),
(307218, 'chr17', 41223094, 41223094, 'T', 'C', 0.3558, 0.3266, 'BRCA1', 'missense,non_coding_exon', 'BRCA1:NM_007300.3:missense:MODERATE:exon16/24:c.4900A>G:p.Ser1634Gly,BRCA1:NM_007298.3:missense:MODERATE:exon14/22:c.1525A>G:p.Ser509Gly,BRCA1:NM_007297.3:missense:MODERATE:exon14/22:c.4696A>G:p.Ser1566Gly,BRCA1:NM_007299.3:missense:MODERATE:exon15/22:c.1525A>G:p.Ser509Gly,BRCA1:NM_007294.3:missense:MODERATE:exon15/23:c.4837A>G:p.Ser1613Gly,BRCA1:NR_027676.1:non_coding_exon:MODIFIER:exon15/23:n.4973A>G:'),
(307223, 'chr17', 41234470, 41234470, 'A', 'G', 0.3363, 0.326, 'BRCA1', 'synonymous,non_coding_exon', 'BRCA1:NM_007300.3:synonymous:LOW:exon12/24:c.4308T>C:p.Ser1436Ser,BRCA1:NM_007298.3:synonymous:LOW:exon11/22:c.999T>C:p.Ser333Ser,BRCA1:NM_007297.3:synonymous:LOW:exon11/22:c.4167T>C:p.Ser1389Ser,BRCA1:NM_007299.3:synonymous:LOW:exon12/22:c.999T>C:p.Ser333Ser,BRCA1:NM_007294.3:synonymous:LOW:exon12/23:c.4308T>C:p.Ser1436Ser,BRCA1:NR_027676.1:non_coding_exon:MODIFIER:exon12/23:n.4444T>C:'),
(307225, 'chr17', 41244000, 41244000, 'T', 'C', 0.3526, 0.3244, 'BRCA1', 'missense,intron,non_coding_exon', 'BRCA1:NM_007300.3:missense:MODERATE:exon10/24:c.3548A>G:p.Lys1183Arg,BRCA1:NM_007297.3:missense:MODERATE:exon9/22:c.3407A>G:p.Lys1136Arg,BRCA1:NM_007294.3:missense:MODERATE:exon10/23:c.3548A>G:p.Lys1183Arg,BRCA1:NM_007298.3:intron:MODIFIER:exon9/21:c.788-951A>G:,BRCA1:NM_007299.3:intron:MODIFIER:exon10/21:c.788-951A>G:,BRCA1:NR_027676.1:non_coding_exon:MODIFIER:exon10/23:n.3684A>G:'),
(307226, 'chr17', 41244435, 41244435, 'T', 'C', 0.3357, 0.3255, 'BRCA1', 'missense,intron,non_coding_exon', 'BRCA1:NM_007300.3:missense:MODERATE:exon10/24:c.3113A>G:p.Glu1038Gly,BRCA1:NM_007297.3:missense:MODERATE:exon9/22:c.2972A>G:p.Glu991Gly,BRCA1:NM_007294.3:missense:MODERATE:exon10/23:c.3113A>G:p.Glu1038Gly,BRCA1:NM_007298.3:intron:MODIFIER:exon9/21:c.788-1386A>G:,BRCA1:NM_007299.3:intron:MODIFIER:exon10/21:c.788-1386A>G:,BRCA1:NR_027676.1:non_coding_exon:MODIFIER:exon10/23:n.3249A>G:'),
(307227, 'chr17', 41244936, 41244936, 'G', 'A', 0.5439, 0.3359, 'BRCA1', 'missense,intron,non_coding_exon', 'BRCA1:NM_007300.3:missense:MODERATE:exon10/24:c.2612C>T:p.Pro871Leu,BRCA1:NM_007297.3:missense:MODERATE:exon9/22:c.2471C>T:p.Pro824Leu,BRCA1:NM_007294.3:missense:MODERATE:exon10/23:c.2612C>T:p.Pro871Leu,BRCA1:NM_007298.3:intron:MODIFIER:exon9/21:c.787+1825C>T:,BRCA1:NM_007299.3:intron:MODIFIER:exon10/21:c.787+1825C>T:,BRCA1:NR_027676.1:non_coding_exon:MODIFIER:exon10/23:n.2748C>T:'),
(307228, 'chr17', 41245237, 41245237, 'A', 'G', 0.3353, 0.3237, 'BRCA1', 'synonymous,intron,non_coding_exon', 'BRCA1:NM_007300.3:synonymous:LOW:exon10/24:c.2311T>C:p.Leu771Leu,BRCA1:NM_007297.3:synonymous:LOW:exon9/22:c.2170T>C:p.Leu724Leu,BRCA1:NM_007294.3:synonymous:LOW:exon10/23:c.2311T>C:p.Leu771Leu,BRCA1:NM_007298.3:intron:MODIFIER:exon9/21:c.787+1524T>C:,BRCA1:NM_007299.3:intron:MODIFIER:exon10/21:c.787+1524T>C:,BRCA1:NR_027676.1:non_coding_exon:MODIFIER:exon10/23:n.2447T>C:'),
(307229, 'chr17', 41245466, 41245466, 'G', 'A', 0.3365, 0.3244, 'BRCA1', 'synonymous,intron,non_coding_exon', 'BRCA1:NM_007300.3:synonymous:LOW:exon10/24:c.2082C>T:p.Ser694Ser,BRCA1:NM_007297.3:synonymous:LOW:exon9/22:c.1941C>T:p.Ser647Ser,BRCA1:NM_007294.3:synonymous:LOW:exon10/23:c.2082C>T:p.Ser694Ser,BRCA1:NM_007298.3:intron:MODIFIER:exon9/21:c.787+1295C>T:,BRCA1:NM_007299.3:intron:MODIFIER:exon10/21:c.787+1295C>T:,BRCA1:NR_027676.1:non_coding_exon:MODIFIER:exon10/23:n.2218C>T:'),
(307268, 'chr2', 47601106, 47601106, 'T', 'C', 0.6661, 0.4328, 'EPCAM,MIR559', 'missense,upstream_gene', 'EPCAM:NM_002354.2:missense:MODERATE:exon3/9:c.344T>C:p.Met115Thr,MIR559:NR_030286.1:upstream_gene:MODIFIER::n.-3708T>C:'),
(307285, 'chr2', 48010488, 48010488, 'G', 'A', 0.2009, 0.1724, 'MSH6', 'missense,5''UTR,upstream_gene', 'MSH6:NM_000179.2:missense:MODERATE:exon1/10:c.116G>A:p.Gly39Glu,MSH6:NM_001281492.1:missense:MODERATE:exon1/8:c.116G>A:p.Gly39Glu,MSH6:NM_001281493.1:5''UTR:MODIFIER:exon1/9:c.-621G>A:,MSH6:NM_001281494.1:upstream_gene:MODIFIER::c.-15541G>A:'),
(307299, 'chr2', 215595164, 215595164, 'G', 'A', 0.0054, 0.0078, 'BARD1', 'missense,non_coding_exon', 'BARD1:NM_000465.3:missense:MODERATE:exon10/11:c.1972C>T:p.Arg658Cys,BARD1:NM_001282543.1:missense:MODERATE:exon9/10:c.1915C>T:p.Arg639Cys,BARD1:NM_001282545.1:missense:MODERATE:exon6/7:c.619C>T:p.Arg207Cys,BARD1:NM_001282548.1:missense:MODERATE:exon5/6:c.562C>T:p.Arg188Cys,BARD1:NM_001282549.1:missense:MODERATE:exon4/5:c.433C>T:p.Arg145Cys,BARD1:NR_104212.1:non_coding_exon:MODIFIER:exon9/10:n.1965C>T:,BARD1:NR_104215.1:non_coding_exon:MODIFIER:exon8/9:n.1908C>T:,BARD1:NR_104216.1:non_coding_exon:MODIFIER:exon9/10:n.1164C>T:'),
(307332, 'chr3', 10085536, 10085536, 'A', 'G', 0, 0, 'FANCD2', 'synonymous', 'FANCD2:NM_033084.3:synonymous:LOW:exon14/43:c.1122A>G:p.Val374Val,FANCD2:NM_001018115.1:synonymous:LOW:exon14/44:c.1122A>G:p.Val374Val'),
(307339, 'chr3', 10106532, 10106532, 'C', 'T', 0, 0, 'FANCD2', 'missense', 'FANCD2:NM_033084.3:missense:MODERATE:exon23/43:c.2141C>T:p.Pro714Leu,FANCD2:NM_001018115.1:missense:MODERATE:exon23/44:c.2141C>T:p.Pro714Leu'),
(307347, 'chr3', 10138069, 10138069, 'T', 'G', 0.2422, 0.1647, 'FANCD2,FANCD2OS', 'synonymous,intron', 'FANCD2:NM_033084.3:synonymous:LOW:exon42/43:c.4098T>G:p.Leu1366Leu,FANCD2:NM_001018115.1:synonymous:LOW:exon42/44:c.4098T>G:p.Leu1366Leu,FANCD2OS:NM_173472.1:intron:MODIFIER:exon2/2:c.*43+7813A>C:'),
(307371, 'chr5', 79950781, 79950781, 'A', 'G', 0.7686, 0, 'MSH3,DHFR,MTRNR2L2', 'missense&splice_region,5''UTR,upstream_gene,non_coding_exon', 'MSH3:NM_002439.4:missense&splice_region:MODERATE:exon1/24:c.235A>G:p.Ile79Val,DHFR:NM_000791.3:5''UTR:MODIFIER:exon1/6:c.-473T>C:,DHFR:NM_001290354.1:5''UTR:MODIFIER:exon1/5:c.-579T>C:,DHFR:NM_001290357.1:5''UTR:MODIFIER:exon1/5:c.-473T>C:,MTRNR2L2:NM_001190470.1:upstream_gene:MODIFIER::c.-4876T>C:,DHFR:NR_110936.1:non_coding_exon:MODIFIER:exon1/4:n.20T>C:'),
(307379, 'chr5', 80168937, 80168937, 'G', 'A', 0.7202, 0.7094, 'MSH3', 'missense&splice_region', 'MSH3:NM_002439.4:missense&splice_region:MODERATE:exon23/24:c.3133G>A:p.Ala1045Thr'),
(307440, 'chr9', 21971137, 21971137, 'T', 'G', 0, 0, 'CDKN2A,CDKN2A-AS1', 'missense,synonymous,3''UTR,downstream_gene', 'CDKN2A:NM_001195132.1:missense:MODERATE:exon2/4:c.221A>C:p.Asp74Ala,CDKN2A:NM_000077.4:missense:MODERATE:exon2/3:c.221A>C:p.Asp74Ala,CDKN2A:NM_058195.3:synonymous:LOW:exon2/3:c.264A>C:p.Arg88Arg,CDKN2A:NM_058197.4:3''UTR:MODIFIER:exon2/3:c.*144A>C:,CDKN2A-AS1:NR_024274.1:downstream_gene:MODIFIER::n.*3384T>G:'),
(307447, 'chr9', 98209594, 98209594, 'G', 'A', 0.3968, 0.3298, 'PTCH1', 'missense', 'PTCH1:NM_000264.3:missense:MODERATE:exon23/24:c.3944C>T:p.Pro1315Leu,PTCH1:NM_001083607.1:missense:MODERATE:exon23/24:c.3491C>T:p.Pro1164Leu,PTCH1:NM_001083606.1:missense:MODERATE:exon23/24:c.3491C>T:p.Pro1164Leu,PTCH1:NM_001083604.1:missense:MODERATE:exon23/24:c.3491C>T:p.Pro1164Leu,PTCH1:NM_001083605.1:missense:MODERATE:exon23/24:c.3491C>T:p.Pro1164Leu,PTCH1:NM_001083602.1:missense:MODERATE:exon23/24:c.3746C>T:p.Pro1249Leu,PTCH1:NM_001083603.1:missense:MODERATE:exon23/24:c.3941C>T:p.Pro1314Leu'),
(309268, 'chr11', 108175394, 108175394, 'T', 'C', 0.0132, 0.0334, 'ATM', 'splice_region&intron', 'ATM:NM_000051.3:splice_region&intron:LOW:exon36/62:c.5497-8T>C:'),
(311040, 'chr17', 33445549, 33445549, 'G', 'A', 0.1875, 0.082, 'RAD51D,FNDC8,RAD51L3-RFFL', 'synonymous,upstream_gene,intron', 'RAD51D:NM_002878.3:synonymous:LOW:exon3/10:c.234C>T:p.Ser78Ser,FNDC8:NM_017559.2:upstream_gene:MODIFIER::c.-3164G>A:,RAD51L3-RFFL:NR_037714.1:intron:MODIFIER:exon1/6:n.232+2761C>T:,RAD51D:NM_001142571.1:intron:MODIFIER:exon2/9:c.144+581C>T:,RAD51D:NM_133629.2:intron:MODIFIER:exon2/6:c.144+581C>T:,RAD51D:NR_037711.1:intron:MODIFIER:exon2/8:n.400+581C>T:,RAD51D:NR_037712.1:intron:MODIFIER:exon2/7:n.400+581C>T:'),
(311089, 'chr19', 17389648, 17389648, 'C', 'T', 0.1118, 0.1799, 'BABAM1,ANKLE1', 'splice_region&intron,upstream_gene', 'BABAM1:NM_001033549.2:splice_region&intron:LOW:exon8/8:c.787-6C>T:,BABAM1:NM_001288756.1:splice_region&intron:LOW:exon8/8:c.787-6C>T:,BABAM1:NM_001288757.1:splice_region&intron:LOW:exon5/5:c.562-6C>T:,BABAM1:NM_014173.3:splice_region&intron:LOW:exon8/8:c.787-6C>T:,ANKLE1:NM_152363.5:upstream_gene:MODIFIER::c.-2920C>T:,ANKLE1:NM_001278443.1:upstream_gene:MODIFIER::c.-2920C>T:,ANKLE1:NM_001278444.1:upstream_gene:MODIFIER::c.-2920C>T:,ANKLE1:NM_001278445.1:upstream_gene:MODIFIER::c.-3011C>T:,ANKLE1:NR_103530.1:upstream_gene:MODIFIER::n.-2806C>T:'),
(311090, 'chr19', 17389704, 17389704, 'G', 'A', 0.1124, 0.1811, 'BABAM1,ANKLE1', 'synonymous,upstream_gene', 'BABAM1:NM_001033549.2:synonymous:LOW:exon9/9:c.837G>A:p.Lys279Lys,BABAM1:NM_001288756.1:synonymous:LOW:exon9/9:c.837G>A:p.Lys279Lys,BABAM1:NM_001288757.1:synonymous:LOW:exon6/6:c.612G>A:p.Lys204Lys,BABAM1:NM_014173.3:synonymous:LOW:exon9/9:c.837G>A:p.Lys279Lys,ANKLE1:NM_152363.5:upstream_gene:MODIFIER::c.-2864G>A:,ANKLE1:NM_001278443.1:upstream_gene:MODIFIER::c.-2864G>A:,ANKLE1:NM_001278444.1:upstream_gene:MODIFIER::c.-2864G>A:,ANKLE1:NM_001278445.1:upstream_gene:MODIFIER::c.-2955G>A:,ANKLE1:NR_103530.1:upstream_gene:MODIFIER::n.-2750G>A:'),
(313463, 'chr15', 43701947, 43701947, 'A', '-', 0.2502,0, 'TP53BP1,TUBGCP4', 'splice_region&intron,downstream_gene', 'TP53BP1:NM_001141980.1:splice_region&intron:LOW:exon24/27:c.5306-8delT:,TP53BP1:NM_001141979.1:splice_region&intron:LOW:exon24/27:c.5300-8delT:,TP53BP1:NM_005657.2:splice_region&intron:LOW:exon24/27:c.5291-8delT:,TUBGCP4:NM_001286414.2:downstream_gene:MODIFIER::c.*4535delA:,TUBGCP4:NM_014444.4:downstream_gene:MODIFIER::c.*4535delA:'),
(313480, 'chr17', 33433487, 33433487, 'C', 'T', 0.095, 0.1427, 'RAD51D,RAD51L3-RFFL', 'missense,non_coding_exon', 'RAD51D:NM_001142571.1:missense:MODERATE:exon6/10:c.554G>A:p.Arg185Gln,RAD51D:NM_002878.3:missense:MODERATE:exon6/10:c.494G>A:p.Arg165Gln,RAD51D:NM_133629.2:missense:MODERATE:exon3/7:c.158G>A:p.Arg53Gln,RAD51L3-RFFL:NR_037714.1:non_coding_exon:MODIFIER:exon2/7:n.246G>A:,RAD51D:NR_037711.1:non_coding_exon:MODIFIER:exon5/9:n.631G>A:,RAD51D:NR_037712.1:non_coding_exon:MODIFIER:exon4/8:n.496G>A:'),
(313553, 'chr14', 104165753, 104165753, 'G', 'A', 0.2169, 0.3782, 'XRCC3,KLC1', 'missense,intron', 'XRCC3:NM_001100118.1:missense:MODERATE:exon7/9:c.722C>T:p.Thr241Met,XRCC3:NM_001100119.1:missense:MODERATE:exon8/10:c.722C>T:p.Thr241Met,XRCC3:NM_005432.3:missense:MODERATE:exon8/10:c.722C>T:p.Thr241Met,KLC1:NM_001130107.1:intron:MODIFIER:exon14/15:c.1782-1239G>A:,KLC1:NM_182923.3:intron:MODIFIER:exon13/14:c.1651-1239G>A:'),
(555378, 'chr3', 142217537, 142217537, 'A', 'G', 0.0669, 0.1062, 'ATR', 'synonymous', 'ATR:NM_001184.3:synonymous:LOW:exon32/47:c.5460T>C:p.Tyr1820Tyr'),
(564081, 'chr1', 27687466, 27687466, 'G', 'T', 0.122, 0.2959, 'MAP3K6', 'missense', 'MAP3K6:NM_004672.4:missense:MODERATE:exon14/29:c.1866C>A:p.Asn622Lys,MAP3K6:NM_001297609.1:missense:MODERATE:exon13/28:c.1842C>A:p.Asn614Lys'),
(564637, 'chr1', 62713246, 62713246, 'G', 'A', 0.3189, 0.4209, 'KANK4', 'synonymous', 'KANK4:NM_181712.4:synonymous:LOW:exon9/10:c.2781C>T:p.His927His'),
(564643, 'chr1', 62738904, 62738904, 'T', 'C', 0.8005, 0.6733, 'KANK4', 'synonymous', 'KANK4:NM_181712.4:synonymous:LOW:exon3/10:c.1872A>G:p.Pro624Pro'),
(564644, 'chr1', 62739198, 62739198, 'G', 'A', 0.2506, 0.3309, 'KANK4', 'synonymous', 'KANK4:NM_181712.4:synonymous:LOW:exon3/10:c.1578C>T:p.Ser526Ser'),
(564645, 'chr1', 62740065, 62740065, 'A', 'G', 0.4822, 0.4271, 'KANK4', 'synonymous', 'KANK4:NM_181712.4:synonymous:LOW:exon3/10:c.711T>C:p.Gly237Gly'),
(565154, 'chr1', 120611960, 120611960, 'C', 'T', 0, 0, 'NOTCH2', 'missense', 'NOTCH2:NM_024408.3:missense:MODERATE:exon1/34:c.61G>A:p.Ala21Thr,NOTCH2:NM_001200001.1:missense:MODERATE:exon1/22:c.61G>A:p.Ala21Thr'),
(565156, 'chr1', 120612003, 120612004, 'GG', '-', 0, 0, 'NOTCH2', 'frameshift', 'NOTCH2:NM_024408.3:frameshift:HIGH:exon1/34:c.17_18delCC:p.Pro6fs,NOTCH2:NM_001200001.1:frameshift:HIGH:exon1/22:c.17_18delCC:p.Pro6fs'),
(565159, 'chr1', 120612034, 120612034, 'T', 'G', 0, 0, 'NOTCH2', '5''UTR', 'NOTCH2:NM_024408.3:5''UTR:MODIFIER:exon1/34:c.-14A>C:,NOTCH2:NM_001200001.1:5''UTR:MODIFIER:exon1/22:c.-14A>C:'),
(586510, 'chr2', 220435375, 220435375, 'G', 'A', 0.3588, 0.3695, 'OBSL1,INHA', 'synonymous,upstream_gene', 'OBSL1:NM_015311.2:synonymous:LOW:exon1/21:c.580C>T:p.Leu194Leu,OBSL1:NM_001173431.1:synonymous:LOW:exon1/14:c.580C>T:p.Leu194Leu,OBSL1:NM_001173408.1:synonymous:LOW:exon1/9:c.580C>T:p.Leu194Leu,INHA:NM_002191.3:upstream_gene:MODIFIER::c.-1722G>A:'),
(599471, 'chr9', 17394996, 17394996, 'T', 'C', 0.8411, 0.9149, 'CNTLN', 'synonymous', 'CNTLN:NM_017738.3:synonymous:LOW:exon15/26:c.2544T>C:p.His848His'),
(599473, 'chr9', 17409366, 17409366, 'G', 'A', 0.8389, 0.9115, 'CNTLN', 'synonymous', 'CNTLN:NM_017738.3:synonymous:LOW:exon16/26:c.2691G>A:p.Thr897Thr'),
(603008, 'chr1', 62740446, 62740446, 'T', 'C', 0.8045, 0.6742, 'KANK4', 'synonymous', 'KANK4:NM_181712.4:synonymous:LOW:exon3/10:c.330A>G:p.Pro110Pro'),
(603009, 'chr1', 62740449, 62740449, 'T', 'C', 0.781, 0.6734, 'KANK4', 'synonymous', 'KANK4:NM_181712.4:synonymous:LOW:exon3/10:c.327A>G:p.Ser109Ser'),
(639841, 'chr9', 17273878, 17273878, 'A', 'G', 0.2258, 0.2651, 'CNTLN', 'splice_region&intron', 'CNTLN:NM_017738.3:splice_region&intron:LOW:exon6/25:c.983+14A>G:,CNTLN:NM_001114395.2:splice_region&intron:LOW:exon6/6:c.983+14A>G:'),
(643627, 'chr1', 62732421, 62732421, 'T', 'C', 0.3055, 0.2449, 'KANK4', 'missense', 'KANK4:NM_181712.4:missense:MODERATE:exon6/10:c.2302A>G:p.Thr768Ala'),
(644295, 'chr1', 120612006, 120612006, 'G', 'A', 0.394, 0, 'NOTCH2', 'synonymous', 'NOTCH2:NM_024408.3:synonymous:LOW:exon1/34:c.15C>T:p.Arg5Arg,NOTCH2:NM_001200001.1:synonymous:LOW:exon1/22:c.15C>T:p.Arg5Arg'),
(666218, 'chr2', 17962518, 17962518, 'C', 'T', 0.758, 0.6245, 'GEN1', 'missense', 'GEN1:NM_001130009.1:missense:MODERATE:exon14/14:c.2039C>T:p.Thr680Ile,GEN1:NM_182625.3:missense:MODERATE:exon14/14:c.2039C>T:p.Thr680Ile'),
(683610, 'chr9', 17394536, 17394536, 'C', 'T', 0.7806, 0.8763, 'CNTLN', 'missense', 'CNTLN:NM_017738.3:missense:MODERATE:exon15/26:c.2084C>T:p.Thr695Ile'),
(1447293, 'chr14', 45606387, 45606387, 'A', 'G', 0.0054, 0.0151, 'FANCM,FKBP3', 'missense,upstream_gene', 'FANCM:NM_020937.3:missense:MODERATE:exon2/23:c.624A>G:p.Ile208Met,FANCM:NM_001308134.1:missense:MODERATE:exon2/11:c.624A>G:p.Ile208Met,FANCM:NM_001308133.1:missense:MODERATE:exon2/22:c.624A>G:p.Ile208Met,FKBP3:NM_002013.3:upstream_gene:MODIFIER::c.-2728T>C:'),
(1502204, 'chr2', 48030692, 48030692, 'T', 'A', 0.0493, 0.0016, 'MSH6,FBXO11', 'synonymous,downstream_gene', 'MSH6:NM_000179.2:synonymous:LOW:exon5/10:c.3306T>A:p.Thr1102Thr,MSH6:NM_001281492.1:synonymous:LOW:exon3/8:c.2916T>A:p.Thr972Thr,MSH6:NM_001281493.1:synonymous:LOW:exon4/9:c.2400T>A:p.Thr800Thr,MSH6:NM_001281494.1:synonymous:LOW:exon5/10:c.2400T>A:p.Thr800Thr,FBXO11:NM_001190274.1:downstream_gene:MODIFIER::c.*4565A>T:,FBXO11:NM_025133.4:downstream_gene:MODIFIER::c.*4565A>T:'),
(2032063, 'chr3', 142188337, 142188337, 'A', 'C', 0.002, 0.0069, 'ATR', 'missense', 'ATR:NM_001184.3:missense:MODERATE:exon38/47:c.6394T>G:p.Tyr2132Asp'),
(2105956, 'chr9', 17457513, 17457513, 'A', '-', 0.7342, 0.9938, 'CNTLN', 'splice_acceptor', 'CNTLN:NM_017738.3:splice_acceptor:HIGH:exon18/25:c.3115-2delA:'),
(2134105, 'chr1', 120539331, 120539331, 'C', 'T', 0, 0, 'NOTCH2', 'intron', 'NOTCH2:NM_024408.3:intron:MODIFIER:exon4/33:c.751+289G>A:,NOTCH2:NM_001200001.1:intron:MODIFIER:exon4/21:c.751+289G>A:'),
(2184394, 'chr1', 120611496, 120611496, 'C', 'A', 0, 0, 'NOTCH2', 'intron', 'NOTCH2:NM_024408.3:intron:MODIFIER:exon1/33:c.73+452G>T:,NOTCH2:NM_001200001.1:intron:MODIFIER:exon1/21:c.73+452G>T:'),
(2207224, 'chr2', 220422686, 220422686, 'C', 'T', 0.007, 0.0055, 'OBSL1', 'missense,downstream_gene', 'OBSL1:NM_015311.2:missense:MODERATE:exon11/21:c.3649G>A:p.Glu1217Lys,OBSL1:NM_001173431.1:missense:MODERATE:exon11/14:c.3649G>A:p.Glu1217Lys,OBSL1:NM_001173408.1:downstream_gene:MODIFIER::c.*3920G>A:'),
(2336573, 'chr5', 131925483, 131925483, 'G', 'C', 0.0002, 0, 'RAD50', 'missense', 'RAD50:NM_005732.3:missense:MODERATE:exon9/25:c.1406G>C:p.Gly469Ala'),
(2336993, 'chr16', 3639230, 3639230, 'G', 'A', 0.0002, 0.0002, 'SLX4', 'missense', 'SLX4:NM_032444.2:missense:MODERATE:exon12/15:c.4409C>T:p.Pro1470Leu'),
(2346586, 'chr7', 6037057, 6037057, '-', 'A', 0.0264, 0, 'PMS2', 'splice_region&intron', 'PMS2:NM_000535.5:splice_region&intron:LOW:exon6/14:c.706-4dupT:,PMS2:NR_003085.2:splice_region&intron:LOW:exon6/14:n.788-4dupT:'),
(2346588, 'chr7', 140453136, 140453136, 'T', 'A', 0.0, 0.0, 'BRAF', 'missense', 'BRAF:ENST00000288602:missense_variant:MODERATE:exon15/18:c.1799T>A:p.Val600Glu:PF07714'),
(2407544, 'chr9', 98232224, 98232224, 'A', '-', 0, 0.0018, 'PTCH1,LOC100507346', 'splice_region&intron,non_coding_exon', 'PTCH1:NM_000264.3:splice_region&intron:LOW:exon12/23:c.1729-11delT:,PTCH1:NM_001083607.1:splice_region&intron:LOW:exon12/23:c.1276-11delT:,PTCH1:NM_001083606.1:splice_region&intron:LOW:exon12/23:c.1276-11delT:,PTCH1:NM_001083604.1:splice_region&intron:LOW:exon12/23:c.1276-11delT:,PTCH1:NM_001083605.1:splice_region&intron:LOW:exon12/23:c.1276-11delT:,PTCH1:NM_001083602.1:splice_region&intron:LOW:exon12/23:c.1531-11delT:,PTCH1:NM_001083603.1:splice_region&intron:LOW:exon12/23:c.1726-11delT:,LOC100507346:NR_038982.1:non_coding_exon:MODIFIER:exon4/4:n.1887delA:'),
(2407599, 'chr9', 98232299, 98232299, '-', 'AGT', 0, 0.0018, 'unused', 'unused', 'unused'),
(2407600, 'chr6', 26093141, 26093141, 'G', 'A', 0, 0.0001, 'unused', 'unused', 'unused'),
(2407601, 'chr2', 178096717, 178096717, 'T', 'C', 0, 0.0001, 'unused', 'unused', 'unused');

INSERT INTO `detected_somatic_variant` (`id`, `processed_sample_id_tumor`, `processed_sample_id_normal`, `variant_id`, `variant_frequency`, `depth`, `quality_snp`) VALUES
(5, 4000, 3999, 2346588, 0.6, 642, 0.9);


INSERT INTO `detected_variant` (`processed_sample_id`, `variant_id`, `genotype`) VALUES
(3999, 6, 'hom'),
(3999, 405, 'het'),
(3999, 447, 'het'),
(3999, 199842, 'hom'),
(3999, 199843, 'hom'),
(3999, 199844, 'hom'),
(3999, 199845, 'hom'),
(3999, 199846, 'hom'),
(3999, 200567, 'het'),
(3999, 200575, 'het'),
(3999, 203397, 'hom'),
(3999, 204170, 'hom'),
(3999, 204172, 'het'),
(3999, 204173, 'hom'),
(3999, 206534, 'hom'),
(3999, 206535, 'hom'),
(3999, 207343, 'het'),
(3999, 207345, 'hom'),
(3999, 207750, 'hom'),
(3999, 210578, 'het'),
(3999, 210580, 'hom'),
(3999, 210582, 'hom'),
(3999, 210584, 'het'),
(3999, 210585, 'hom'),
(3999, 213346, 'het'),
(3999, 213347, 'hom'),
(3999, 213349, 'hom'),
(3999, 213350, 'hom'),
(3999, 214723, 'het'),
(3999, 215568, 'het'),
(3999, 216149, 'het'),
(3999, 216155, 'het'),
(3999, 217254, 'hom'),
(3999, 218033, 'hom'),
(3999, 218034, 'hom'),
(3999, 218035, 'hom'),
(3999, 222995, 'hom'),
(3999, 222996, 'hom'),
(3999, 222997, 'hom'),
(3999, 225333, 'het'),
(3999, 225334, 'hom'),
(3999, 225336, 'het'),
(3999, 225337, 'het'),
(3999, 225526, 'hom'),
(3999, 225528, 'hom'),
(3999, 225529, 'hom'),
(3999, 225530, 'hom'),
(3999, 225532, 'hom'),
(3999, 225533, 'hom'),
(3999, 225536, 'hom'),
(3999, 225541, 'hom'),
(3999, 225543, 'hom'),
(3999, 229029, 'het'),
(3999, 230800, 'hom'),
(3999, 230801, 'het'),
(3999, 230806, 'het'),
(3999, 230807, 'het'),
(3999, 234144, 'hom'),
(3999, 234361, 'het'),
(3999, 234362, 'het'),
(3999, 234365, 'het'),
(3999, 234366, 'het'),
(3999, 234367, 'het'),
(3999, 234368, 'het'),
(3999, 235808, 'het'),
(3999, 237622, 'hom'),
(3999, 237626, 'hom'),
(3999, 239025, 'hom'),
(3999, 239327, 'hom'),
(3999, 239334, 'hom'),
(3999, 240595, 'het'),
(3999, 240596, 'het'),
(3999, 240598, 'het'),
(3999, 240600, 'het'),
(3999, 240602, 'het'),
(3999, 241479, 'hom'),
(3999, 241663, 'hom'),
(3999, 307040, 'het'),
(3999, 307049, 'het'),
(3999, 307137, 'het'),
(3999, 307158, 'hom'),
(3999, 307173, 'het'),
(3999, 307180, 'hom'),
(3999, 307218, 'het'),
(3999, 307223, 'het'),
(3999, 307225, 'het'),
(3999, 307226, 'het'),
(3999, 307227, 'het'),
(3999, 307228, 'het'),
(3999, 307229, 'het'),
(3999, 307268, 'het'),
(3999, 307285, 'het'),
(3999, 307299, 'het'),
(3999, 307332, 'het'),
(3999, 307339, 'het'),
(3999, 307347, 'het'),
(3999, 307371, 'het'),
(3999, 307379, 'hom'),
(3999, 307440, 'het'),
(3999, 307447, 'het'),
(3999, 309268, 'het'),
(3999, 311040, 'het'),
(3999, 311089, 'het'),
(3999, 311090, 'het'),
(3999, 313463, 'het'),
(3999, 313480, 'hom'),
(3999, 313553, 'het'),
(3999, 555378, 'het'),
(3999, 564081, 'het'),
(3999, 564637, 'het'),
(3999, 564643, 'hom'),
(3999, 564644, 'het'),
(3999, 564645, 'het'),
(3999, 565154, 'het'),
(3999, 565156, 'het'),
(3999, 565159, 'het'),
(3999, 586510, 'het'),
(3999, 599471, 'hom'),
(3999, 599473, 'hom'),
(3999, 603008, 'hom'),
(3999, 603009, 'hom'),
(3999, 639841, 'het'),
(3999, 643627, 'het'),
(3999, 644295, 'het'),
(3999, 666218, 'hom'),
(3999, 683610, 'hom'),
(3999, 1447293, 'het'),
(3999, 1502204, 'het'),
(3999, 2032063, 'het'),
(3999, 2105956, 'hom'),
(3999, 2134105, 'het'),
(3999, 2184394, 'het'),
(3999, 2207224, 'het'),
(3999, 2336573, 'het'),
(3999, 2336993, 'het'),
(3999, 2346586, 'het'),
(3999, 2407544, 'het'),
(4000, 2346586, 'hom'),
(4000, 2407544, 'het'),
(4001, 2346586, 'hom'),
(4001, 2407544, 'het'),
(4003, 2346586, 'hom'),
(4003, 2407544, 'het');

INSERT INTO `hpo_term`(`id`, `hpo_id`, `name`, `definition`, `synonyms`) VALUES
(1,'HP:0000001','All','def',''),
(2,'HP:0000005','Mode of inheritance','def',''),
(3,'HP:0000118','Phenotypic abnormality','def',''),
(4,'HP:0012823','Clinical modifier','def','synonym 1'),
(5,'HP:0040279','Frequency','def','synonym 2'),
(6,'HP:0000006','Autosomal dominant inheritance','def',''),
(7,'HP:0000007','Autosomal recessive inheritance','def',''),
(8,'HP:0001427','Mitochondrial inheritance','def',''),
(9,'HP:0001417','X-linked inheritance','def',''),
(10,'HP:0001419','X-linked recessive inheritance','def',''),
(11,'HP:0001423','X-linked dominant inheritance','def',''),
(12,'HP:0001251','Ataxia','def','');

INSERT INTO `hpo_parent`(`parent`, `child`) VALUES
(1,2),
(1,3),
(1,4),
(1,5),
(2,6),
(2,7),
(2,8),
(2,9),
(9,10),
(9,11);

INSERT INTO `qc_terms`(`id`, `qcml_id`, `name`, `description`, `type`, `obsolete`) VALUES
(31, "QC:2000027", "target region 20x percentage", "Percentage of the target region that is covered at...", 'float', 0),
(47, "QC:2000025", "target region read depth", "Average sequencing depth in target region.", 'float', 0);

INSERT INTO `processed_sample_qc`(`id`, `processed_sample_id`, `qc_terms_id`, `value`) VALUES
(1, 3999, 31, "95.96"),
(2, 3999, 47, "103.24"),
(3, 4001, 47, "132.24"),
(4, 8,    47, "457.34"),
(5, 7,    47, "251.78");


INSERT INTO `analysis_job`(`type`, `high_priority`, `args`, `sge_id`, `sge_queue`) VALUES
('single sample',0,'','4711','default_srv018');

INSERT INTO `analysis_job_sample`(`analysis_job_id`, `processed_sample_id`, `info`) VALUES
(1, 3999, '');

INSERT INTO `analysis_job_history`(`analysis_job_id`, `time`, `user_id`, `status`, `output`) VALUES
(1, '2018-02-12T10:20:00', 99, 'queued', ''),
(1, '2018-02-12T10:20:45', null, 'started', ''),
(1, '2018-02-12T10:34:09', null, 'finished', 'warning: bla bla bla');

INSERT INTO `sample_relations`(`sample1_id`, `relation`, `sample2_id`) VALUES
(2, 'same sample', 4);

INSERT INTO `sample_disease_info`(`id`, `sample_id`, `disease_info`, `type`, `user_id`) VALUES
(1, 3, 'HP:0001251', 'HPO term id', 99),
(2, 5, 'C17.2', 'ICD10 code', 99),
(3, 5, 'C20.21', 'ICD10 code', 99);


-- cnv_callset
INSERT INTO `cnv_callset` (`id`, `processed_sample_id`, `caller`, `caller_version`, `call_date`, `quality_metrics`, `quality`) VALUES
(1, 3999, 'ClinCNV', 'v 1.16.1', '2019-10-20T09:55:01', '{"fraction of outliers":"0.052","gender of sample":"M","high-quality cnvs":"127","number of iterations":"1","quality used at final iteration":"20","was it outlier after clustering":"FALSE"}', 'good'),
(2, 4003, 'ClinCNV', 'v 1.16.1', '2019-10-20T09:55:01', '{"fraction of outliers":"0.052","gender of sample":"M","high-quality cnvs":"127","number of iterations":"1","quality used at final iteration":"20","was it outlier after clustering":"FALSE"}', 'good');

-- cnv
INSERT INTO `cnv` (`id`, `cnv_callset_id`, `chr`, `start`, `end`, `cn`) VALUES
(1, 1, 'chr1', 1000, 2000, 1),
(2, 1, 'chr1', 3000, 4000, 1),
(3, 1, 'chr2', 10000, 40000, 1),
(4, 2, 'chr1', 3000, 4000, 1);

-- report config
INSERT INTO `report_configuration` (`id`, `processed_sample_id`, `created_by`, `created_date`, `last_edit_by`, `last_edit_date`) VALUES
(1, 4003, 99, '2018-02-12T10:20:45', 99, '2018-07-12T10:20:43');

INSERT INTO `report_configuration_variant` (`report_configuration_id`, `variant_id`, `type`, `causal`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES
(1, 2407544, 'diagnostic variant', '1', 'AD', '0', '0', '0', '0', '0', '0', '0', '0', 'bla', 'bla bla');

INSERT INTO `report_configuration_cnv` (`report_configuration_id`, `cnv_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES
(1, 4, 'diagnostic variant', '1', '4', 'AD', '0', '0', '0', '0', '0', '0', '0', '0', 'bla', 'bla bla');

-- somatic_cnv_callset_id`
INSERT INTO `somatic_cnv_callset` (`id`, `ps_tumor_id`, `ps_normal_id`, `caller`, `caller_version`, `call_date`, `quality_metrics`, `quality`) VALUES
(1, 4000, 3999, 'ClinCNV', 'v 1.16', '2020-01-12T13:35:01', '{"estimated fdr":"0","gender of sample":"F","ploidy":"2.21","clonality by BAF (if != 1)":"0;0.725;0.25"}', 'good'),
(5, 4002, 4001, 'ClinCNV', 'v 1.16', '2019-01-12T13:35:01', '{"estimated fdr":"0","gender of sample":"F","ploidy":"2.21","clonality by BAF (if != 1)":"0;0.725;0.25"}', 'bad');

-- somatic cnv_callset
INSERT INTO `somatic_cnv` (`id`, `somatic_cnv_callset_id`, `chr`, `start`, `end`, `cn`, `tumor_cn`, `tumor_clonality`) VALUES
(1, 1, 'chr4', 18000, 200000, 2.54, 3, 0.75),
(4, 5, 'chr7', 87000, 350000, 3.14, 4, 0.8);

-- somatic_report_configuration
INSERT INTO `somatic_report_configuration` (`id`, `ps_tumor_id`, `ps_normal_id`, `created_by`, `created_date`, `last_edit_by`, `last_edit_date`, `mtb_xml_upload_date`, `target_file`, `tum_content_max_af`, `tum_content_max_clonality`, `tum_content_hist`, `msi_status`, `cnv_burden`, `hrd_score`, `tmb_ref_text`, `quality`, `filter`) VALUES 
(3,5,6,3,'2019-01-05 14:06:12', 99, '2019-12-07 17:06:10', '2020-07-29 09:06:10', NULL, false, false, false, false, false, 0, NULL, 'tumor cell content too low', 'somatic'),
(51,5,4000,99,'2019-01-05 14:06:12', 101, '2019-12-07 17:06:10', '2020-07-27 09:20:10', 'nowhere.bed' , true, true, true, true, true, 1, "Median: 1.70 Var/Mbp, Maximum: 10.80 Var/Mbp", NULL, NULL);

-- somatic_report_configuration_cnv
INSERT INTO `somatic_report_configuration_cnv` (`somatic_report_configuration_id`, `somatic_cnv_id`, `exclude_artefact`, `exclude_low_tumor_content` , `exclude_low_copy_number`,
`exclude_high_baf_deviation`, `exclude_other_reason`, `comment`) VALUES
(3, 4, true, false, false, false, false, "");

-- somatic_vic
INSERT INTO `somatic_vicc_interpretation` (`id`, `variant_id`, `null_mutation_in_tsg`, `known_oncogenic_aa`, `strong_cancerhotspot`, `located_in_canerhotspot`, `absent_from_controls`, `protein_length_change`, `other_aa_known_oncogenic`, `weak_cancerhotspot`, `computational_evidence`, `mutation_in_gene_with_etiology`, `very_weak_cancerhotspot`, `very_high_maf`, `benign_functional_studies`, `high_maf`, `benign_computational_evidence`, `synonymous_mutation`, `comment`, `created_by`, `created_date`, `last_edit_by`, `last_edit_date`) VALUES
(1, 210585, true, false, false, true, true, null, true, false, null, false, true, false, false, false, false, null, 'this variant was evaluated as an oncogenic variant', 99, '2020-11-05 13:06:13', 101, '2020-12-07 11:06:10'),
(2, 213346, false, true, false, true, true, null, true, false, null, false, true, true, true, true, true, true, 'this variant was evaluated as variant of unclear significance', 99, '2020-12-05 12:07:11', 101, '2020-12-08 13:45:11'),
(3, 2407600,  true, false, false, true, true, null, true, false, null, false, true, false, false, false, false, null, 'this variant was evaluated as an oncogenic variant', 99, '2021-01-05 13:06:13', 101, '2021-02-07 11:06:10'),
(4, 2407601,  false, false, false, false, true, null, true, false, null, false, true, true, true, false, false, null, 'this variant was evaluated as an oncogenic variant', 99, '2021-02-06 11:06:14', 101, '2021-02-08 13:06:10');


-- omim
INSERT INTO `omim_gene` (`id`, `gene`, `mim`) VALUES
(199989, 'ATM', '607585'),
(193906, 'SHOX', '312865'),
(193942, 'SHOX', '400020');

INSERT INTO `omim_phenotype` (`omim_gene_id`, `phenotype`) VALUES
(199989, 'Ataxia-telangiectasia, 208900 (3)'),
(199989, 'Lymphoma, B-cell non-Hodgkin, somatic (3)'),
(199989, 'Lymphoma, mantle cell, somatic (3)'),
(199989, 'T-cell prolymphocytic leukemia, somatic (3)'),
(199989, '{Breast cancer, susceptibility to}, 114480 (3)'),
(193906, 'Langer mesomelic dysplasia, 249700 (3)'),
(193906, 'Leri-Weill dyschondrosteosis, 127300 (3)'),
(193906, 'Short stature, idiopathic familial, 300582 (3)'),
(193942, 'Langer mesomelic dysplasia, 249700 (3)'),
(193942, 'Leri-Weill dyschondrosteosis, 127300 (3)'),
(193942, 'Short stature, idiopathic familial, 300582 (3)');

INSERT INTO `omim_preferred_phenotype`(`gene`, `disease_group`, `phenotype_accession`) VALUES
('ATM', 'Neoplasms', '114480');

-- structural variants
INSERT INTO `sv_callset` (`id`, `processed_sample_id`, `caller`, `caller_version`, `call_date`) VALUES
(1, 3999, 'Manta', '1.6.0', '2020-01-01');

INSERT INTO `sv_deletion` (`id`, `sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `quality_metrics`) VALUES
(1, 1, 'chr1', 1000, 1020, 12000, 13000, ''),
(2, 1, 'chr1', 1000, 1020, 20000, 20000, ''),
(3, 1, 'chr1', 5, 50, 12000, 13000, '');

INSERT INTO `sv_duplication` (`id`, `sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `quality_metrics`) VALUES
(1, 1, 'chr1', 101000, 101020, 112000, 113000, ''),
(2, 1, 'chr1', 101000, 101020, 120000, 120000, ''),
(3, 1, 'chr1', 100005, 100050, 112000, 113000, '');

INSERT INTO `sv_inversion` (`id`, `sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `quality_metrics`) VALUES
(1, 1, 'chr1', 9101000, 9101020, 9112000, 9113000, ''),
(2, 1, 'chr1', 9101000, 9101020, 9120000, 9120000, ''),
(3, 1, 'chr1', 9100005, 9100050, 9112000, 9113000, '');

INSERT INTO `sv_insertion` (`id`, `sv_callset_id`, `chr`, `pos`, `ci_upper`) VALUES
(1, 1, 'chr1', 15482205, 250),
(2, 1, 'chr1', 16482455, 60),
(3, 1, 'chr1', 17482432, 77);

INSERT INTO `sv_translocation` (`id`, `sv_callset_id`, `chr1`, `start1`, `end1`, `chr2`, `start2`, `end2`, `quality_metrics`) VALUES
(1, 1, 'chr1', 9101000, 9101020, 'chr5', 4112000, 4113000, ''),
(2, 1, 'chr1', 9101000, 9101020, 'chr5', 4120000, 4120000, ''),
(3, 1, 'chr1', 9100005, 9100050, 'chr5', 4112000, 4113000, '');

-- study
INSERT INTO `study`(`id`, `name`, `description`) VALUES
(1, "SomeStudy","");

INSERT INTO `study_sample`(`study_id`, `processed_sample_id`) VALUES
(1, 3999),
(1, 4000),
(1, 4001),
(1, 4002),
(1, 4003),
(1, 5),
(1, 6),
(1, 7),
(1, 8);

INSERT INTO `subpanels` (`id`, `name`, `created_by`, `created_date`, `mode`, `extend`, `genes`, `roi`, `archived`) VALUES
(1, 'some_target_region', 99, '2021-04-15', 'exon', 5, 'WDPCP\r\n', 'chr2	63349135	63349196\r\nchr2	63380043	63380085\r\nchr2	63380624	63380714\r\nchr2	63401799	63401972\r\nchr2	63456957	63457012\r\nchr2	63486436	63486549\r\nchr2	63540377	63540451\r\nchr2	63605515	63605649\r\nchr2	63609035	63609234\r\nchr2	63631177	63631797\r\nchr2	63660873	63661075\r\nchr2	63664549	63664693\r\nchr2	63664990	63665022\r\nchr2	63666885	63667010\r\nchr2	63711732	63711802\r\nchr2	63712045	63712126\r\nchr2	63713670	63713725\r\nchr2	63714575	63714633\r\nchr2	63719984	63720079\r\nchr2	63815325	63815410\r\n', 0);
