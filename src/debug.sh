# launching server (/dev/null will cause no log to be written)
./distprime 2 100000 /dev/null
./distprime 10000000500000 10000000504000 test1.log
./distprime 150000000 500000000 test2.log

# launching worker
./distprimeworker 1
./distprimeworker 4
./distprimeworker 8

# concurrent launch
./distprimeworker 1 & ./distprimeworker 2 & ./distprime 2500000000000 2500001000000 /dev/null
./distprimeworker 2 & ./distprimeworker 2 & ./distprime 1 10000000 /dev/null

# kill all processes
pidof distprimeworker | xargs kill $1
pidof distprime | xargs sudo gdb distprime $1
