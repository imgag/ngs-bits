
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
