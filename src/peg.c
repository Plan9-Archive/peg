/* Copyright (c) 2007 by Ian Piumarta
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the 'Software'),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, provided that the above copyright notice(s) and this
 * permission notice appear in all copies of the Software.  Acknowledgement
 * of the use of this Software in supporting documentation would be
 * appreciated but is not required.
 * 
 * THE SOFTWARE IS PROVIDED 'AS IS'.  USE ENTIRELY AT YOUR OWN RISK.
 * 
 * Last edited: 2012-05-16 08:55:38 by piumarta on emilia
 */

#include <u.h>
#include <libc.h>
#include <stdio.h>

#include "tree.h"
#include "version.h"

FILE *input= 0;

int   verboseFlag= 0;

static int   lineNumber= 0;
static char *fileName= 0;

void yyerror(char *message);

#define YY_INPUT(buf, result, max)			\
{							\
  int c= getc(input);					\
  if ('\n' == c || '\r' == c) ++lineNumber;		\
  result= (EOF == c) ? 0 : (*(buf)= c, 1);		\
}

#define YY_LOCAL(T)	static T
#define YY_RULE(T)	static T

#include "peg.peg-c"

void yyerror(char *message)
{
  fprintf(stderr, "%s:%d: %s", fileName, lineNumber, message);
  if (yyctx->__text[0]) fprintf(stderr, " near token '%s'", yyctx->__text);
  if (yyctx->__pos < yyctx->__limit || !feof(input))
    {
      yyctx->__buf[yyctx->__limit]= '\0';
      fprintf(stderr, " before text \"");
      while (yyctx->__pos < yyctx->__limit)
	{
	  if ('\n' == yyctx->__buf[yyctx->__pos] || '\r' == yyctx->__buf[yyctx->__pos]) break;
	  fputc(yyctx->__buf[yyctx->__pos++], stderr);
	}
      if (yyctx->__pos == yyctx->__limit)
	{
	  int c;
	  while (EOF != (c= fgetc(input)) && '\n' != c && '\r' != c)
	    fputc(c, stderr);
	}
      fputc('\"', stderr);
    }
  fprintf(stderr, "\n");
  exits("yyerror");
}

static void version(char *name)
{
  printf("%s version %d.%d.%d\n", name, PEG_MAJOR, PEG_MINOR, PEG_LEVEL);
}

static void usage(char *name)
{
  version(name);
  fprintf(stderr, "usage: %s [<option>...] [<file>...]\n", name);
  fprintf(stderr, "where <option> can be\n");
  fprintf(stderr, "  -h          print this help information\n");
  fprintf(stderr, "  -o <ofile>  write output to <ofile>\n");
  fprintf(stderr, "  -v          be verbose\n");
  fprintf(stderr, "  -V          print version number and exit\n");
  fprintf(stderr, "if no <file> is given, input is read from stdin\n");
  fprintf(stderr, "if no <ofile> is given, output is written to stdout\n");
  exits("usage");
}

void
main(int argc, char **argv)
{
  Node *n;
  int   c;

  output= stdout;
  input= stdin;
  lineNumber= 1;
  fileName= "<stdin>";

  ARGBEGIN{
    case 'V':
      version(argv[0]);
      exits("version");
    case 'h':
      usage(argv[0]);
    case 'o':
      if(!(output = fopen(EARGF(usage(argv[0])), "w")))
		sysfatal("fopen: %r");
      break;
   case 'v':
      verboseFlag = 1;
      break;
  }ARGEND

  if (argc)
    {
      for (;  argc;  --argc, ++argv)
	{
	  if (!strcmp(*argv, "-"))
	    {
	      input= stdin;
	      fileName= "<stdin>";
	    }
	  else
	    {
	      if (!(input= fopen(*argv, "r")))
	        sysfatal("fopen: %r");
	      fileName= *argv;
	    }
	  lineNumber= 1;
	  if (!yyparse())
	    yyerror("syntax error");
	  if (input != stdin)
	    fclose(input);
	}
    }
  else
    if (!yyparse())
      yyerror("syntax error");

  if (verboseFlag)
    for (n= rules;  n;  n= n->any.next)
      Rule_print(n);

  Rule_compile_c_header();
  if (rules) Rule_compile_c(rules);

  exits(nil);
}
