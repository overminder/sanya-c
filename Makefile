# automatically generated from .pymakegen_v2.conf
# timestamp: 1318677567.802614
# handler_namespaces: ['root', handler.flexbison, handler.gcc]

flexbison_BISON=bison
flexbison_FLEX=flex
gcc_CC=gcc
gcc_CFLAGS=-ggdb3 -O0 -Wall -Winline -Wwrite-strings  \
	    -Wno-unused -c
gcc_INCLUDES=-I./
gcc_LDFLAGS=-lreadline -O1
gcc_TARGET=omscm-c

all : pgen/scm_token.flex.h pgen/scm_token.flex.c  \
	    pgen/scm_syntax.bison.h pgen/scm_syntax.bison.c $(gcc_TARGET)
	
$(gcc_TARGET) : sobj.o main.o seval.o sgc.o slib.o  \
	    pgen/scm_token.flex.o pgen/scm_syntax.bison.o
	$(gcc_CC) $(gcc_LDFLAGS) sobj.o main.o seval.o sgc.o slib.o  \
	    pgen/scm_token.flex.o pgen/scm_syntax.bison.o -o $(gcc_TARGET)

main.o : main.c sgc.h sobj.h rl.h seval.h sobj.h
	$(gcc_CC) $(gcc_CFLAGS) main.c $(gcc_INCLUDES) -o main.o

pgen/scm_syntax.bison.c : pgen/scm_syntax.y
	$(flexbison_BISON) -o pgen/scm_syntax.bison.c  \
	    pgen/scm_syntax.y

pgen/scm_syntax.bison.h : pgen/scm_syntax.y
	$(flexbison_BISON) -o /dev/null pgen/scm_syntax.y  \
	    --defines=pgen/scm_syntax.bison.h

pgen/scm_syntax.bison.o : pgen/scm_syntax.bison.c  \
	    pgen/scm_token.flex.h pgen/obj_api.h sgc.h sobj.h sobj.h
	$(gcc_CC) $(gcc_CFLAGS) pgen/scm_syntax.bison.c  \
	    $(gcc_INCLUDES) -o pgen/scm_syntax.bison.o

pgen/scm_token.flex.c : pgen/scm_token.lex
	$(flexbison_FLEX) -o pgen/scm_token.flex.c pgen/scm_token.lex

pgen/scm_token.flex.h : pgen/scm_token.lex
	$(flexbison_FLEX) --header-file=pgen/scm_token.flex.h -o  \
	    /dev/null pgen/scm_token.lex

pgen/scm_token.flex.o : pgen/scm_token.flex.c  \
	    pgen/scm_syntax.bison.h pgen/obj_api.h sgc.h sobj.h sobj.h
	$(gcc_CC) $(gcc_CFLAGS) pgen/scm_token.flex.c $(gcc_INCLUDES)  \
	    -o pgen/scm_token.flex.o

seval.o : seval.c sgc.h sobj.h seval_impl.h slib.h sobj.h  \
	    seval.h sobj.h
	$(gcc_CC) $(gcc_CFLAGS) seval.c $(gcc_INCLUDES) -o seval.o

sgc.o : sgc.c sgc.h sobj.h
	$(gcc_CC) $(gcc_CFLAGS) sgc.c $(gcc_INCLUDES) -o sgc.o

slib.o : slib.c sgc.h sobj.h seval_impl.h seval.h sobj.h slib.h \
	     sobj.h
	$(gcc_CC) $(gcc_CFLAGS) slib.c $(gcc_INCLUDES) -o slib.o

sobj.o : sobj.c sgc.h sobj.h sobj.h
	$(gcc_CC) $(gcc_CFLAGS) sobj.c $(gcc_INCLUDES) -o sobj.o

clean : 
	rm -rf pgen/scm_token.flex.h pgen/scm_token.flex.c  \
	    pgen/scm_syntax.bison.h pgen/scm_syntax.bison.c 
	 rm -rf  \
	    sobj.o main.o seval.o sgc.o slib.o pgen/scm_token.flex.o  \
	    pgen/scm_syntax.bison.o
.PHONY : clean
