/** 2019 Neil Edelman, distributed under the terms of the
 [MIT License](https://opensource.org/licenses/MIT).

 This is a standard C file.

 @file Text
 @author Neil
 @std C89/90
 @version 20xx-xx
 @since 20xx-xx
 @param
 @fixme
 @deprecated */

#include <stdlib.h> /* EXIT */
#include <stdio.h>  /* fprintf */
#include <string.h> /* strcmp */
#include "Scanner.h"
#include "Report.h"

/** @param[argc, argv] If "debug", `freopens` a path that is on my computer. */
int main(int argc, char **argv) {
	int exit_code = EXIT_FAILURE;
	
	/* https://stackoverflow.com/questions/10293387/piping-into-application-run-under-xcode/13658537 */
	if (argc == 2 && strcmp(argv[1], "debug") == 0 ) {
		const char *test_file_path = "/Users/neil/Movies/Cdoc/foo.c";
		fprintf(stderr, "== [RUNNING IN DEBUG MODE with %s]==\n\n",
				test_file_path);
		freopen(test_file_path, "r", stdin);
	}
	
	if(!Scanner()) goto catch;
	/*{
		struct Segment *segment = 0;
		fputs("\n -- Print out: --\n", stdout);
		while((segment = SegmentArrayNext(&doc, segment))) {
			struct Tag *tag = 0;
			printf("Segment(%s):\n\tdoc: %s.\n\tcode: %s.\n",
				   namespaces[segment->namespace],
				   TokenArrayToString(&segment->doc),
				   TokenArrayToString(&segment->code));
			while((tag = TagArrayNext(&segment->tags, tag))) {
				printf("\t%s{%s} %s.\n", symbols[tag->symbol],
					   TokenArrayToString(&tag->header),
					   TokenArrayToString(&tag->contents));
			}
			fputc('\n', stdout);
		}
	}
	
	{
		fputs("\n\n", stdout);
		out(&doc);
	}*/
	
	exit_code = EXIT_SUCCESS; goto finally;
	
catch:
	perror("scanner");
	
finally:
	Report_();
	Scanner_();
	
	return exit_code;
}
