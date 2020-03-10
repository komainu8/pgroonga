#include "pgroonga.h"

#include "pgrn-column-name.h"
#include "pgrn-global.h"
#include "pgrn-groonga.h"
#include "pgrn-wal.h"

#include <catalog/catalog.h>
#include <miscadmin.h>

bool PGrnIsLZ4Available;
bool PGrnIsZlibAvailable;
bool PGrnIsZstdAvailable;
bool PGrnIsTemporaryIndexSearchAvailable;

static grn_ctx *ctx = &PGrnContext;
static struct PGrnBuffers *buffers = &PGrnBuffers;

static bool
IsTemporaryIndexSearchAvailable(void)
{
	const char *libgroonga_version;
	int major_version;

	libgroonga_version = grn_get_version();

	major_version = atoi(libgroonga_version);

	if (major_version > 8)
		return true;

	if (major_version < 8)
		return false;

	return strcmp(libgroonga_version, "8.0.2") > 0;
}

void
PGrnInitializeGroongaInformation(void)
{
	grn_obj grnIsSupported;

	GRN_BOOL_INIT(&grnIsSupported, 0);

	GRN_BULK_REWIND(&grnIsSupported);
	grn_obj_get_info(ctx, NULL, GRN_INFO_SUPPORT_LZ4, &grnIsSupported);
	PGrnIsLZ4Available = (GRN_BOOL_VALUE(&grnIsSupported));

	GRN_BULK_REWIND(&grnIsSupported);
	grn_obj_get_info(ctx, NULL, GRN_INFO_SUPPORT_ZLIB, &grnIsSupported);
	PGrnIsZlibAvailable = (GRN_BOOL_VALUE(&grnIsSupported));

	GRN_BULK_REWIND(&grnIsSupported);
	grn_obj_get_info(ctx, NULL, GRN_INFO_SUPPORT_ZSTD, &grnIsSupported);
	PGrnIsZstdAvailable = (GRN_BOOL_VALUE(&grnIsSupported));
	PGrnIsTemporaryIndexSearchAvailable = IsTemporaryIndexSearchAvailable();

	GRN_OBJ_FIN(ctx, &grnIsSupported);
}

const char *
PGrnInspect(grn_obj *object)
{
	grn_obj *buffer = &(PGrnBuffers.inspect);

	GRN_BULK_REWIND(buffer);
	{
		grn_rc rc = ctx->rc;
		grn_inspect(ctx, buffer, object);
		ctx->rc = rc;
	}
	GRN_TEXT_PUTC(ctx, buffer, '\0');
	return GRN_TEXT_VALUE(buffer);
}

const char *
PGrnInspectName(grn_obj *object)
{
	static char name[GRN_TABLE_MAX_KEY_SIZE];
	int nameSize;

	{
		grn_rc rc = ctx->rc;
		nameSize = grn_obj_name(ctx, object, name, GRN_TABLE_MAX_KEY_SIZE);
		name[nameSize] = '\0';
		ctx->rc = rc;
	}

	return name;
}

int
PGrnRCToPgErrorCode(grn_rc rc)
{
	int errorCode = ERRCODE_SYSTEM_ERROR;

	/* TODO: Fill me. */
	switch (rc)
	{
	case GRN_NO_SUCH_FILE_OR_DIRECTORY:
		errorCode = ERRCODE_IO_ERROR;
		break;
	case GRN_INPUT_OUTPUT_ERROR:
		errorCode = ERRCODE_IO_ERROR;
		break;
	case GRN_INVALID_ARGUMENT:
		errorCode = ERRCODE_INVALID_PARAMETER_VALUE;
		break;
	default:
		break;
	}

	return errorCode;
}

grn_bool
PGrnCheck(const char *format, ...)
{
#define MESSAGE_SIZE 4096
	va_list args;
	char message[MESSAGE_SIZE];

	if (ctx->rc == GRN_SUCCESS)
		return GRN_TRUE;

	va_start(args, format);
	grn_vsnprintf(message, MESSAGE_SIZE, format, args);
	va_end(args);
	ereport(ERROR,
			(errcode(PGrnRCToPgErrorCode(ctx->rc)),
			 errmsg("pgroonga: %s: %s", message, ctx->errbuf)));
	return GRN_FALSE;
#undef MESSAGE_SIZE
}

grn_obj *
PGrnLookup(const char *name, int errorLevel)
{
	return PGrnLookupWithSize(name, strlen(name), errorLevel);
}

grn_obj *
PGrnLookupWithSize(const char *name,
				   size_t nameSize,
				   int errorLevel)
{
	grn_obj *object;
	PGrnEnsureDatabase();
	object = grn_ctx_get(ctx, name, nameSize);
	if (!object)
		ereport(errorLevel,
				(errcode(ERRCODE_INVALID_NAME),
				 errmsg("pgroonga: object isn't found: <%.*s>",
						(int)nameSize, name)));
	return object;
}

grn_obj *
PGrnLookupColumn(grn_obj *table, const char *name, int errorLevel)
{
	return PGrnLookupColumnWithSize(table, name, strlen(name), errorLevel);
}

grn_obj *
PGrnLookupColumnWithSize(grn_obj *table,
						 const char *name,
						 size_t nameSize,
						 int errorLevel)
{
	char columnName[GRN_TABLE_MAX_KEY_SIZE];
	size_t columnNameSize;
	grn_obj *column;

	columnNameSize = PGrnColumnNameEncodeWithSize(name, nameSize, columnName);
	column = grn_obj_column(ctx, table, columnName, columnNameSize);
	if (!column)
	{
		char tableName[GRN_TABLE_MAX_KEY_SIZE];
		int tableNameSize;

		tableNameSize = grn_obj_name(ctx, table, tableName, sizeof(tableName));
		ereport(errorLevel,
				(errcode(ERRCODE_INVALID_NAME),
				 errmsg("pgroonga: column isn't found: <%.*s>:<%.*s>",
						tableNameSize, tableName,
						(int)nameSize, name)));
	}

	return column;
}

grn_obj *
PGrnLookupSourcesTable(Relation index, int errorLevel)
{
	char name[GRN_TABLE_MAX_KEY_SIZE];

	snprintf(name, sizeof(name),
			 PGrnSourcesTableNameFormat,
			 index->rd_node.relNode);
	return PGrnLookup(name, errorLevel);
}

grn_obj *
PGrnLookupSourcesCtidColumn(Relation index, int errorLevel)
{
	char name[GRN_TABLE_MAX_KEY_SIZE];

	snprintf(name, sizeof(name),
			 PGrnSourcesTableNameFormat "." PGrnSourcesCtidColumnName,
			 index->rd_node.relNode);
	return PGrnLookup(name, errorLevel);
}

grn_obj *
PGrnLookupLexicon(Relation index, unsigned int nthAttribute, int errorLevel)
{
	char name[GRN_TABLE_MAX_KEY_SIZE];

	snprintf(name, sizeof(name),
			 PGrnLexiconNameFormat,
			 index->rd_node.relNode,
			 nthAttribute);
	return PGrnLookup(name, errorLevel);
}

grn_obj *
PGrnLookupIndexColumn(Relation index, unsigned int nthAttribute, int errorLevel)
{
	char name[GRN_TABLE_MAX_KEY_SIZE];

	snprintf(name, sizeof(name),
			 PGrnLexiconNameFormat ".%s",
			 index->rd_node.relNode,
			 nthAttribute,
			 PGrnIndexColumnName);
	return PGrnLookup(name, errorLevel);
}

grn_obj *
PGrnCreateTable(Relation index,
				const char *name,
				grn_table_flags flags,
				grn_obj *type,
				grn_obj *tokenizer,
				grn_obj *normalizer,
				grn_obj *tokenFilters)
{
	unsigned int nameSize = 0;

	if (name)
		nameSize = strlen(name);

	return PGrnCreateTableWithSize(index,
								   name,
								   nameSize,
								   flags,
								   type,
								   tokenizer,
								   normalizer,
								   tokenFilters);
}

grn_obj *
PGrnCreateTableWithSize(Relation index,
						const char *name,
						size_t nameSize,
						grn_table_flags flags,
						grn_obj *type,
						grn_obj *tokenizer,
						grn_obj *normalizer,
						grn_obj *tokenFilters)
{
	const char *path = NULL;
	char pathBuffer[MAXPGPATH];
	grn_obj	*table;

	if (name)
	{
		flags |= GRN_OBJ_PERSISTENT;
		if (index && index->rd_node.spcNode != MyDatabaseTableSpace)
		{
			char *databasePath;
			char filePath[MAXPGPATH];

			databasePath =
				GetDatabasePath(MyDatabaseId, index->rd_node.spcNode);
			snprintf(filePath, sizeof(filePath),
					 "%s.%.*s",
					 PGrnDatabaseBasename,
					 (int)nameSize,
					 name);
			join_path_components(pathBuffer,
								 databasePath,
								 filePath);
			pfree(databasePath);

			path = pathBuffer;
		}
	}

	table = grn_table_create(ctx,
							 name,
							 nameSize,
							 path,
							 flags,
							 type,
							 NULL);
	PGrnCheck("failed to create table: <%.*s>",
			  (int)nameSize, name);
	if (tokenizer)
		grn_obj_set_info(ctx, table, GRN_INFO_DEFAULT_TOKENIZER, tokenizer);
	if (normalizer)
		grn_obj_set_info(ctx, table, GRN_INFO_NORMALIZER, normalizer);
	if (tokenFilters)
		grn_obj_set_info(ctx, table, GRN_INFO_TOKEN_FILTERS, tokenFilters);

	PGrnWALCreateTable(index,
					   name,
					   nameSize,
					   flags,
					   type,
					   tokenizer,
					   normalizer,
					   tokenFilters);

	return table;
}

grn_obj *
PGrnCreateColumn(Relation	index,
				 grn_obj	*table,
				 const char *name,
				 grn_column_flags flags,
				 grn_obj	*type)
{
	return PGrnCreateColumnWithSize(index,
									table,
									name,
									strlen(name),
									flags,
									type);
}

grn_obj *
PGrnCreateColumnWithSize(Relation	index,
						 grn_obj	*table,
						 const char *name,
						 size_t		nameSize,
						 grn_column_flags flags,
						 grn_obj	*type)
{
	const char *path = NULL;
	char pathBuffer[MAXPGPATH];
	grn_obj *column;

	if (name)
	{
		flags |= GRN_OBJ_PERSISTENT;
		if (index && index->rd_node.spcNode != MyDatabaseTableSpace)
		{
			char *databasePath;
			char tableName[GRN_TABLE_MAX_KEY_SIZE];
			int tableNameSize;
			char filePath[MAXPGPATH];

			databasePath =
				GetDatabasePath(MyDatabaseId, index->rd_node.spcNode);
			tableNameSize = grn_obj_name(ctx,
										 table,
										 tableName,
										 GRN_TABLE_MAX_KEY_SIZE);
			snprintf(filePath, sizeof(filePath),
					 "%s.%.*s.%.*s",
					 PGrnDatabaseBasename,
					 tableNameSize,
					 tableName,
					 (int)nameSize,
					 name);
			join_path_components(pathBuffer,
								 databasePath,
								 filePath);
			pfree(databasePath);

			path = pathBuffer;
		}
	}
    column = grn_column_create(ctx,
							   table,
							   name,
							   nameSize,
							   path,
							   flags,
							   type);
	PGrnCheck("failed to create column: <%.*s>",
			  (int)nameSize, name);

	PGrnWALCreateColumn(index, table, name, nameSize, flags, type);

	return column;
}

void
PGrnIndexColumnClearSources(Relation index,
							grn_obj *indexColumn)
{
	GRN_BULK_REWIND(&(buffers->sourceIDs));
	PGrnIndexColumnSetSourceIDs(index, indexColumn, &(buffers->sourceIDs));
}

void
PGrnIndexColumnSetSource(Relation index,
						 grn_obj *indexColumn,
						 grn_obj *source)
{
	grn_id sourceID;

	GRN_BULK_REWIND(&(buffers->sourceIDs));

	sourceID = grn_obj_id(ctx, source);
	GRN_UINT32_PUT(ctx, &(buffers->sourceIDs), sourceID);

	PGrnIndexColumnSetSourceIDs(index, indexColumn, &(buffers->sourceIDs));
}

void
PGrnIndexColumnSetSourceIDs(Relation index,
							grn_obj *indexColumn,
							grn_obj *sourceIDs)
{
	grn_obj_set_info(ctx, indexColumn, GRN_INFO_SOURCE, sourceIDs);
	PGrnCheck("failed to set sources: <%s>: <%s>",
			  PGrnInspectName(indexColumn),
			  PGrnInspect(sourceIDs));
	PGrnWALSetSourceIDs(index, indexColumn, sourceIDs);
}

void
PGrnRenameTable(Relation index, grn_obj *table, const char *newName)
{
	char name[GRN_TABLE_MAX_KEY_SIZE];
	int nameSize;
	size_t newNameSize;

	nameSize = grn_obj_name(ctx, table, name, GRN_TABLE_MAX_KEY_SIZE);
	newNameSize = strlen(newName);
	grn_table_rename(ctx, table, newName, strlen(newName));
	PGrnCheck("failed to rename table: <%s> -> <%s>",
			  PGrnInspectName(table),
			  newName);

	PGrnWALRenameTable(index,
					   name,
					   nameSize,
					   newName,
					   newNameSize);
}

void
PGrnRemoveObject(const char *name)
{
	PGrnRemoveObjectWithSize(name, strlen(name));
}

void
PGrnRemoveObjectWithSize(const char *name,
						 size_t nameSize)
{
	grn_obj *object;

	object = PGrnLookupWithSize(name, nameSize, ERROR);
	grn_obj_remove(ctx, object);
	PGrnCheck("failed to remove: <%.*s>",
			  (int)nameSize, name);
}

void
PGrnFlushObject(grn_obj *object, bool recursive)
{
	grn_rc rc;
	char name[GRN_TABLE_MAX_KEY_SIZE];
	int nameSize;

	if (recursive)
	{
		rc = grn_obj_flush_recursive(ctx, object);
	}
	else
	{
		rc = grn_obj_flush(ctx, object);
	}
	if (rc == GRN_SUCCESS)
		return;

	nameSize = grn_obj_name(ctx, object, name, GRN_TABLE_MAX_KEY_SIZE);
	PGrnCheck("failed to flush: <%.*s>",
			  nameSize, name);
}

void
PGrnRemoveColumns(grn_obj *table)
{
	grn_hash *columns;

	columns = grn_hash_create(ctx,
							  NULL,
							  sizeof(grn_id),
							  0,
							  GRN_TABLE_HASH_KEY | GRN_HASH_TINY);
	if (!columns)
	{
		PGrnCheck("failed to create columns container for removing columns: "
				  "<%s>",
				  PGrnInspectName(table));
	}

	GRN_HASH_EACH_BEGIN(ctx, columns, cursor, id) {
		grn_id *columnID;
		grn_obj *column;

		grn_hash_cursor_get_key(ctx, cursor, (void **)&columnID);
		column = grn_ctx_at(ctx, *columnID);
		if (!column)
			continue;

		grn_obj_remove(ctx, column);
		PGrnCheck("failed to remove column: <%s>",
				  PGrnInspectName(column));
	} GRN_HASH_EACH_END(ctx, cursor);

	grn_hash_close(ctx, columns);
}
