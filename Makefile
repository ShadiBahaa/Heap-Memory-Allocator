libhmm.a: hmm.o
	ar -rs libhmm.a hmm.o

libhmm.so: hmm_pic.o
	gcc -shared -o libhmm.so hmm_pic.o

hmm.o: HMM.c
	gcc -o hmm.o -c HMM.c

hmm_pic.o: HMM.c
	gcc -fPIC -o hmm_pic.o -c HMM.c
