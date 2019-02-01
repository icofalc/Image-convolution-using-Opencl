CC      = g++
CFLAGS  = -g
RM      = rm -f





prova: prova.cpp
	$(CC) $(CFLAGS) -o prova prova.cpp -lpng -lOpenCL

clean veryclean:
	$(RM) Hello
