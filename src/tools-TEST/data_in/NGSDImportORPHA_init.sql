
INSERT INTO `gene` (`id`, `hgnc_id`, `symbol`, `name`, `type`) VALUES
(418546, 4944, 'HLA-DQB1', 'major histocompatibility complex, class II, DQ beta 1', 'protein-coding gene'),
(418551, 4948, 'HLA-DRB1', 'major histocompatibility complex, class II, DR beta 1', 'protein-coding gene'),
(420778, 30497, 'KIF7', 'kinesin family member 7', 'protein-coding gene'),
(458452, 4162, 'GARS1', 'glycyl-tRNA synthetase 1', 'protein-coding gene');

INSERT INTO `gene_alias`(`gene_id`, `symbol`, `type`) VALUES 
(458452, 'GARS', 'previous');

