dtarget: myshell.c lex.yy.c
	cc -o myshell myshell.c lex.yy.c -lfl

lex.yy.c: 
	flex lex.l

clean:
	rm myshell lex.yy.c 
