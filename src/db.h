#ifndef __DB_H
#define __DB_H

#include <sqlite3.h>

typedef struct
{
	char		*filename;
	sqlite3		*sq_hdl;
} na_db_t;

int na_db_init();
void na_db_free();
void na_db_import_directory(char *directory, int *stat_ok, int *stat_exist, int *stat_err);
char *na_db_get_filename_from_title(char *title);

#endif
