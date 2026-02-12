
-- device
INSERT INTO device (id, type, name) VALUES 
(1, 'HiSeq2500', 'Morpheus');

-- sequencing_run
INSERT INTO sequencing_run (id, name, fcid, device_id, recipe, quality) VALUES 
(1, 'run1', 'ABC', 1, '100+8+8+100', 'good');

-- user
INSERT INTO user (id, user_id, password, user_role, name, email, created, active) VALUES 
(99, 'ahuser', 's2d12kjg234hla0830t6hp9h3tt3t3tsdfg', 'user', 'The user', 'u@s.er', NOW(), '1'),
(100, 'ahReportCreator', '123', 'user', 'Report Creator', 'ahReportCreator@email.com', NOW(), '1'),
(101, 'ahReportModifier', '123', 'user', 'Report Modifier', 'ahReportModifier@email.com', NOW(), '1');

-- sender
INSERT INTO sender (id, name) VALUES 
(1, 'Klaus-Erhard');

-- project
INSERT INTO project (id, name, type, internal_coordinator_id, analysis, archived) VALUES 
(1, 'First_project', 'research', 1, 'variants', 0);

-- processing_system
INSERT INTO processing_system (id, name_manufacturer, shotgun, name_short, genome_id) VALUES
(1, 'SureSelect Human All Exon v5', '1', 'ssHAEv5', 1);

-- sample
INSERT INTO sample (id, name, sample_type, species_id, gender, tumor, ffpe, sender_id, quality, disease_group, disease_status, tissue, received, year_of_birth) VALUES
(1, 'NA12878', 'DNA', 1, 'female', '0', '0', 1, 'good', 'Neoplasms', 'Affected', 'blood', '2023-07-13', 1977);

-- processed_sample
INSERT INTO processed_sample (id, sample_id, process_id, sequencing_run_id, lane, operator_id, processing_system_id, project_id, quality, normal_id) VALUES
(1, 1, 5, 1, 1, 1, 1, 1, 'good', NULL),
(2, 1, 6, 1, 1, 1, 1, 1, 'good', NULL);

--variants:
INSERT INTO `variant` (`id`,`chr`,`start`,`end`,`ref`,`obs`,`gnomad`,`coding`,`comment`,`cadd`,`spliceai`,`germline_het`,`germline_hom`,`germline_mosaic`) VALUES 
(1,'chr1',12198,12198,'G','C',0,'DDX11L16:ENST00000832828.1:non_coding_transcript_exon_variant:MODIFIER,DDX11L1:ENST00000450305.2:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831158.1:downstream_gene_variant:MODIFIER,WASH7P:ENST00000831210.1:downstream_gene_variant:MODIFIER,WASH7P:ENST00000831361.1:downstream_gene_variant:MODIFIER,WASH7P:ENST00000831289.1:downstream_gene_variant:MODIFIER,WASH7P:ENST00000831499.1:downstream_gene_variant:MODIFIER,WASH7P:ENST00000831463.1:downstream_gene_variant:MODIFIER,WASH7P:ENST00000831292.1:downstream_gene_variant:MODIFIER,WASH7P:ENST00000831355.1:downstream_gene_variant:MODIFIER,WASH7P:ENST00000831487.1:downstream_gene_variant:MODIFIER,WASH7P:ENST00000488147.2:downstream_gene_variant:MODIFIER,WASH7P:ENST00000831582.1:downstream_gene_variant:MODIFIER',NULL,10.6,NULL,0,0,0),
(2,'chr1',16856,16856,'A','G',0.0117,'WASH7P:ENST00000831158.1:splice_donor_variant&non_coding_transcript_variant:HIGH,WASH7P:ENST00000831210.1:splice_donor_variant&non_coding_transcript_variant:HIGH,WASH7P:ENST00000831361.1:splice_region_variant&non_coding_transcript_exon_variant:LOW,WASH7P:ENST00000831289.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH7P:ENST00000831499.1:splice_donor_variant&non_coding_transcript_variant:HIGH,WASH7P:ENST00000831463.1:splice_donor_variant&non_coding_transcript_variant:HIGH,WASH7P:ENST00000831292.1:splice_region_variant&non_coding_transcript_exon_variant:LOW,WASH7P:ENST00000831355.1:splice_donor_variant&non_coding_transcript_variant:HIGH,WASH7P:ENST00000831487.1:splice_donor_variant&non_coding_transcript_variant:HIGH,WASH7P:ENST00000488147.2:splice_donor_variant&non_coding_transcript_variant:HIGH,WASH7P:ENST00000831582.1:splice_region_variant&non_coding_transcript_exon_variant:LOW,DDX11L16:ENST00000832828.1:downstream_gene_variant:MODIFIER,DDX11L1:ENST00000450305.2:downstream_gene_variant:MODIFIER,MIR6859-1:ENST00000619216.1:downstream_gene_variant:MODIFIER',NULL,20.3,NULL,0,0,0),
(3,'chr1',16957,16957,'G','T',0.01387,'WASH7P:ENST00000831158.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831210.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831361.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831289.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831499.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831463.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831292.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831355.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831487.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000488147.2:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831582.1:non_coding_transcript_exon_variant:MODIFIER,DDX11L16:ENST00000832828.1:downstream_gene_variant:MODIFIER,DDX11L1:ENST00000450305.2:downstream_gene_variant:MODIFIER,MIR6859-1:ENST00000619216.1:downstream_gene_variant:MODIFIER',NULL,12.22,NULL,0,0,0),
(4,'chr1',16996,16996,'T','C',0.03278,'WASH7P:ENST00000831158.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831210.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831361.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831289.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831499.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831463.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831292.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831355.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831487.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000488147.2:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831582.1:non_coding_transcript_exon_variant:MODIFIER,DDX11L16:ENST00000832828.1:downstream_gene_variant:MODIFIER,DDX11L1:ENST00000450305.2:downstream_gene_variant:MODIFIER,MIR6859-1:ENST00000619216.1:downstream_gene_variant:MODIFIER',NULL,0.01,NULL,0,0,0),
(5,'chr1',17379,17379,'G','A',0.00565,'WASH7P:ENST00000831158.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH7P:ENST00000831210.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH7P:ENST00000831361.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH7P:ENST00000831289.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831499.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH7P:ENST00000831463.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH7P:ENST00000831292.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH7P:ENST00000831355.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831487.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH7P:ENST00000488147.2:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH7P:ENST00000831582.1:non_coding_transcript_exon_variant:MODIFIER,MIR6859-1:ENST00000619216.1:non_coding_transcript_exon_variant:MODIFIER,DDX11L16:ENST00000832828.1:downstream_gene_variant:MODIFIER,DDX11L1:ENST00000450305.2:downstream_gene_variant:MODIFIER',NULL,8.86,NULL,0,0,0),
(6,'chr1',17407,17407,'G','A',0.01824,'WASH7P:ENST00000831158.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH7P:ENST00000831210.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH7P:ENST00000831361.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH7P:ENST00000831289.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831499.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH7P:ENST00000831463.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH7P:ENST00000831292.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH7P:ENST00000831355.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831487.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH7P:ENST00000488147.2:intron_variant&non_coding_transcript_variant:MODIFIER,WASH7P:ENST00000831582.1:non_coding_transcript_exon_variant:MODIFIER,MIR6859-1:ENST00000619216.1:non_coding_transcript_exon_variant:MODIFIER,DDX11L16:ENST00000832828.1:downstream_gene_variant:MODIFIER,DDX11L1:ENST00000450305.2:downstream_gene_variant:MODIFIER',NULL,8.95,NULL,0,0,0),
(7,'chr1',30315,30315,'G','C',0,'MIR1302-2HG:ENST00000473358.1:intron_variant&non_coding_transcript_variant:MODIFIER,MIR1302-2HG:ENST00000469289.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831158.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831210.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831361.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831289.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831499.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831463.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831292.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831355.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831487.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831582.1:upstream_gene_variant:MODIFIER,MIR1302-2HG:ENST00000834619.1:downstream_gene_variant:MODIFIER,MIR1302-2:ENST00000607096.1:upstream_gene_variant:MODIFIER,FAM138A:ENST00000834251.1:downstream_gene_variant:MODIFIER',NULL,1.87,NULL,0,0,0),
(8,'chr1',30376,30376,'C','T',0,'MIR1302-2HG:ENST00000473358.1:intron_variant&non_coding_transcript_variant:MODIFIER,MIR1302-2HG:ENST00000469289.1:non_coding_transcript_exon_variant:MODIFIER,MIR1302-2:ENST00000607096.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831158.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831210.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831361.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831289.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831499.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831463.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831292.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831355.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831487.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831582.1:upstream_gene_variant:MODIFIER,MIR1302-2HG:ENST00000834619.1:downstream_gene_variant:MODIFIER,FAM138A:ENST00000834251.1:downstream_gene_variant:MODIFIER',NULL,8.99,NULL,0,0,0),
(9,'chr1',30411,30411,'G','T',0,'MIR1302-2HG:ENST00000473358.1:intron_variant&non_coding_transcript_variant:MODIFIER,MIR1302-2HG:ENST00000469289.1:non_coding_transcript_exon_variant:MODIFIER,MIR1302-2:ENST00000607096.1:non_coding_transcript_exon_variant:MODIFIER,WASH7P:ENST00000831158.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831210.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831361.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831289.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831499.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831463.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831292.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831355.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831487.1:upstream_gene_variant:MODIFIER,WASH7P:ENST00000831582.1:upstream_gene_variant:MODIFIER,MIR1302-2HG:ENST00000834619.1:downstream_gene_variant:MODIFIER,FAM138A:ENST00000834251.1:downstream_gene_variant:MODIFIER',NULL,0.92,NULL,0,0,0),
(10,'chr1',69428,69428,'T','G',0.00567,'OR4F5:ENST00000641515.2:missense_variant:MODERATE',NULL,22.7,0,0,0,0),
(11,'chr1',182785,182785,'A','G',0.0007,'DDX11L2:ENST00000833859.1:intron_variant&non_coding_transcript_variant:MODIFIER,DDX11L2:ENST00000833862.1:intron_variant&non_coding_transcript_variant:MODIFIER,DDX11L17:ENST00000624431.2:intron_variant&non_coding_transcript_variant:MODIFIER,WASH9P:ENST00000831135.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831350.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831171.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831194.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831415.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831676.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831399.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831445.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831345.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831294.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831516.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831327.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000623083.4:downstream_gene_variant:MODIFIER',NULL,0.82,NULL,0,0,0),
(12,'chr1',182791,182791,'A','G',0.00497,'DDX11L2:ENST00000833859.1:intron_variant&non_coding_transcript_variant:MODIFIER,DDX11L2:ENST00000833862.1:intron_variant&non_coding_transcript_variant:MODIFIER,DDX11L17:ENST00000624431.2:intron_variant&non_coding_transcript_variant:MODIFIER,WASH9P:ENST00000831135.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831350.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831171.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831194.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831415.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831676.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831399.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831445.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831345.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831294.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831516.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000831327.1:downstream_gene_variant:MODIFIER,WASH9P:ENST00000623083.4:downstream_gene_variant:MODIFIER',NULL,0.51,NULL,0,0,0),
(13,'chr1',187901,187901,'G','A',0.00663,'WASH9P:ENST00000831135.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831350.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831171.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831194.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831415.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831676.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831399.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831445.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831345.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH9P:ENST00000831294.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831516.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH9P:ENST00000831327.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH9P:ENST00000623083.4:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,MIR6859-2:ENST00000612080.1:non_coding_transcript_exon_variant:MODIFIER,DDX11L2:ENST00000833859.1:downstream_gene_variant:MODIFIER,DDX11L2:ENST00000833862.1:downstream_gene_variant:MODIFIER,DDX11L17:ENST00000624431.2:downstream_gene_variant:MODIFIER',NULL,10.15,NULL,0,0,0),
(14,'chr1',187907,187907,'G','A',0.00221,'WASH9P:ENST00000831135.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831350.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831171.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831194.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831415.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831676.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831399.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831445.1:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,WASH9P:ENST00000831345.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH9P:ENST00000831294.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831516.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH9P:ENST00000831327.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH9P:ENST00000623083.4:splice_region_variant&intron_variant&non_coding_transcript_variant:LOW,MIR6859-2:ENST00000612080.1:non_coding_transcript_exon_variant:MODIFIER,DDX11L2:ENST00000833859.1:downstream_gene_variant:MODIFIER,DDX11L2:ENST00000833862.1:downstream_gene_variant:MODIFIER,DDX11L17:ENST00000624431.2:downstream_gene_variant:MODIFIER',NULL,8.64,NULL,0,0,0),
(15,'chr1',188240,188240,'G','A',0.01703,'WASH9P:ENST00000831135.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831350.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831171.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831194.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831415.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831676.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831399.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831445.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831345.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH9P:ENST00000831294.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831516.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831327.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000623083.4:non_coding_transcript_exon_variant:MODIFIER,DDX11L2:ENST00000833859.1:downstream_gene_variant:MODIFIER,DDX11L2:ENST00000833862.1:downstream_gene_variant:MODIFIER,DDX11L17:ENST00000624431.2:downstream_gene_variant:MODIFIER,MIR6859-2:ENST00000612080.1:upstream_gene_variant:MODIFIER',NULL,9.85,NULL,0,0,0),
(16,'chr1',188254,188254,'C','A',0.01633,'WASH9P:ENST00000831135.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831350.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831171.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831194.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831415.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831676.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831399.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831445.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831345.1:intron_variant&non_coding_transcript_variant:MODIFIER,WASH9P:ENST00000831294.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831516.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000831327.1:non_coding_transcript_exon_variant:MODIFIER,WASH9P:ENST00000623083.4:non_coding_transcript_exon_variant:MODIFIER,DDX11L2:ENST00000833859.1:downstream_gene_variant:MODIFIER,DDX11L2:ENST00000833862.1:downstream_gene_variant:MODIFIER,DDX11L17:ENST00000624431.2:downstream_gene_variant:MODIFIER,MIR6859-2:ENST00000612080.1:upstream_gene_variant:MODIFIER',NULL,15.04,NULL,0,0,0),
(17,'chr1',348054,348054,'G','A',NULL,'RPL23AP24:ENST00000458203.2:non_coding_transcript_exon_variant:MODIFIER',NULL,5.81,NULL,0,0,0),
(18,'chr1',348192,348192,'C','A',NULL,'RPL23AP24:ENST00000458203.2:non_coding_transcript_exon_variant:MODIFIER',NULL,6.09,NULL,0,0,0),
(19,'chr1',450750,450750,'C','T',NULL,'OR4F29:ENST00000426406.4:missense_variant:MODERATE',NULL,0.14,NULL,0,0,0),
(20,'chr1',451002,451002,'C','A',NULL,'OR4F29:ENST00000426406.4:missense_variant:MODERATE',NULL,13.11,NULL,0,0,0),
(21,'chr1',685726,685726,'C','T',NULL,'LINC00115:ENST00000419394.2:intron_variant&non_coding_transcript_variant:MODIFIER,LINC00115:ENST00000648019.1:intron_variant&non_coding_transcript_variant:MODIFIER,OR4F16:ENST00000332831.5:missense_variant:MODERATE,LINC00115:ENST00000743860.1:downstream_gene_variant:MODIFIER',NULL,0.14,0.01,0,0,0),
(22,'chr1',722941,722941,'C','A',0.00001,'LINC00115:ENST00000648019.1:intron_variant&non_coding_transcript_variant:MODIFIER,LINC00115:ENST00000743821.1:intron_variant&non_coding_transcript_variant:MODIFIER,LINC00115:ENST00000743822.1:intron_variant&non_coding_transcript_variant:MODIFIER,LINC00115:ENST00000447954.3:intron_variant&non_coding_transcript_variant:MODIFIER,LINC00115:ENST00000745229.1:intron_variant&non_coding_transcript_variant:MODIFIER,CICP3:ENST00000440782.3:non_coding_transcript_exon_variant:MODIFIER,LINC00115:ENST00000419394.2:upstream_gene_variant:MODIFIER,LINC00115:ENST00000506640.4:downstream_gene_variant:MODIFIER',NULL,13.89,NULL,0,0,0),
(23,'chr1',873939,873939,'C','T',0.00011,'FAM41C:ENST00000745843.1:intron_variant&non_coding_transcript_variant:MODIFIER,FAM41C:ENST00000745847.1:intron_variant&non_coding_transcript_variant:MODIFIER,TUBB8P11:ENST00000415481.1:non_coding_transcript_exon_variant:MODIFIER',NULL,5.88,NULL,0,0,0),
(24,'chr1',948519,948519,'T','G',0.00001,'NOC2L:ENST00000870725.1:missense_variant:MODERATE,NOC2L:ENST00000934933.1:missense_variant:MODERATE,NOC2L:ENST00000934953.1:splice_region_variant&intron_variant:LOW,NOC2L:ENST00000870737.1:missense_variant:MODERATE,NOC2L:ENST00000327044.7:missense_variant:MODERATE,NOC2L:ENST00000934941.1:missense_variant:MODERATE,NOC2L:ENST00000934940.1:missense_variant:MODERATE,NOC2L:ENST00000870729.1:missense_variant:MODERATE,NOC2L:ENST00000870730.1:missense_variant:MODERATE,NOC2L:ENST00000934939.1:missense_variant:MODERATE,NOC2L:ENST00000870728.1:missense_variant:MODERATE,NOC2L:ENST00000934938.1:missense_variant:MODERATE,NOC2L:ENST00000934937.1:missense_variant:MODERATE,NOC2L:ENST00000870727.1:missense_variant:MODERATE,NOC2L:ENST00000934935.1:missense_variant:MODERATE,NOC2L:ENST00000934936.1:missense_variant:MODERATE,NOC2L:ENST00000934934.1:missense_variant:MODERATE,NOC2L:ENST00000870726.1:missense_variant:MODERATE,NOC2L:ENST00000968816.1:missense_variant:MODERATE,NOC2L:ENST00000934942.1:missense_variant:MODERATE,NOC2L:ENST00000870731.1:missense_variant:MODERATE,NOC2L:ENST00000968805.1:missense_variant:MODERATE,NOC2L:ENST00000968806.1:missense_variant:MODERATE,NOC2L:ENST00000968807.1:missense_variant:MODERATE,NOC2L:ENST00000934932.1:missense_variant:MODERATE,NOC2L:ENST00000870734.1:missense_variant:MODERATE,NOC2L:ENST00000870736.1:intron_variant:MODIFIER,NOC2L:ENST00000870733.1:missense_variant:MODERATE,NOC2L:ENST00000968815.1:missense_variant:MODERATE,NOC2L:ENST00000968809.1:missense_variant:MODERATE,NOC2L:ENST00000968808.1:splice_region_variant&intron_variant:LOW,NOC2L:ENST00000968817.1:missense_variant:MODERATE,NOC2L:ENST00000934950.1:missense_variant:MODERATE,NOC2L:ENST00000934951.1:missense_variant:MODERATE,NOC2L:ENST00000934952.1:missense_variant:MODERATE,NOC2L:ENST00000934944.1:missense_variant:MODERATE,NOC2L:ENST00000968811.1:missense_variant:MODERATE,NOC2L:ENST00000870732.1:missense_variant:MODERATE,NOC2L:ENST00000968820.1:missense_variant:MODERATE,NOC2L:ENST00000968821.1:missense_variant:MODERATE,NOC2L:ENST00000870738.1:missense_variant:MODERATE,NOC2L:ENST00000934956.1:missense_variant:MODERATE,NOC2L:ENST00000968822.1:missense_variant:MODERATE,NOC2L:ENST00000968823.1:intron_variant:MODIFIER,NOC2L:ENST00000934943.1:missense_variant:MODERATE,NOC2L:ENST00000968810.1:missense_variant:MODERATE,NOC2L:ENST00000934954.1:missense_variant:MODERATE,NOC2L:ENST00000870739.1:missense_variant:MODERATE,NOC2L:ENST00000934955.1:missense_variant:MODERATE,NOC2L:ENST00000968818.1:missense_variant:MODERATE,NOC2L:ENST00000934948.1:missense_variant:MODERATE,NOC2L:ENST00000968814.1:missense_variant:MODERATE,NOC2L:ENST00000870735.1:missense_variant:MODERATE,NOC2L:ENST00000934947.1:missense_variant:MODERATE,NOC2L:ENST00000934946.1:missense_variant:MODERATE,NOC2L:ENST00000968819.1:missense_variant:MODERATE,NOC2L:ENST00000934949.1:missense_variant:MODERATE,NOC2L:ENST00000968813.1:missense_variant:MODERATE,NOC2L:ENST00000934958.1:missense_variant:MODERATE,NOC2L:ENST00000934945.1:missense_variant:MODERATE,NOC2L:ENST00000968812.1:missense_variant:MODERATE,NOC2L:ENST00000968824.1:missense_variant:MODERATE,NOC2L:ENST00000934959.1:missense_variant:MODERATE,NOC2L:ENST00000934957.1:missense_variant:MODERATE,SAMD11:ENST00000968544.1:downstream_gene_variant:MODIFIER,SAMD11:ENST00000616016.5:downstream_gene_variant:MODIFIER,SAMD11:ENST00000618323.5:downstream_gene_variant:MODIFIER,SAMD11:ENST00000968542.1:downstream_gene_variant:MODIFIER,SAMD11:ENST00000968543.1:downstream_gene_variant:MODIFIER,SAMD11:ENST00000342066.8:downstream_gene_variant:MODIFIER',NULL,22.9,0,0,0,0),
(25,'chr1',948711,948711,'C','G',0.03328,'NOC2L:ENST00000870725.1:intron_variant:MODIFIER,NOC2L:ENST00000934933.1:intron_variant:MODIFIER,NOC2L:ENST00000934953.1:intron_variant:MODIFIER,NOC2L:ENST00000870737.1:intron_variant:MODIFIER,NOC2L:ENST00000327044.7:intron_variant:MODIFIER,NOC2L:ENST00000934941.1:intron_variant:MODIFIER,NOC2L:ENST00000934940.1:intron_variant:MODIFIER,NOC2L:ENST00000870729.1:intron_variant:MODIFIER,NOC2L:ENST00000870730.1:intron_variant:MODIFIER,NOC2L:ENST00000934939.1:intron_variant:MODIFIER,NOC2L:ENST00000870728.1:intron_variant:MODIFIER,NOC2L:ENST00000934938.1:intron_variant:MODIFIER,NOC2L:ENST00000934937.1:intron_variant:MODIFIER,NOC2L:ENST00000870727.1:intron_variant:MODIFIER,NOC2L:ENST00000934935.1:intron_variant:MODIFIER,NOC2L:ENST00000934936.1:intron_variant:MODIFIER,NOC2L:ENST00000934934.1:intron_variant:MODIFIER,NOC2L:ENST00000870726.1:intron_variant:MODIFIER,NOC2L:ENST00000968816.1:intron_variant:MODIFIER,NOC2L:ENST00000934942.1:intron_variant:MODIFIER,NOC2L:ENST00000870731.1:intron_variant:MODIFIER,NOC2L:ENST00000968805.1:intron_variant:MODIFIER,NOC2L:ENST00000968806.1:intron_variant:MODIFIER,NOC2L:ENST00000968807.1:intron_variant:MODIFIER,NOC2L:ENST00000934932.1:intron_variant:MODIFIER,NOC2L:ENST00000870734.1:intron_variant:MODIFIER,NOC2L:ENST00000870736.1:intron_variant:MODIFIER,NOC2L:ENST00000870733.1:intron_variant:MODIFIER,NOC2L:ENST00000968815.1:intron_variant:MODIFIER,NOC2L:ENST00000968809.1:intron_variant:MODIFIER,NOC2L:ENST00000968808.1:intron_variant:MODIFIER,NOC2L:ENST00000968817.1:intron_variant:MODIFIER,NOC2L:ENST00000934950.1:intron_variant:MODIFIER,NOC2L:ENST00000934951.1:intron_variant:MODIFIER,NOC2L:ENST00000934952.1:intron_variant:MODIFIER,NOC2L:ENST00000934944.1:intron_variant:MODIFIER,NOC2L:ENST00000968811.1:intron_variant:MODIFIER,NOC2L:ENST00000870732.1:intron_variant:MODIFIER,NOC2L:ENST00000968820.1:intron_variant:MODIFIER,NOC2L:ENST00000968821.1:intron_variant:MODIFIER,NOC2L:ENST00000870738.1:intron_variant:MODIFIER,NOC2L:ENST00000934956.1:intron_variant:MODIFIER,NOC2L:ENST00000968822.1:intron_variant:MODIFIER,NOC2L:ENST00000968823.1:intron_variant:MODIFIER,NOC2L:ENST00000934943.1:intron_variant:MODIFIER,NOC2L:ENST00000968810.1:intron_variant:MODIFIER,NOC2L:ENST00000934954.1:intron_variant:MODIFIER,NOC2L:ENST00000870739.1:intron_variant:MODIFIER,NOC2L:ENST00000934955.1:intron_variant:MODIFIER,NOC2L:ENST00000968818.1:intron_variant:MODIFIER,NOC2L:ENST00000934948.1:intron_variant:MODIFIER,NOC2L:ENST00000968814.1:intron_variant:MODIFIER,NOC2L:ENST00000870735.1:intron_variant:MODIFIER,NOC2L:ENST00000934947.1:intron_variant:MODIFIER,NOC2L:ENST00000934946.1:intron_variant:MODIFIER,NOC2L:ENST00000968819.1:intron_variant:MODIFIER,NOC2L:ENST00000934949.1:intron_variant:MODIFIER,NOC2L:ENST00000968813.1:intron_variant:MODIFIER,NOC2L:ENST00000934958.1:intron_variant:MODIFIER,NOC2L:ENST00000934945.1:intron_variant:MODIFIER,NOC2L:ENST00000968812.1:intron_variant:MODIFIER,NOC2L:ENST00000968824.1:intron_variant:MODIFIER,NOC2L:ENST00000934959.1:intron_variant:MODIFIER,NOC2L:ENST00000934957.1:intron_variant:MODIFIER,SAMD11:ENST00000968544.1:downstream_gene_variant:MODIFIER,SAMD11:ENST00000616016.5:downstream_gene_variant:MODIFIER,SAMD11:ENST00000618323.5:downstream_gene_variant:MODIFIER,SAMD11:ENST00000968542.1:downstream_gene_variant:MODIFIER,SAMD11:ENST00000968543.1:downstream_gene_variant:MODIFIER,SAMD11:ENST00000342066.8:downstream_gene_variant:MODIFIER',NULL,0.3,0,0,0,0),
(26,'chr1',948721,948721,'A','C',0.02267,'NOC2L:ENST00000870725.1:intron_variant:MODIFIER,NOC2L:ENST00000934933.1:intron_variant:MODIFIER,NOC2L:ENST00000934953.1:intron_variant:MODIFIER,NOC2L:ENST00000870737.1:intron_variant:MODIFIER,NOC2L:ENST00000327044.7:intron_variant:MODIFIER,NOC2L:ENST00000934941.1:intron_variant:MODIFIER,NOC2L:ENST00000934940.1:intron_variant:MODIFIER,NOC2L:ENST00000870729.1:intron_variant:MODIFIER,NOC2L:ENST00000870730.1:intron_variant:MODIFIER,NOC2L:ENST00000934939.1:intron_variant:MODIFIER,NOC2L:ENST00000870728.1:intron_variant:MODIFIER,NOC2L:ENST00000934938.1:intron_variant:MODIFIER,NOC2L:ENST00000934937.1:intron_variant:MODIFIER,NOC2L:ENST00000870727.1:intron_variant:MODIFIER,NOC2L:ENST00000934935.1:intron_variant:MODIFIER,NOC2L:ENST00000934936.1:intron_variant:MODIFIER,NOC2L:ENST00000934934.1:intron_variant:MODIFIER,NOC2L:ENST00000870726.1:intron_variant:MODIFIER,NOC2L:ENST00000968816.1:intron_variant:MODIFIER,NOC2L:ENST00000934942.1:intron_variant:MODIFIER,NOC2L:ENST00000870731.1:intron_variant:MODIFIER,NOC2L:ENST00000968805.1:intron_variant:MODIFIER,NOC2L:ENST00000968806.1:intron_variant:MODIFIER,NOC2L:ENST00000968807.1:intron_variant:MODIFIER,NOC2L:ENST00000934932.1:intron_variant:MODIFIER,NOC2L:ENST00000870734.1:intron_variant:MODIFIER,NOC2L:ENST00000870736.1:intron_variant:MODIFIER,NOC2L:ENST00000870733.1:intron_variant:MODIFIER,NOC2L:ENST00000968815.1:intron_variant:MODIFIER,NOC2L:ENST00000968809.1:intron_variant:MODIFIER,NOC2L:ENST00000968808.1:intron_variant:MODIFIER,NOC2L:ENST00000968817.1:intron_variant:MODIFIER,NOC2L:ENST00000934950.1:intron_variant:MODIFIER,NOC2L:ENST00000934951.1:intron_variant:MODIFIER,NOC2L:ENST00000934952.1:intron_variant:MODIFIER,NOC2L:ENST00000934944.1:intron_variant:MODIFIER,NOC2L:ENST00000968811.1:intron_variant:MODIFIER,NOC2L:ENST00000870732.1:intron_variant:MODIFIER,NOC2L:ENST00000968820.1:intron_variant:MODIFIER,NOC2L:ENST00000968821.1:intron_variant:MODIFIER,NOC2L:ENST00000870738.1:intron_variant:MODIFIER,NOC2L:ENST00000934956.1:intron_variant:MODIFIER,NOC2L:ENST00000968822.1:intron_variant:MODIFIER,NOC2L:ENST00000968823.1:intron_variant:MODIFIER,NOC2L:ENST00000934943.1:intron_variant:MODIFIER,NOC2L:ENST00000968810.1:intron_variant:MODIFIER,NOC2L:ENST00000934954.1:intron_variant:MODIFIER,NOC2L:ENST00000870739.1:intron_variant:MODIFIER,NOC2L:ENST00000934955.1:intron_variant:MODIFIER,NOC2L:ENST00000968818.1:intron_variant:MODIFIER,NOC2L:ENST00000934948.1:intron_variant:MODIFIER,NOC2L:ENST00000968814.1:intron_variant:MODIFIER,NOC2L:ENST00000870735.1:intron_variant:MODIFIER,NOC2L:ENST00000934947.1:intron_variant:MODIFIER,NOC2L:ENST00000934946.1:intron_variant:MODIFIER,NOC2L:ENST00000968819.1:intron_variant:MODIFIER,NOC2L:ENST00000934949.1:intron_variant:MODIFIER,NOC2L:ENST00000968813.1:intron_variant:MODIFIER,NOC2L:ENST00000934958.1:intron_variant:MODIFIER,NOC2L:ENST00000934945.1:intron_variant:MODIFIER,NOC2L:ENST00000968812.1:intron_variant:MODIFIER,NOC2L:ENST00000968824.1:intron_variant:MODIFIER,NOC2L:ENST00000934959.1:intron_variant:MODIFIER,NOC2L:ENST00000934957.1:intron_variant:MODIFIER,SAMD11:ENST00000968544.1:downstream_gene_variant:MODIFIER,SAMD11:ENST00000616016.5:downstream_gene_variant:MODIFIER,SAMD11:ENST00000618323.5:downstream_gene_variant:MODIFIER,SAMD11:ENST00000968542.1:downstream_gene_variant:MODIFIER,SAMD11:ENST00000968543.1:downstream_gene_variant:MODIFIER,SAMD11:ENST00000342066.8:downstream_gene_variant:MODIFIER',NULL,4.42,0,0,0,0);

INSERT INTO `detected_variant` (`processed_sample_id`,`variant_id`,`genotype`,`mosaic`) VALUES 
(1,1,'het',0),
(1,2,'het',0),
(1,3,'het',0),
(1,4,'het',0),
(1,5,'het',0),
(1,6,'het',0),
(1,7,'het',0),
(1,8,'het',0),
(1,9,'het',0),
(1,10,'het',0),
(1,11,'hom',0),
(1,12,'hom',0),
(1,13,'het',0),
(1,14,'het',0),
(1,15,'het',0),
(1,16,'het',0),
(1,17,'het',0),
(1,18,'het',0),
(1,19,'het',0),
(1,20,'het',0),
(1,21,'het',0),
(1,22,'het',0),
(1,23,'het',0),
(1,24,'het',0),
(1,25,'het',0),
(1,26,'het',0),
(2,1,'het',0),
(2,2,'het',0),
(2,3,'het',0),
(2,4,'het',0),
(2,5,'het',0),
(2,6,'het',0),
(2,7,'het',0),
(2,8,'het',0),
(2,9,'het',0),
(2,10,'het',0),
(2,11,'hom',0),
(2,12,'hom',0),
(2,13,'het',0),
(2,14,'het',0),
(2,15,'het',0),
(2,16,'het',0),
(2,17,'het',0),
(2,18,'het',0),
(2,19,'het',0),
(2,21,'het',0),
(2,22,'het',0),
(2,24,'het',0),
(2,25,'het',0);

-- CNVs
INSERT INTO `cnv_callset` (`id`,`processed_sample_id`,`caller`,`caller_version`,`call_date`,`quality_metrics`,`quality`) VALUES 
(1,1,'ClinCNV','v1.18.3','2023-05-10','{\"fraction of outliers\":\"0.04\",\"gender of sample\":\"F\",\"high-quality cnvs\":\"72\",\"mean correlation to reference samples\":\"0.894\",\"number of iterations\":\"1\",\"quality used at final iteration\":\"20\",\"was it outlier after clustering\":\"\"}','n/a'),
(2,2,'ClinCNV','v1.18.3','2023-05-10','{\"fraction of outliers\":\"0.04\",\"gender of sample\":\"F\",\"high-quality cnvs\":\"72\",\"mean correlation to reference samples\":\"0.894\",\"number of iterations\":\"1\",\"quality used at final iteration\":\"20\",\"was it outlier after clustering\":\"\"}','n/a');

INSERT INTO `cnv` (`id`, `cnv_callset_id`, `chr`, `start`, `end`, `cn`, `quality_metrics`) VALUES
(1, 1, 'chr1', 1179636, 1179757, 3, ''),
(2, 1, 'chr1', 1765207, 1787457, 3, ''),
(3, 1, 'chr1', 1332003, 1332826, 3, ''),
(4, 1, 'chr1', 3836474, 3836712, 1, ''),
(5, 1, 'chr1', 9871166, 9871297, 3, '');

--SVs
INSERT INTO `sv_callset` (`id`,`processed_sample_id`,`caller`,`caller_version`,`call_date`) VALUES 
(1,1,'Manta','1.6.0','2023-05-10'),
(2,2,'Manta','1.6.0','2023-05-10');

INSERT INTO `sv_deletion` (`id`,`sv_callset_id`,`chr`,`start_min`,`start_max`,`end_min`,`end_max`,`genotype`,`quality_metrics`) VALUES 
(1,1,'chr1',1968935,1968935,1969006,1969006,'het','{\"filter\":\"SampleFT;off-target\",\"quality\":\"277\"}'),
(2,1,'chr1',2859237,2859242,2859315,2859315,'het','{\"filter\":\"off-target\",\"quality\":\"216\"}'),
(3,1,'chr1',7829912,7829992,7829966,7829966,'hom','{\"filter\":\"PASS\",\"quality\":\"843\"}'),
(4,1,'chr1',26345031,26345031,26345084,26345084,'het','{\"filter\":\"PASS\",\"quality\":\"999\"}'),
(5,1,'chr1',152305218,152305384,152306156,152306333,'het','{\"filter\":\"PASS\",\"quality\":\"999\"}'),
(6,1,'chr1',241674876,241674876,241675027,241675027,'hom','{\"filter\":\"off-target\",\"quality\":\"246\"}'),
(7,1,'chr2',11538609,11538637,11538661,11538661,'hom','{\"filter\":\"SampleFT;off-target\",\"quality\":\"101\"}'),
(8,2,'chr1',1968935,1968935,1969006,1969006,'het','{\"filter\":\"SampleFT;off-target\",\"quality\":\"277\"}'),
(9,2,'chr1',2859237,2859242,2859315,2859315,'het','{\"filter\":\"off-target\",\"quality\":\"216\"}'),
(10,2,'chr1',7829912,7829992,7829968,7829968,'hom','{\"filter\":\"PASS\",\"quality\":\"843\"}'),
(11,2,'chr1',26344831,26345131,26345084,26345084,'het','{\"filter\":\"PASS\",\"quality\":\"999\"}');

INSERT INTO `sv_duplication` (`id`,`sv_callset_id`,`chr`,`start_min`,`start_max`,`end_min`,`end_max`,`genotype`,`quality_metrics`) VALUES 
(1,1,'chr1',1948974,1948993,1949086,1949086,'het','{\"filter\":\"SampleFT;off-target\",\"quality\":\"94\"}'),
(2,1,'chr1',29394195,29394326,30405719,30405850,'hom','{\"filter\":\"off-target\",\"quality\":\"86\"}'),
(3,1,'chr1',152305127,152305218,152306080,152306296,'het','{\"filter\":\"PASS\",\"quality\":\"528\"}'),
(4,1,'chr2',909702,909827,910075,910225,'het','{\"filter\":\"PASS\",\"quality\":\"491\"}'),
(5,2,'chr1',1948974,1948993,1949086,1949086,'het','{\"filter\":\"SampleFT;off-target\",\"quality\":\"94\"}'),
(6,2,'chr1',29394195,29394326,30405719,30405850,'hom','{\"filter\":\"off-target\",\"quality\":\"86\"}');

INSERT INTO `sv_insertion` (`id`,`sv_callset_id`,`chr`,`pos`,`ci_lower`,`ci_upper`,`inserted_sequence`,`known_left`,`known_right`,`genotype`,`quality_metrics`) VALUES 
(1,1,'chr1',54458803,0,70,NULL,'AAGGAGGGAAGGAAGGAATGTGGGCAGGTGGGAAGGAGGGAGGGAGGGAAGGAAAGAAGGAAGAAAGGAAGAAAGGAAGGAAGGAGGGAGGGAAGGAAGGAAGAAAGGAAGGAAGGAAGGTGGGCAGGCAGGAAGGAGGGAGGGAGGG','AAGGAGGGAAGGAAGGAAGGTGGGCAGGCAGGAAGGAGGGAGGGTGGGAGGGAGGGAAGGAAGGAAGGGAGGGAGGGAAGGAAAGAAGGAAGGGAGGAAGGAAGGAGGGAGGGT','hom','{\"filter\":\"off-target\",\"quality\":\"304\"}'),
(2,1,'chr2',28973386,0,1,'TCCCTTCCTTCCTTCTTTCCTCCCTTCCTTCCCTTCCTTCTTTGCTTCTTTCCTCC',NULL,NULL,'hom','{\"filter\":\"SampleFT;off-target\",\"quality\":\"48\"}'),
(3,2,'chr1',54458803,0,70,NULL,'AAGGAGGGAAGGAAGGAATGTGGGCAGGTGGGAAGGAGGGAGGGAGGGAAGGAAAGAAGGAAGAAAGGAAGAAAGGAAGGAAGGAGGGAGGGAAGGAAGGAAGAAAGGAAGGAAGGAAGGTGGGCAGGCAGGAAGGAGGGAGGGAGGG','AAGGAGGGAAGGAAGGAAGGTGGGCAGGCAGGAAGGAGGGAGGGTGGGAGGGAGGGAAGGAAGGAAGGGAGGGAGGGAAGGAAAGAAGGAAGGGAGGAAGGAAGGAGGGAGGGT','hom','{\"filter\":\"off-target\",\"quality\":\"304\"}');

INSERT INTO `sv_inversion` (`id`,`sv_callset_id`,`chr`,`start_min`,`start_max`,`end_min`,`end_max`,`genotype`,`quality_metrics`) VALUES 
(1,1,'chr1',30405556,30405684,30405862,30405990,'het','{\"filter\":\"off-target\",\"quality\":\"33\"}'),
(2,1,'chr1',92796683,92796756,92796717,92796847,'hom','{\"filter\":\"SampleFT;off-target\",\"quality\":\"20\"}'),
(3,1,'chr2',88860605,88860605,88886154,88886154,'het','{\"filter\":\"PASS\",\"quality\":\"982\"}'),
(4,1,'chr2',88860605,88860605,90210528,90210528,'het','{\"filter\":\"PASS\",\"quality\":\"999\"}'),
(5,1,'chr2',88861923,88861923,88897786,88897786,'hom','{\"filter\":\"PASS\",\"quality\":\"582\"}'),
(6,2,'chr1',30405556,30405684,30405862,30405990,'het','{\"filter\":\"off-target\",\"quality\":\"33\"}'),
(7,2,'chr1',92796683,92796756,92796717,92796847,'hom','{\"filter\":\"SampleFT;off-target\",\"quality\":\"20\"}');

INSERT INTO `sv_translocation` (`id`,`sv_callset_id`,`chr1`,`start1`,`end1`,`chr2`,`start2`,`end2`,`genotype`,`quality_metrics`) VALUES 
(1,1,'chr1',9061390,9061394,'chr14',93246136,93246140,'het','{\"filter\":\"AMBIGUOUS\",\"quality\":\"79\"}'),
(2,1,'chr1',109108013,109108013,'chr22',29767384,29767384,'het','{\"filter\":\"PASS\",\"quality\":\"999\"}'),
(3,1,'chr1',246700009,246700015,'chr18',76649051,76649057,'het','{\"filter\":\"MinQUAL;SampleFT;off-target\",\"quality\":\"13\"}'),
(4,1,'chr2',2952634,2952639,'chrX',86441544,86441549,'het','{\"filter\":\"MinQUAL;SampleFT;off-target\",\"quality\":\"10\"}'),
(5,1,'chr2',27283787,27283812,'chr3',84748882,84748907,'het','{\"filter\":\"SampleFT;off-target\",\"quality\":\"23\"}'),
(6,1,'chr2',32916190,32916257,'chr17',79115663,79115741,'het','{\"filter\":\"AMBIGUOUS\",\"quality\":\"16\"}'),
(7,1,'chr2',97158071,97158071,'chr10',85355494,85355494,'het','{\"filter\":\"off-target\",\"quality\":\"805\"}'),
(8,2,'chr1',9061390,9061394,'chr14',93246136,93246140,'het','{\"filter\":\"AMBIGUOUS\",\"quality\":\"79\"}');


--REs
INSERT INTO `re_callset` (`id`,`processed_sample_id`,`caller`,`caller_version`,`call_date`) VALUES 
(1,1,'ExpansionHunter','v5.0.0','2023-05-10'),
(2,2,'ExpansionHunter','v5.0.0','2023-05-10');

INSERT INTO `repeat_expansion_genotype` (`id`,`processed_sample_id`,`repeat_expansion_id`,`allele1`,`allele2`,`filter`) VALUES 
(1,1,26,13,18,NULL),
(2,1,33,14,14,'LowDepth'),
(3,1,6,10,12,NULL),
(4,1,55,4,4,NULL),
(5,1,15,16,25,NULL),
(6,1,59,9,12,NULL),
(7,1,25,20,20,NULL),
(8,1,17,10,14,NULL),
(9,1,3,30,31,NULL),
(10,1,19,37,36,NULL),
(11,1,70,11,11,'LowDepth'),
(12,1,2,19,19,NULL),
(13,1,23,7,16,NULL),
(14,2,26,12,18,NULL),
(15,2,33,14,14,'LowDepth'),
(16,2,6,10,14,NULL),
(17,2,55,4,4,NULL),
(18,2,15,16,26,NULL),
(19,2,59,9,12,NULL);


--report_config
INSERT INTO `report_configuration`(`id`, `processed_sample_id`, `created_by`, `created_date`, `last_edit_by`, `last_edit_date`, `finalized_by`, `finalized_date`) VALUES
(1,1,100,'2000-01-01 11:11:11',101,'2020-01-01 22:22:22',NULL,NULL);

INSERT INTO `report_configuration_variant` (`id`, `report_configuration_id`, `variant_id`, `type`, `causal`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES
(1, 1, 3, 'diagnostic variant', 1, 'n/a', 0, 0, 0, 0, 0, 0, 0, 0, 'exact match', ''),
(2, 1, 6, 'diagnostic variant', 1, 'n/a', 0, 0, 0, 0, 0, 0, 0, 0, 'exact match', ''),
(3, 1, 9, 'diagnostic variant', 1, 'n/a', 0, 0, 0, 0, 0, 0, 1, 0, 'exact match (excluded)', ''),
(4, 1, 20, 'diagnostic variant', 1, 'n/a', 0, 0, 0, 0, 0, 0, 0, 0, 'missed not excluded', ''),
(5, 1, 23, 'diagnostic variant', 1, 'n/a', 0, 0, 0, 0, 0, 0, 0, 0, 'missed not excluded', ''),
(6, 1, 26, 'diagnostic variant', 1, 'n/a', 0, 0, 0, 1, 0, 0, 0, 0, 'missed excluded', '');

INSERT INTO `report_configuration_cnv` (`id`, `report_configuration_id`, `cnv_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES
(1, 1, 1, 'diagnostic variant', 0, 3, 'AD', 0, 0, 0, 0, 0, 0, 0, 0, 'exact match', ''),
(2, 1, 2, 'diagnostic variant', 0, 3, 'AD', 0, 0, 0, 0, 0, 0, 0, 0, 'partial match (<90%)', ''),
(3, 1, 3, 'diagnostic variant', 0, 3, 'AD', 0, 0, 0, 0, 0, 0, 0, 0, 'partial match (>90%)', ''),
(4, 1, 4, 'diagnostic variant', 0, 3, 'AD', 0, 0, 0, 0, 0, 0, 0, 0, 'missed not excluded', ''),
(5, 1, 5, 'diagnostic variant', 0, 3, 'AD', 0, 0, 0, 0, 0, 1, 0, 0, 'missed excluded', '');

INSERT INTO `report_configuration_sv` (`id`, `report_configuration_id`, `sv_deletion_id`, `sv_duplication_id`, `sv_insertion_id`, `sv_inversion_id`, `sv_translocation_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_hit2_missing`, `exclude_gus`, `exclude_used_other_var_type`, `exclude_other`, `comments`, `comments2`, `rna_info`, `manual_hgvs_type_bnd`) VALUES
(1, 1, NULL, 1, NULL, NULL, NULL, 'diagnostic variant', 1, 5, 'AR', 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'exact match', '', 'n/a', NULL),
(2, 1, NULL, NULL, NULL, NULL, 1, 'candidate variant', 1, 5, 'AR', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'exact match with manual HGVS BND', '', 'n/a', 'some manual HGVS BND description'),
(3, 1, 4, NULL, NULL, NULL, NULL, 'candidate variant', 1, 5, 'AR', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'match via CI overlap', '', 'n/a', NULL),
(4, 1, 3, NULL, NULL, NULL, NULL, 'candidate variant', 1, 5, 'AR', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'no CI overlap', '', 'n/a', NULL),
(5, 1, NULL, NULL, 2, NULL, NULL, 'candidate variant', 1, 5, 'AR', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'missed', '', 'n/a', NULL),
(6, 1, NULL, NULL, NULL, 5, NULL, 'candidate variant', 1, 5, 'AR', 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 'missed excluded', '', 'n/a', NULL);

INSERT INTO `report_configuration_re` (`id`, `report_configuration_id`, `repeat_expansion_genotype_id`, `type`, `causal`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_phenotype`, `exclude_other`, `comments`, `comments2`, `manual_allele1`, `manual_allele2`) VALUES
(1, 1, 6, 'incidental finding', 1, 'MT', 1, 1, 0, 0, 0, 0, 'exact match', '', NULL, NULL),
(2, 1, 1, 'incidental finding', 1, 'AD', 1, 0, 0, 0, 0, 0, 'smaller allele changes', '', NULL, NULL),
(3, 1, 5, 'incidental finding', 1, 'XLR', 0, 0, 0, 0, 0, 0, 'larger allele changes in allowed range', '', NULL, NULL),
(4, 1, 3, 'incidental finding', 1, 'MT', 0, 0, 0, 0, 0, 0, 'larger allele changes too much', '', NULL, NULL),
(5, 1, 10, 'incidental finding', 1, 'MT', 0, 0, 0, 0, 0, 0, 'missed', '', NULL, NULL),
(6, 1, 9, 'incidental finding', 1, 'n/a', 0, 1, 0, 0, 1, 0, 'missed excluded', '', NULL, NULL);

INSERT INTO `report_configuration_other_causal_variant` (`id`, `report_configuration_id`, `coordinates`, `gene`, `type`, `inheritance`, `comment`, `comment_reviewer1`, `comment_reviewer2`) VALUES
(1, 1, 'chr2:123456-789012', 'EPRS', 'uncalled CNV', 'AR', 'This is a comment!', 'Reviewer1: \n This is a comment!\n', 'Reviewer2: \n This is not a comment!\n');

