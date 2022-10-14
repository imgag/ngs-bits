INSERT INTO `gene`(`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES
	('79301','25662','AAGAB','alpha and gamma adaptin binding protein','protein-coding gene'),
	('79463','25070','ACD','ACD, shelterin complex subunit and telomerase recr...','protein-coding gene'),
	('80028','391','AKT1','AKT serine/threonine kinase 1','protein-coding gene'),
	('80441','583','APC','APC, WNT signaling pathway regulator','protein-coding gene'),
	('81157','882','ATR','ATR serine/threonine kinase','protein-coding gene'),
	('81308','950','BAP1','BRCA1 associated protein 1','protein-coding gene'),
	('81674','1100','BRCA1','BRCA1, DNA repair associated','protein-coding gene'),
	('81709','20473','BRIP1','BRCA1 interacting protein C-terminal helicase 1','protein-coding gene'),
	('83194','1748','CDH1','cadherin 1','protein-coding gene'),
	('83239','1773','CDK4','cyclin dependent kinase 4','protein-coding gene'),
	('83277','1787','CDKN2A','cyclin dependent kinase inhibitor 2A','protein-coding gene'),
	('83284','1788','CDKN2B','cyclin dependent kinase inhibitor 2B','protein-coding gene'),
	('83287','1790','CDKN2D','cyclin dependent kinase inhibitor 2D','protein-coding gene'),
	('83629','16627','CHEK2','checkpoint kinase 2','protein-coding gene'),
	('84170','2191','COL14A1','collagen type XIV alpha 1 chain','protein-coding gene'),
	('84688','2514','CTNNB1','catenin beta 1','protein-coding gene'),
	('87897','3689','FGFR2','fibroblast growth factor receptor 2','protein-coding gene'),
	('89025','4392','GNAS','GNAS complex locus','protein-coding gene'),
	('90916','5382','IDH1','isocitrate dehydrogenase (NADP(+)) 1, cytosolic','protein-coding gene'),
	('90918','5383','IDH2','isocitrate dehydrogenase (NADP(+)) 2, mitochondria...','protein-coding gene'),
	('92490','37212','KLLN','killin, p53-regulated DNA replication inhibitor','protein-coding gene'),
	('92539','6407','KRAS','KRAS proto-oncogene, GTPase','protein-coding gene'),
	('95188','6636','LMNA','lamin A/C','protein-coding gene'),
	('95887','6929','MC1R','melanocortin 1 receptor','protein-coding gene'),
	('95957','6973','MDM2','MDM2 proto-oncogene','protein-coding gene'),
	('96186','7059','MGMT','O-6-methylguanine-DNA methyltransferase','protein-coding gene'),
	('98207','7105','MITF','melanogenesis associated transcription factor','protein-coding gene'),
	('98240','7127','MLH1','mutL homolog 1','protein-coding gene'),
	('98669','7325','MSH2','mutS homolog 2','protein-coding gene'),
	('98674','7329','MSH6','mutS homolog 6','protein-coding gene'),
	('100636','8028','NTHL1','nth like DNA glycosylase 1','protein-coding gene'),
	('102817','8975','PIK3CA','phosphatidylinositol-4,5-bisphosphate 3-kinase cat...','protein-coding gene'),
	('103417','17284','POT1','protection of telomeres 1','protein-coding gene'),
	('103553','9277','PPM1D','protein phosphatase, Mg2+/Mn2+ dependent 1D','protein-coding gene'),
	('103861','8607','PRKN','parkin RBR E3 ubiquitin protein ligase','protein-coding gene'),
	('104236','9588','PTEN','phosphatase and tensin homolog','protein-coding gene'),
	('104770','15574','RB1CC1','RB1 inducible coiled-coil 1','protein-coding gene'),
	('111530','10681','SDHB','succinate dehydrogenase complex iron sulfur subuni...','protein-coding gene'),
	('111531','10682','SDHC','succinate dehydrogenase complex subunit C','protein-coding gene'),
	('111536','10683','SDHD','succinate dehydrogenase complex subunit D','protein-coding gene'),
	('111582','10702','SEC23B','Sec23 homolog B, coat complex II component','protein-coding gene'),
	('114220','11389','STK11','serine/threonine kinase 11','protein-coding gene'),
	('114955','19246','TERF2IP','TERF2 interacting protein','protein-coding gene'),
	('114957','11730','TERT','telomerase reverse transcriptase','protein-coding gene'),
	('115836','11998','TP53','tumor protein p53','protein-coding gene'),
	('117094','15971','TSG101','tumor susceptibility 101','protein-coding gene'),
	('117426','12428','TWIST1','twist family bHLH transcription factor 1','protein-coding gene'),
	('118584','12791','WRN','Werner syndrome RecQ like helicase','protein-coding gene'),
	('79360', '14639', 'ABCC11', 'ATP binding cassette subfamily C member 11', 'protein-coding gene'),
	('80974', '795', 'ATM', 'ATM serine/threonine kinase', 'protein-coding gene'),
	('81269', '935', 'BACH1', 'BTB domain and CNC homolog 1', 'protein-coding gene'),
	('81309', '952', 'BARD1', 'BRCA1 associated RING domain 1', 'protein-coding gene'),
	('81676', '1101', 'BRCA2', 'BRCA2, DNA repair associated', 'protein-coding gene'),
	('82619', '1509', 'CASP8', 'caspase 8', 'protein-coding gene'),
	('86959', '3467', 'ESR1', 'estrogen receptor 1', 'protein-coding gene'),
	('90346', '5012', 'HMMR', 'hyaluronan mediated motility receptor', 'protein-coding gene'),
	('100504', '7856', 'NQO2', 'N-ribosyldihydronicotinamide:quinone reductase 2', 'protein-coding gene'),
	('100916', '8143', 'OPCML', 'opioid binding protein/cell adhesion molecule like', 'protein-coding gene'),
	('102040', '26144', 'PALB2', 'partner and localizer of BRCA2', 'protein-coding gene'),
	('102651', '8912', 'PHB', 'prohibitin', 'protein-coding gene'),
	('104619', '9826', 'RAD54L', 'RAD54 like (S. cerevisiae)', 'protein-coding gene'),
	('118673', '12830', 'XRCC3', 'X-ray repair cross complementing 3', 'protein-coding gene');
	
INSERT INTO `gene_alias` (`gene_id`, `symbol`, `type`) VALUES
(104236, 'PTEN_ALT', 'synonym');

INSERT INTO `user`(`id`, `user_id`, `password`, `user_role`, `name`, `email`, `created`, `active`) VALUES
(99, 'ahmustm1', '', 'user', 'Max Mustermann', '', '2022-10-14', 1);

INSERT INTO `sender` (`id`, `name`) VALUES
(1, 'Coriell');

INSERT INTO `sample` (`id`, `name`, `sample_type`, `species_id`, `gender`, `quality`, `tumor`, `ffpe`, `sender_id`) VALUES 
(1, 'NA12878', 'DNA', 1, 'n/a', 'good', 0 ,0, 1);

INSERT INTO `sample_disease_info`(`id`, `sample_id`, `disease_info`, `type`, `user_id`, `date`) VALUES
(1, 1, 'HP:0003114', 'HPO term id', 99, '2022-10-14 14:33:35'),
(2, 1, 'HP:???????', 'HPO term id', 99, '2022-10-14 14:33:35');