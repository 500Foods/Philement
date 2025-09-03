create database hydrogen;
connect to hydrogen;

create table TEST456 (
    TEST_NUMBER INTEGER NOT NULL)
;

insert into TEST456 values(789);
select * from TEST456;

drop table TEST456;

disconnect hydrogen;
drop database hydrogen;
