
-- table `gene`
INSERT INTO `gene` (`id`, `hgnc_id`, `symbol`, `name`, `chromosome`, `type`) VALUES
(1, 1000, 'PARK2', 'parkin RBR E3 ubiquitin protein ligase', '6', 'protein-coding gene'),
(2, 2000, 'LRRK2', 'leucine-rich repeat kinase 2', '12', 'protein-coding gene'),
(3, 3000, 'GBA', 'glucosidase, beta, acid', '1', 'protein-coding gene');

-- table `gene_transcript`
INSERT INTO `gene_transcript` (`id`, `gene_id`, `name`, `source`, `start_coding`, `end_coding`, `strand`) VALUES
(1, 1, 'CCDS5281.1', 'ccds', 161771131, 163148700, '-'),
(2, 2, 'CCDS31774.1', 'ccds', 40618934, 40761567, '+'),
(3, 3, 'CCDS53374.1', 'ccds', 155204786, 155210903, '-');