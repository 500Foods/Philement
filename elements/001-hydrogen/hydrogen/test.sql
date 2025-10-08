CREATE OR REPLACE FUNCTION TEST.json_ingest(s CLOB)
RETURNS CLOB
LANGUAGE SQL
DETERMINISTIC
BEGIN
  DECLARE i INTEGER DEFAULT 1;
  DECLARE L INTEGER;
  DECLARE ch CHAR(1);
  DECLARE out CLOB(10M) DEFAULT '';
  DECLARE in_str SMALLINT DEFAULT 0;
  DECLARE esc SMALLINT DEFAULT 0;

  SET L = LENGTH(s);

  -- fast path: Validate input JSON (supports arrays and objects)
  IF s IS JSON THEN
    RETURN s;
  END IF;

  WHILE i <= L DO
    SET ch = SUBSTR(s, i, 1);

    IF esc = 1 THEN
      SET out = out || ch;
      SET esc = 0;

    ELSEIF ch = '\' THEN
      SET out = out || ch;
      SET esc = 1;

    ELSEIF ch = '"' THEN
      SET out = out || ch;
      SET in_str = 1 - in_str;

    ELSEIF in_str = 1 AND ch = X'0A' THEN -- \n
      SET out = out || '\n';

    ELSEIF in_str = 1 AND ch = X'0D' THEN -- \r
      SET out = out || '\r';

    ELSEIF in_str = 1 AND ch = X'09' THEN -- \t
      SET out = out || '\t';

    ELSE
      SET out = out || ch;
    END IF;

    SET i = i + 1;
  END WHILE;

  -- ensure result is JSON
  IF out IS NOT JSON THEN
    SIGNAL SQLSTATE '22032' SET MESSAGE_TEXT = 'Invalid JSON after normalization';
  END IF;
  RETURN out;
END@
