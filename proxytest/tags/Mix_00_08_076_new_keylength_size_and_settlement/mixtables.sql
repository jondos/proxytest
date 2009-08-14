--will create an error if used with a fresh database, this is normal and no problem
-- to avoid the error message, use 
-- drop table if exists costconfirmations
-- (but will only work on Postgresql 8.2 and higher)
drop table costconfirmations;
drop table prepaidamounts;
drop table accountstatus;
create table costconfirmations(accountnumber bigserial,bytes bigint,xmlcc varchar(2000),settled integer,cascade varchar(200),primary key(accountnumber,cascade));
create table prepaidamounts(accountnumber bigint not null,prepaidbytes integer,cascade varchar(300),primary key(accountnumber, cascade));
create table accountstatus(accountnumber bigint, statuscode integer, expires varchar(10));	
