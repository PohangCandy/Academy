-- logdb 생성
CREATE DATABASE IF NOT EXISTS `logdb`;

-- 템플릿 테이블 생성 (월별 테이블 자동 생성 시 참조)
CREATE TABLE IF NOT EXISTS `logdb`.`monitorlog_template`
(
  `no`       BIGINT NOT NULL AUTO_INCREMENT,
  `logtime`  DATETIME NOT NULL,
  `serverno` INT NOT NULL,
  `type`     INT NOT NULL,
  `avr`      INT NOT NULL DEFAULT 0,
  `min`      INT NOT NULL DEFAULT 0,
  `max`      INT NOT NULL DEFAULT 0,
  PRIMARY KEY (`no`),
  KEY `idx_logtime` (`logtime`),
  KEY `idx_serverno_type` (`serverno`, `type`)
);
