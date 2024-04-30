
-- -----------------------------------------------------
-- Table `user`
-- -----------------------------------------------------
INSERT INTO user VALUES
(NULL, 'admin', 'dd94709528bb1c83d08f3088d4043f4742891f4f', 'admin', 'Admin','no_valid@email.de', NOW(), NULL, 1, NULL, NULL),
(NULL, 'genlab_import', '', 'special', 'GenLab import','no_valid@email2.de', NOW(), NULL, 1, NULL, NULL),
(NULL, 'unknown', '', 'special', 'Unknown user','no_valid@email3.de', NOW(), NULL, 1, NULL, NULL);


-- -----------------------------------------------------
-- Table `species`
-- -----------------------------------------------------
INSERT INTO species VALUES
(NULL, 'human');


-- -----------------------------------------------------
-- Table `genome`
-- -----------------------------------------------------
INSERT INTO genome VALUES
(NULL, 'GRCh37', 'Human genome GRCh37 (hg19)'),
(NULL, 'GRCh38', 'Human genome GRCh38 (hg38)');


-- -----------------------------------------------------
-- Table `repeat_expansion`
-- -----------------------------------------------------
INSERT INTO `repeat_expansion` (name, region, repeat_unit, max_normal, min_pathogenic, inheritance, disease_names, disease_ids_omim, hpo_terms, location, type) VALUES
('AR', 'chrX:67545316-67545385', 'GCA', '34', '38', 'XLR', 'SMAX1', '313200', 'HP:0003690, HP:0001283, HP:0002380, HP:0000771', 'Coding', 'diagnostic - in-house testing'),
('ATN1', 'chr12:6936716-6936773', 'CAG', '35', '48', 'AD', 'DRPLA', '125370', 'HP:0012758, HP:0001249, HP:0001252, HP:0001290, HP:0008947, HP:0011968, HP:0002104, HP:0001250, HP:0200134, HP:0001298, HP:0000505, HP:0001141, HP:0007663, HP:0000486, HP:0000540, HP:0000365, HP:0001155, HP:0001760', 'Coding', 'diagnostic - in-house testing'),
('ATXN1', 'chr6:16327633-16327723', 'TGC', '35', '40', 'AD', 'SCA1', '164400', 'HP:0001251, HP:0001288, HP:0001260, HP:0002015, HP:0000570, HP:0000639', 'Coding', 'diagnostic - in-house testing'),
('ATXN2', 'chr12:111598949-111599018', 'GCT', '31', '33', 'AD', 'SCA2', '183090', 'HP:0001251, HP:0009830, HP:0000639, HP:0000570, HP:0007256, HP:0001300, HP:0001260, HP:0002072, HP:0001332, HP:0002380, HP:0007373', 'Coding', 'diagnostic - in-house testing'),
('ATXN3', 'chr14:92071010-92071040', 'CTG', '44', '60', 'AD', 'MJD,SCA3', '109150', 'HP:0001251, HP:0002066, HP:0007256, HP:0003202, HP:0001284, HP:0001347, HP:0000639, HP:0000570, HP:0001756, HP:0001260, HP:0007373, HP:0001300, HP:0001347', 'Coding', 'diagnostic - in-house testing'),
('ATXN7', 'chr3:63912684-63912714', 'GCA', '27', '35', 'AD', 'SCA7, OPCA', '164500', 'HP:0001251, HP:0000608, HP:0000006', 'Coding', 'diagnostic - in-house testing'),
('ATXN8OS_CTA', 'chr13:70139353-70139383', 'CTA', '50', '54', 'AD', 'SCA8', '608768', 'HP:0001251, HP:0002066, HP:0001260, HP:0001347, HP:0000639, HP:0000570, HP:0100543, HP:0007256, HP:0000763, HP:0001257', '3\' UTR', 'diagnostic - in-house testing'),
('ATXN8OS', 'chr13:70139383-70139428', 'CTG', '50', '54', 'AD', 'SCA8', '608768', 'HP:0001251, HP:0002066, HP:0001260, HP:0001347, HP:0000639, HP:0000570, HP:0100543, HP:0007256, HP:0000763, HP:0001257', '3\' UTR', 'diagnostic - in-house testing'),
('ATXN10', 'chr22:45795354-45795424', 'ATTCT', '32', '801', 'AD', 'SCA10', '603516', 'HP:0001251, HP:0002070, HP:0002066, HP:0002141, HP:0001260, HP:0002015, HP:0001250, HP:0020219, HP:0000639, HP:0001272, HP:0000496', 'Intronic', 'diagnostic - in-house testing'),
('C9ORF72', 'chr9:27573528-27573546', 'GGCCCC', '24', '61', 'AD', 'FTD, ALS1', '105550', 'HP:0007373, HP:0032928, HP:0002380, HP:0002145, HP:0001324, HP:0003202, HP:0033051, HP:0002354, HP:0002186, HP:0001300', 'Intronic', 'diagnostic - in-house testing'),
('CACNA1A', 'chr19:13207858-13207897', 'CTG', '18', '20', 'AD', 'SCA6', '183086', 'HP:0001251, HP:0002066, HP:0002141, HP:0001260, HP:0002015, HP:0002080, HP:0001347, HP:0000639, HP:0000651, HP:0001272', 'Coding', 'diagnostic - in-house testing'),
('FGF14', 'chr13:102161574-102161724', 'AAG', '249', '301', 'AD', 'SCA27B', '620174', 'HP:0002131, HP:0001251, HP:0010545, HP:0010544, HP:0000639, HP:0002073, HP:0001751', 'Intronic', 'diagnostic - in-house testing'),
('FMR1', 'chrX:147912050-147912110', 'CGG', '44', '55', 'XLD', 'FXS, FXTAS, POF', '300624, 300623, 311360', 'HP:0001249, HP:0000729, HP:0000717, HP:0000271, HP:0012758, HP:0001251, HP:0001337, HP:0002080, HP:0100543, HP:0000815, HP:0008209, HP:0001300, HP:0001249, HP:0000726, HP:0002354, HP:0033051, HP:0001250', '5\' UTR', 'diagnostic - in-house testing'),
('FXN', 'chr9:69037286-69037304', 'GAA', '33', '66', 'AR', 'FRDA', '229300', 'HP:0001251, HP:0002066, HP:0001260, HP:0007256, HP:0001257, HP:0001324, HP:0000020, HP:0002650, HP:0001761, HP:0001638, HP:0001639, HP:0000819, HP:0002495', 'Intronic', 'diagnostic - in-house testing'),
('HTT', 'chr4:3074876-3074933', 'CAG', '26', '40', 'AD', 'HD', '143100', 'HP:0002072, HP:0002487, HP:0002067, HP:0002375, HP:0002317, HP:0012547, HP:0000496, HP:0000751', 'Coding', 'diagnostic - in-house testing'),
('JPH3', 'chr16:87604287-87604329', 'CTG', '28', '40', 'AD', 'HDL2', '606438', 'HP:0002072, HP:0001332, HP:0002375, HP:0001260, HP:0002067, HP:0002063, HP:0000726, HP:0000751', 'Intronic, Coding, 3\'UTR', 'diagnostic - in-house testing'),
('PPP2R2B', 'chr5:146878727-146878757', 'GCT', '32', '66', 'AD', 'SCA12', '604326', 'HP:0002345, HP:0001347, HP:0001251, HP:0000726, HP:0001300', '5\' UTR', 'diagnostic - in-house testing'),
('RFC1', 'chr4:39348424-39348479', 'AARRG', '13', '400', 'AR', 'CANVAS', '614575', 'HP:0002066, HP:0001272, HP:0001260, HP:0002015, HP:0000763, HP:0003474, HP:0001756, HP:0002172, HP:0002141, HP:0000570, HP:0000666, HP:0034315', 'Intronic', 'diagnostic - in-house testing'),
('TBP', 'chr6:170561906-170562017', 'GCA', '40', '49', 'AD', 'SCA17, HDL4, OPCA5, CPD2', '607136', 'HP:0001251, HP:0000726, HP:0002072, HP:0001332, HP:0007256, HP:0001300, HP:0004305, HP:0001272', 'Coding', 'diagnostic - in-house testing'),
('AFF2', 'chrX:148500631-148500691', 'GCC', '25', '200', 'XLR', 'MRX109/FRAXE', '309548', 'HP:0000752, HP:0001419, HP:0000718, HP:0004209, HP:0100710, HP:0100023, HP:0000722, HP:0012172, HP:0000750, HP:0000256, HP:0001609, HP:0000729, HP:0000252, HP:0002370, HP:0002312, HP:0000426, HP:0000286, HP:0001511, HP:0004322, HP:0025116, HP:0011341, HP:0009904, HP:0001249, HP:0012471, HP:0000713, HP:0001328', '5\' UTR', 'diagnostic - external testing'),
('CNBP', 'chr3:129172576-129172656', 'CAGG', '26', '75', 'AD', 'DM2, PROMM', '602668', 'HP:0003701, HP:0003722, HP:0003554, HP:0000006, HP:0003326, HP:0002486', 'Intronic', 'diagnostic - external testing'),
('CSTB', 'chr21:43776443-43776479', 'CGCGGGGCGGGG', '3', '30', 'AR', 'EPM1A', '254800', 'HP:0001260, HP:0002070, HP:0007000, HP:0002392, HP:0003621, HP:0000007, HP:0002121, HP:0011182, HP:0002080, HP:0001336, HP:0001251, HP:0002069, HP:0001268, HP:0001249, HP:0010850, HP:0000992, HP:0001256, HP:0000726', 'Promoter', 'diagnostic - external testing'),
('DIP2B', 'chr12:50505001-50505022', 'GGC', '23', '350', 'AD', 'FRA12A', '136630', 'HP:0001249, HP:0003593, HP:0000708, HP:0002783, HP:0001250, HP:0001019, HP:0001263, HP:0000006, HP:0000962', '5\' UTR', 'research'),
('DMPK', 'chr19:45770204-45770264', 'CAG', '37', '51', 'AD', 'DM1', '160900', 'HP:0002486, HP:0012899, HP:0000518, HP:0002058, HP:0003323, HP:0001558, HP:0000708, HP:0002292, HP:0003115', '3\' UTR', 'diagnostic - external testing'),
('PHOX2B', 'chr4:41745972-41746032', 'GCN', '20', '24', 'AD', 'CCHS1', '209880', 'HP:0007110, HP:0040213, HP:0002104, HP:0011968, HP:0003623, HP:0003593, HP:0004370, HP:0030211, HP:0003006', 'Coding', 'diagnostic - external testing'),
('NOTCH2NLC', 'chr1:149390802-149390841', 'GGC', '39', '71', 'AD', 'NIID, OPDM3, ETM6', '603472, 619473, 618866', 'HP:0001337, HP:0002180', '5\' UTR', 'diagnostic - external testing'),
('TCF4', 'chr18:55586155-55586227', 'CAG', '31', '50', 'AD', 'FECD3', '613267', 'HP:0003581, HP:0000006, HP:0007957, HP:0000505, HP:0012038, HP:0012040', 'Intronic', 'diagnostic - external testing'),
('ABCD3', 'chr1:94418421-94418442', 'GCC', '44', '119', 'AD', 'OPDM', '', 'HP:0000508, HP:0000544, HP:0001324', '5\' UTR', 'research'),
('ARX_1', 'chrX:25013649-25013697', 'NGC', '16', '19', 'XLR', 'DEE1, MRXARX', '308350, 300419', 'HP:0100543, HP:0011463', 'Coding', 'research'),
('ARX_2', 'chrX:25013529-25013565', 'NGC', '12', '21', 'XLR', 'DEE1, PRTS, MRXARX', '308350, 300419', 'HP:0100543, HP:0011463', 'Coding', 'research'),
('DAB1', 'chr1:57367043-57367118', 'RAAAT', '0', '31', 'AD', 'SCA37', '615945', 'HP:0001251, HP:0001260', 'Intronic', 'research'),
('GIPC1', 'chr19:14496041-14496074', 'CCG', '32', '73', 'AD', 'OPDM2', '618940', 'HP:0000508, HP:0001283, HP:0002460, HP:0030319, HP:0100297, HP:0003805, HP:0003458, HP:0003557', '5\' UTR', 'research'),
('GLS', 'chr2:190880872-190880920', 'GCA', '29', '450', 'AR', 'GDPAG', '618412', 'HP:0011463, HP:0000007, HP:0000750, HP:0002373, HP:0001272, HP:0001263, HP:0002194, HP:0002073, HP:0003217', '5\' UTR', 'research'),
('LRP12', 'chr8:104588970-104588997', 'CGC', '45', '94', 'AD', 'OPDM1', '164310', 'HP:0000508, HP:0000544, HP:0001324', '5\' UTR', 'research'),
('NIPA1', 'chr15:22786677-22786701', 'GCG', '10', '11', 'AD', 'ALS (Suszeptibilitätsfaktor)', '', 'HP:0007354', 'Coding', 'research'),
('NOP56', 'chr20:2652733-2652757', 'GGCCTG', '14', '650', 'AD', 'SCA36', '614153', 'HP:0001251, HP:0001347, HP:0002380, HP:0012473', 'Intronic', 'research'),
('PABPN1', 'chr14:23321472-23321502', 'GCN', '10', '11', 'AR+AD', 'OPMD1', '164300', 'HP:0002015, HP:0000508, HP:0003325, HP:0003701, HP:0001324', 'Coding', 'research'),
('RILPL1', 'chr12:123533720-123533750', 'GGC', '20', '135', 'AD', 'OPMD4', '619790', 'HP:0000508, HP:0000544, HP:0001324', '5\' UTR', 'research'),
('SOX3', 'chrX:140504316-140504361', 'NGC', '15', '23', 'XLR', 'PHPX, XLMR, MRGH', '312000', 'HP:0040075, HP:0004322, HP:0040075', 'Coding', 'research'),
('THAP11', 'chr16:67842863-67842950', 'CAG', '34', '47', 'AD', 'SCA', '', 'HP:0001251', 'Coding', 'research'),
('ZFHX3', 'chr16:72787694-72787757', 'GCC', '21', '41', 'AD', 'SCA4', '600223', 'HP:0001251', 'Coding', 'research'),
('EIF4A3', 'chr17:80147003-80147139', 'CCTCGCTGYGCCGCTGCCGA', '12', '14', 'AR', 'RCPS', '268305', 'HP:0001263, HP:0012758, HP:0000750, HP:0000160, HP:0000347, HP:0000277, HP:0000175, HP:0000201, HP:0006289, HP:0002992, HP:0001762', '5\' UTR', 'research'),
('MARCHF6', 'chr5:10356346-10356411', 'ATTTY', '0', '668', 'AD', 'FAME3', '613608', 'HP:0002378, HP:0001336, HP:0007359, HP:0002197, HP:0001250, HP:0002069, HP:0001337', 'Intronic', 'research'),
('NUTM2B-AS1', 'chr10:79826383-79826404', 'GGC', '16', '161', 'AD', 'OPML1', '618637', 'HP:0002460, HP:0003701, HP:0002015, HP:0002059, HP:0001260, HP:0001251, HP:0001337, HP:0000508, HP:0000544, HP:0002878, HP:0002579, HP:0030319', 'Noncoding transcript', 'research'),
('RAPGEF2', 'chr4:159342526-159342616', 'TTTYA', '0', '60', 'AD', 'FAME7', '618075', 'HP:0001250, HP:0033054, HP:0001336, HP:0001337', 'Intronic', 'research'),
('SAMD12', 'chr8:118366812-118366918', 'TRAAA', '0', '100', 'AD', 'FAME1', '601068', 'HP:0007359, HP:0001336, HP:0002123, HP:0002378, HP:0001337, HP:0001250', 'Intronic', 'research'),
('STARD7', 'chr2:96197066-96197121', 'RAAAT', '0', '274', 'AD', 'FAME2', '607876', 'HP:0001336, HP:0002378, HP:0001250, HP:0000643', 'Intronic', 'research'),
('TNRC6A', 'chr16:24613439-24613529', 'TTTYA', '0', '225', 'AD', 'FAME7', '618074', 'HP:0001250, HP:0001337, HP:0001336', 'Intronic', 'research'),
('XYLT1', 'chr16:17470907-17470922', 'GCC', '20', '72', 'AR', 'DBQD2/Baratela-Scott syndrome', '615777', 'HP:0008873, HP:0003510, HP:0012725, HP:0002652, HP:0007663, HP:0031936, HP:0002643', '5\'UTR', 'research'),
('YEATS2', 'chr3:183712187-183712222', 'TTTYA', '0', '221', 'AD', 'FAME4', '615127', 'HP:0007359, HP:0002197, HP:0001336, HP:0001250', 'Intronic', 'research');

INSERT INTO `repeat_expansion` (name, region, repeat_unit, disease_names, disease_ids_omim, hpo_terms, type) VALUES
('VWA1', 'chr1:1435798-1435818', 'GGCGCGGAGC', '', '', '', 'low evidence'),
('NOTCH2NLA', 'chr1:146228800-146228821', 'GCC', '', '', '', 'low evidence'),
('AFF3', 'chr2:100104798-100104822', 'GCC', '', '', '', 'low evidence'),
('HOXD13', 'chr2:176093058-176093103', 'GCG', '', '', '', 'low evidence'),
('ATXN7_GCC', 'chr3:63912714-63912726', 'GCC', '', '', '', 'low evidence'),
('CNBP_CAGA', 'chr3:129172656-129172696', 'CAGA', '', '', '', 'low evidence'),
('CNBP_CA', 'chr3:129172696-129172732', 'CA', '', '', '', 'low evidence'),
('FOXL2', 'chr3:138946020-138946062', 'NGC', '', '', '', 'low evidence'),
('HTT_CCG', 'chr4:3074939-3074966', 'CCG', '', '', '', 'low evidence'),
('RUNX2', 'chr6:45422750-45422792', 'GCN', '', '', '', 'low evidence'),
('HOXA13_3', 'chr7:27199678-27199732', 'NGC', '', '', '', 'low evidence'),
('HOXA13_2', 'chr7:27199825-27199861', 'NGC', '', '', '', 'low evidence'),
('HOXA13_1', 'chr7:27199924-27199966', 'NGC', '', '', '', 'low evidence'),
('ZNF713', 'chr7:55887600-55887639', 'GCG', '', '', '', 'low evidence'),
('FXN_A', 'chr9:69037261-69037286', 'A', '', '', '', 'low evidence'),
('PRDM12', 'chr9:130681606-130681639', 'GCC', '', '', '', 'low evidence'),
('FRA10AC1_CCA', 'chr10:93702516-93702522', 'CCA', '', '', '', 'low evidence'),
('FRA10AC1', 'chr10:93702522-93702546', 'CCG', '', '', '', 'low evidence'),
('C11ORF80', 'chr11:66744819-66744843', 'GGC', '', '', '', 'low evidence'),
('CBL', 'chr11:119206289-119206322', 'CGG', '', '', '', 'low evidence'),
('ZIC2', 'chr13:99985448-99985493', 'GCN', '', '', '', 'low evidence'),
('FGF14_AAGA', 'chr13:102161566-102161574', 'AAGA', '', '', '', 'low evidence'),
('BEAN1', 'chr16:66490398-66490453', 'TAAAA', '', '', '', 'low evidence'),
('COMP', 'chr19:18786034-18786049', 'GTC', '', '', '', 'low evidence'),
('NOP56_CGCCTG', 'chr20:2652757-2652775', 'CGCCTG', '', '', '', 'low evidence'),
('PRNP_CCTCAGGGCGGTGGTGGCTGGGGGCAG', 'chr20:4699370-4699397', 'CCTCAGGGCGGTGGTGGCTGGGGGCAG', '', '', '', 'low evidence'),
('PRNP', 'chr20:4699397-4699493', 'CCTCATGGTGGTGGCTGGGGGCAG', '', '', '', 'low evidence'),
('TBX1', 'chr22:19766762-19766807', 'GCN', '', '', '', 'low evidence'),
('CSNK1E', 'chr22:38317282-38317306', 'GCC', '', '', '', 'low evidence'),
('BCLAF3', 'chrX:19990922-19990973', 'CCG', '', '', '', 'low evidence'),
('DMD', 'chrX:31284557-31284605', 'TTC', '', '', '', 'low evidence'),
('ZIC3', 'chrX:137566826-137566856', 'GCC', '', '', '', 'low evidence'),
('TMEM185A_CGCCGT', 'chrX:149631723-149631735', 'CGCCGT', '', '', '', 'low evidence'),
('TMEM185A', 'chrX:149631735-149631780', 'CGC', '', '', '', 'low evidence');
