#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <portaudio.h>
#include <sndfile.h>

#include "noya.h"
#include "db.h"

#define SQL_TEST(a)	"select * from " a " LIMIT 1"

#define SQL_CREATE_CONFIG \
	"create table config (key varchar(200), value varchar(200), primary key (key, value));"

#define SQL_CREATE_SOUNDS \
	"create table sounds (" \
	"id unsigned int auto_increment, " \
	"filename varchar, " \
	"title varchar, "\
	"channels tinyint, " \
	"samplerate int, " \
	"duration float, " \
	"bpm_duration int, " \
	"score tinyint, " \
	"is_imported tinyint, " \
	"is_reviewed tinyint, " \
	"primary key (id));"

LOG_DECLARE("DB");

static na_db_t	*g_db	= NULL;

na_db_t *na_db_open(char *filename);
void na_db_close(na_db_t *db);
int na_db_initialize(na_db_t *db);


na_db_t *na_db_open(char *filename)
{
	na_db_t *db;
	int		ret;

	db = malloc(sizeof(na_db_t));
	if ( db == NULL )
	{
		l_errorf("OOM while opening file");
		goto na_db_open_error;
	}

	bzero(db, sizeof(na_db_t));

	db->filename = strdup(filename);

	ret = sqlite3_open(filename, &db->sq_hdl);
	if ( ret != SQLITE_OK )
	{
		l_errorf("unable to open database %s: %s", filename, sqlite3_errmsg(ret));
		goto na_db_open_error;
	}

	if ( na_db_initialize(db) != 0 )
	{
		l_errorf("unable to initialize database %s", filename);
		goto na_db_open_error;
	}

	return db;

na_db_open_error:
	na_db_close(db);
	return NULL;
}

void na_db_close(na_db_t *db)
{
	if ( db->filename != NULL )
		free(db->filename);
	if ( db->sq_hdl != NULL )
		sqlite3_close(db->sq_hdl);
	free(db);
}

int na_db_initialize(na_db_t *db)
{
	int row, column, ret;
	char **result;

	/* check if initial scheme is available
	 */

	ret = sqlite3_get_table(db->sq_hdl, SQL_TEST("config"), &result, &row, &column, NULL);
	if ( ret != SQLITE_OK )
	{
		l_errorf("table ? %d", ret);
		if ( sqlite3_exec(db->sq_hdl, SQL_CREATE_CONFIG, NULL, NULL, NULL) != SQLITE_OK )
		{
			l_errorf("unable to create config table");
			return -1;
		}

	}
	else
		sqlite3_free_table(result);

	if ( sqlite3_get_table(db->sq_hdl, SQL_TEST("sounds"),
			&result, &row, &column, NULL) != SQLITE_OK )
	{
		if ( sqlite3_exec(db->sq_hdl, SQL_CREATE_SOUNDS, NULL, NULL, NULL) != SQLITE_OK )
		{
			l_errorf("unable to create sounds table");
			return -1;
		}
	}
	else
		sqlite3_free_table(result);

	return 0;
}

int na_db_init()
{
	g_db = na_db_open("noya.sqlite");
	if ( g_db == NULL )
		return -1;
	return 0;
}

void na_db_free()
{
	if ( g_db != NULL )
		na_db_close(g_db);
	g_db = NULL;
}

int na_db_filename_to_id(char *filename)
{
	int row, columns;
	char **result, *sql;

	if ( g_db == NULL )
		return 0;

	sql = sqlite3_mprintf("SELECT id FROM sounds WHERE filename = %Q", filename);
	if ( sqlite3_get_table(g_db->sq_hdl, sql, &result, &row, &columns, NULL) != SQLITE_OK )
	{
		sqlite3_free(sql);
		return 0;
	}
	sqlite3_free(sql);

	return row;
}

void na_db_import_directory(char *directory, int *stat_ok, int *stat_exist, int *stat_err)
{
	char			dl_name[2048];
	DIR				*p_dir;
	struct dirent	*p_dirent;
	struct stat		s;
	int				id;
	SNDFILE			*sfp = NULL;
	SF_INFO			sinfo;
	char			*sql = NULL;


	p_dir = opendir(directory);
	if ( p_dir == NULL )
	{
		l_errorf("unable to open directory %s", directory);
		return;
	}

	while ( (p_dirent = readdir(p_dir)) )
	{
		if ( strlen(p_dirent->d_name) > 0 && p_dirent->d_name[0] == '.' )
			continue;

		snprintf(dl_name, sizeof(dl_name), "%s/%s", directory, p_dirent->d_name);
		if ( stat(dl_name, &s) == -1 )
			continue;

		if ( S_ISDIR(s.st_mode) )
		{
			na_db_import_directory(dl_name, stat_ok, stat_exist, stat_err);
			continue;
		}

		if ( !S_ISREG(s.st_mode) )
			continue;

		/* search him in database
		 */
		id = na_db_filename_to_id(dl_name);
		if ( id != 0 )
		{
			*stat_exist = *stat_exist + 1;
			continue;
		}

		/* try to read wav.
		 */
		sfp = sf_open(dl_name, SFM_READ, &sinfo);
		if ( sfp == NULL )
		{
			*stat_err = *stat_err + 1;
			l_errorf("unable to import %s", dl_name);
			continue;
		}

		sql = sqlite3_mprintf(
			"INSERT INTO sounds (filename, title, channels, samplerate, duration, is_imported)"
			"VALUES (%Q, %Q, %d, %d, %f, 1)",
			dl_name,
			p_dirent->d_name,
			sinfo.channels,
			sinfo.samplerate,
			(float)sinfo.frames / (float)sinfo.samplerate
		);

		sf_close(sfp), sfp = NULL;

		sqlite3_exec(g_db->sq_hdl, sql, NULL, NULL, NULL);
		sqlite3_free(sql);

		*stat_ok = *stat_ok + 1;
	}

	closedir(p_dir);
}
