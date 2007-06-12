drop table costconfirmations;
drop table prepaidamounts;
drop table accountstatus;
create table costconfirmations(accountnumber bigserial,bytes bigint,xmlcc varchar(2000),settled integer,cascade varchar(400),primary key(accountnumber,cascade));
create table prepaidamounts(accountnumber bigint unique not null,prepaidbytes integer,cascade varchar(400),primary key(accountnumber, cascade));
create table accountstatus(accountnumber bigint, statuscode integer);	
