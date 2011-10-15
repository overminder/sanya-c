# automatically generated from .pymakegen_v2.conf
# timestamp: 1318665238.296433
# handler_namespaces: ['root', handler.gcc, handler.flexbison]

flexbison_BISON=bison
flexbison_FLEX=flex

all : scm_token.flex.h scm_token.flex.c scm_syntax.bison.h  \
	    scm_syntax.bison.c
	
scm_syntax.bison.c : scm_syntax.y
	$(flexbison_BISON) -o scm_syntax.bison.c scm_syntax.y

scm_syntax.bison.h : scm_syntax.y
	$(flexbison_BISON) -o /dev/null scm_syntax.y  \
	    --defines=scm_syntax.bison.h

scm_token.flex.c : scm_token.lex
	$(flexbison_FLEX) -o scm_token.flex.c scm_token.lex

scm_token.flex.h : scm_token.lex
	$(flexbison_FLEX) --header-file=scm_token.flex.h -o /dev/null  \
	    scm_token.lex

clean : 
	rm -rf scm_token.flex.h scm_token.flex.c scm_syntax.bison.h  \
	    scm_syntax.bison.c
.PHONY : clean
