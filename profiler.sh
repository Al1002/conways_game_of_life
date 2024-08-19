gprof build/src/main gmon.out > analisys.txt
gprof2dot -w -s analisys.txt > analisys.dot
xdot analisys.dot