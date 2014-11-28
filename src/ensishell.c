/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "variante.h"
#include "readcmd.h"
#include "interpretor.h"


#ifndef VARIANTE
#error "Variante non défini !!"
#endif

int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);
        init();
	while (1) {
		struct cmdline *l;
		char *prompt = "ensishell>";


		l = readcmd(prompt);

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

    	interpret_seq(l);
	}
}
