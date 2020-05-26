

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
(11,'HP:0001423','X-linked dominant inheritance','def','');

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
