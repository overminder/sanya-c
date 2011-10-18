# automatically generated from .pymakegen_v2.conf
# timestamp: 1318931093.679702
# handler_namespaces: ['root', handler.flexbison, handler.gcc]

flexbison_BISON=bison
flexbison_FLEX=flex
gcc_CC=gcc
gcc_CFLAGS=-O3 -ggdb3 -Wall -Winline -Wwrite-strings  \
	    -Wno-unused -c
gcc_INCLUDES=-I./
gcc_LDFLAGS=-lreadline -ggdb3 -O3
gcc_TARGET=omscm-c

all : sparse/scm_token.flex.h sparse/scm_token.flex.c  \
	    sparse/scm_syntax.bison.h sparse/scm_syntax.bison.c  \
	    $(gcc_TARGET)
	
$(gcc_TARGET) : sobj.o main.o seval.o sgc.o slang.o slib.o  \
	    sparse/scm_token.flex.o sparse/scm_syntax.bison.o
	$(gcc_CC) $(gcc_LDFLAGS) sobj.o main.o seval.o sgc.o slang.o  \
	    slib.o sparse/scm_token.flex.o sparse/scm_syntax.bison.o -o  \
	    $(gcc_TARGET)

main.o : main.c sgc.h sobj.h slib.h sobj.h seval.h sobj.h
	$(gcc_CC) $(gcc_CFLAGS) main.c $(gcc_INCLUDES) -o main.o

seval.o : seval.c sgc.h sobj.h slang.h sobj.h seval_impl.h  \
	    slib.h sobj.h seval.h sobj.h
	$(gcc_CC) $(gcc_CFLAGS) seval.c $(gcc_INCLUDES) -o seval.o

sgc.o : sgc.c sgc.h sobj.h
	$(gcc_CC) $(gcc_CFLAGS) sgc.c $(gcc_INCLUDES) -o sgc.o

slang.o : slang.c sgc.h sobj.h slang.h sobj.h seval_impl.h  \
	    seval.h sobj.h
	$(gcc_CC) $(gcc_CFLAGS) slang.c $(gcc_INCLUDES) -o slang.o

slib.o : slib.c sgc.h sobj.h seval_impl.h rl.h seval.h sobj.h  \
	    slib.h sobj.h
	$(gcc_CC) $(gcc_CFLAGS) slib.c $(gcc_INCLUDES) -o slib.o

sobj.o : sobj.c sgc.h sobj.h sobj.h seval.h sobj.h
	$(gcc_CC) $(gcc_CFLAGS) sobj.c $(gcc_INCLUDES) -o sobj.o

sparse/scm_syntax.bison.c : sparse/scm_syntax.y
	$(flexbison_BISON) -o sparse/scm_syntax.bison.c  \
	    sparse/scm_syntax.y

sparse/scm_syntax.bison.h : sparse/scm_syntax.y
	$(flexbison_BISON) -o /dev/null sparse/scm_syntax.y  \
	    --defines=sparse/scm_syntax.bison.h

sparse/scm_syntax.bison.o : sparse/scm_syntax.bison.c  \
	    sparse/scm_token.flex.h sparse/obj_api.h sgc.h sobj.h sobj.h
	$(gcc_CC) $(gcc_CFLAGS) sparse/scm_syntax.bison.c  \
	    $(gcc_INCLUDES) -o sparse/scm_syntax.bison.o

sparse/scm_token.flex.c : sparse/scm_token.lex
	$(flexbison_FLEX) -o sparse/scm_token.flex.c  \
	    sparse/scm_token.lex

sparse/scm_token.flex.h : sparse/scm_token.lex
	$(flexbison_FLEX) --header-file=sparse/scm_token.flex.h -o  \
	    /dev/null sparse/scm_token.lex

sparse/scm_token.flex.o : sparse/scm_token.flex.c  \
	    sparse/obj_api.h sgc.h sobj.h sobj.h sparse/scm_syntax.bison.h
	$(gcc_CC) $(gcc_CFLAGS) sparse/scm_token.flex.c  \
	    $(gcc_INCLUDES) -o sparse/scm_token.flex.o

clean : 
	rm -rf sparse/scm_token.flex.h sparse/scm_token.flex.c  \
	    sparse/scm_syntax.bison.h sparse/scm_syntax.bison.c 
	 rm -rf  \
	    sobj.o main.o seval.o sgc.o slang.o slib.o  \
	    sparse/scm_token.flex.o sparse/scm_syntax.bison.o
.PHONY : clean
