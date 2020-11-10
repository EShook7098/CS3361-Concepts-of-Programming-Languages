#I would strongly recommend NOT changing any lines below except the CC and MYFILE lines.
#Before running this file, run the command:   module load gnu


EXECS=danc_parser


#Replace the g++ with gcc if using C
CC=g++

#Replace with the name of your C or C++ source code file.
MYFILE=Ethan_Shook_11469438_Assignment4.cpp


all: ${EXECS}

${EXECS}: ${MYFILE}
	${CC} -std=c++11 -o ${EXECS} ${MYFILE}

clean:
	rm -f ${EXECS}
