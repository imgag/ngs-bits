
INSERT INTO `gene`(`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES (1,1001,'BRCA1','Breast cancer associated gene 1', 'protein-coding gene');
INSERT INTO `gene`(`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES (2,1002,'BRCA2','Breast cancer associated gene 2', 'protein-coding gene');
INSERT INTO `gene`(`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES (3,1003, 'OR4F5', 'olfactory receptor family 4 subfamily F member 5', 'protein-coding gene');

INSERT INTO `geneinfo_germline`(`symbol`, `inheritance`, `exac_pli`, `comments`) VALUES ('BRCA1', 'AD', 0.00, '');
INSERT INTO `geneinfo_germline`(`symbol`, `inheritance`, `exac_pli`, `comments`) VALUES ('BRCA2', 'AD', NULL, '');





