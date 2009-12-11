-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-- this table is needed by the AccountingInstance for storing cost 
-- confirmations. before you start the First Mix you must create the 
-- following table in the database. to do this you can log in to postgresql 
-- using the following shell command:
--
-- # psql -U <PayUser> -d <PayDB>
--
-- (Note that you should create the user <PayUser> and the database <PayDB> before using 'createuser' and 'createdb' commands
-- from the command line
-- You should then get the following Postgresql Prompt:
--
-- PayDB =>
--
-- On this prompt you can type "\i tables.sql" and hit enter. The tables are 
-- then created for you.
--
-- (c) 2oo4 Bastian Voigt <bavoigt@inf.fu-berlin.de>
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------


CREATE TABLE COSTCONFIRMATIONS (
	ACCOUNTNUMBER BIGSERIAL PRIMARY KEY,
	BYTES BIGINT,
	XMLCC VARCHAR(2000),
	SETTLED INTEGER
);
