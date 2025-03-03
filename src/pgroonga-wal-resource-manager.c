#include <groonga.h>

#define PGRN_TAG "pgroonga: wal-resource-manager"

static grn_ctx PGrnWRMContext;
static grn_ctx *ctx = &PGrnWRMContext;
static grn_obj PGrnInspectBuffer;

#include "pgrn-compatible.h"
#include "pgrn-constant.h"
#include "pgrn-file.h"
#include "pgrn-groonga.h"
#include "pgrn-log-level.h"
#include "pgrn-value.h"
#ifdef PGRN_SUPPORT_WAL_RESOURCE_MANAGER
#	include "pgrn-wal-custom.h"
#endif

#include <utils/guc.h>

PG_MODULE_MAGIC;

static grn_ctx PGrnWRMContext;
static char *PGrnWRMLogPath;
static int PGrnWRMLogLevel = GRN_LOG_DEFAULT_LEVEL;
PGRN_DEFINE_LOG_LEVEL_ENTRIES(PGrnWRMLogLevelEntries);
static Oid PGrnWRMCurrentDatabaseID = InvalidOid;
static Oid PGrnWRMCurrentDatabaseTableSpaceID = InvalidOid;

extern PGDLLEXPORT void _PG_init(void);

#ifdef PGRN_SUPPORT_WAL_RESOURCE_MANAGER
static uint32_t
pgrnwrm_get_thread_limit(void *data)
{
	return 1;
}

typedef struct PGrnWRMRedoData
{
	PGrnWALRecordCommon *walRecord;
	grn_obj *db;
} PGrnWRMRedoData;

static void
pgrnwrm_redo_setup(PGrnWRMRedoData *data, const char *tag)
{
	PGrnWALRecordCommon *walRecord = data->walRecord;
	grn_encoding encoding = PGrnPGEncodingToGrnEncoding(walRecord->dbEncoding);
	char *databasePath;
	char path[MAXPGPATH];
	grn_obj *db;

	GRN_CTX_SET_ENCODING(ctx, encoding);
	/* TODO: Cache DB only if the next WAL record is for the same DB. */
	if (walRecord->dbID == PGrnWRMCurrentDatabaseID &&
		walRecord->dbTableSpaceID == PGrnWRMCurrentDatabaseTableSpaceID)
		return;

	databasePath = GetDatabasePath(walRecord->dbID, walRecord->dbTableSpaceID);
	join_path_components(path, databasePath, PGrnDatabaseBasename);
	pfree(databasePath);
	db = grn_ctx_db(ctx);
	if (db)
		grn_obj_close(ctx, db);
	if (pgrn_file_exist(path))
	{
		data->db = grn_db_open(ctx, path);
		PGrnCheck("%s need to recreate this standby "
				  "because Groonga DB is broken: <%s>",
				  tag,
				  path);
	}
	else
	{
		data->db = grn_db_create(ctx, path, NULL);
		PGrnCheck("%s need to recreate this standby "
				  "because can't create Groonga DB: <%s>",
				  tag,
				  path);
	}
	PGrnWRMCurrentDatabaseID = walRecord->dbID;
	PGrnWRMCurrentDatabaseTableSpaceID = walRecord->dbTableSpaceID;
}

static void
pgrnwrm_redo_teardown(PGrnWRMRedoData *data)
{
	/* if (!data->db) */
	/* 	return; */

	/* grn_obj_close(ctx, data->db); */
}

static void
pgrnwrm_redo_create_table(XLogReaderState *record)
{
	const char *tag = "[redo][create-table]";
	PGrnWALRecordRaw raw = {
		.data = XLogRecGetData(record),
		.size = XLogRecGetDataLen(record),
	};
	grn_obj typeName;
	grn_obj tokenizerName;
	grn_obj normalizersName;
	grn_obj tokenFiltersName;
	PGrnWALRecordCreateTable walRecord = {0};
	PGrnWRMRedoData data = {
		.walRecord = (PGrnWALRecordCommon *) &walRecord,
		.db = NULL,
	};
	GRN_TEXT_INIT(&typeName, GRN_OBJ_DO_SHALLOW_COPY);
	GRN_TEXT_INIT(&tokenizerName, GRN_OBJ_DO_SHALLOW_COPY);
	GRN_TEXT_INIT(&normalizersName, GRN_OBJ_DO_SHALLOW_COPY);
	GRN_TEXT_INIT(&tokenFiltersName, GRN_OBJ_DO_SHALLOW_COPY);
	walRecord.type = &typeName;
	walRecord.tokenizer = &tokenizerName;
	walRecord.normalizers = &normalizersName;
	walRecord.tokenFilters = &tokenFiltersName;
	PG_TRY();
	{
		grn_obj *type = NULL;
		PGrnWALRecordCreateTableRead(&walRecord, &raw);
		pgrnwrm_redo_setup(&data, tag);
		GRN_LOG(ctx,
				GRN_LOG_DEBUG,
				PGRN_TAG ": %s %x/%08X %u(%s)/%u/%u name=<%.*s> flags=<%u> "
						 "type=<%.*s> tokenizer=<%.*s> normalizers=<%.*s> "
						 "token-filters=<%.*s>",
				tag,
				LSN_FORMAT_ARGS(record->ReadRecPtr),
				walRecord.dbID,
				pg_encoding_to_char(walRecord.dbEncoding),
				walRecord.dbTableSpaceID,
				walRecord.indexTableSpaceID,
				(int) (walRecord.nameSize),
				walRecord.name,
				walRecord.flags,
				(int) GRN_TEXT_LEN(walRecord.type),
				GRN_TEXT_VALUE(walRecord.type),
				(int) GRN_TEXT_LEN(walRecord.tokenizer),
				GRN_TEXT_VALUE(walRecord.tokenizer),
				(int) GRN_TEXT_LEN(walRecord.normalizers),
				GRN_TEXT_VALUE(walRecord.normalizers),
				(int) GRN_TEXT_LEN(walRecord.tokenFilters),
				GRN_TEXT_VALUE(walRecord.tokenFilters));
		if (GRN_BULK_VSIZE(walRecord.type) > 0)
		{
			type = PGrnLookupWithSize(GRN_TEXT_VALUE(walRecord.type),
									  GRN_TEXT_LEN(walRecord.type),
									  ERROR);
		}
		{
#	ifdef GRN_OBJ_REMOVE_ENSURE
			grn_ctx_remove(ctx,
						   walRecord.name,
						   walRecord.nameSize,
						   GRN_OBJ_REMOVE_DEPENDENT | GRN_OBJ_REMOVE_ENSURE);
#	else
			grn_obj *table =
				grn_ctx_get(ctx, walRecord.name, walRecord.nameSize);
			if (table)
				grn_obj_remove_dependent(ctx, table);
#	endif
			PGrnCheck("%s failed to remove existing table: <%.*s>",
					  tag,
					  (int) walRecord.nameSize,
					  walRecord.name);
		}
		PGrnCreateTableRawWithSize(walRecord.indexTableSpaceID,
								   walRecord.name,
								   walRecord.nameSize,
								   walRecord.flags,
								   type,
								   walRecord.tokenizer,
								   walRecord.normalizers,
								   walRecord.tokenFilters);
		grn_obj_flush_only_opened(ctx, grn_ctx_db(ctx));
	}
	PG_FINALLY();
	{
		pgrnwrm_redo_teardown(&data);
		GRN_OBJ_FIN(ctx, &typeName);
		GRN_OBJ_FIN(ctx, &tokenizerName);
		GRN_OBJ_FIN(ctx, &normalizersName);
		GRN_OBJ_FIN(ctx, &tokenFiltersName);
	}
	PG_END_TRY();
}

static void
pgrnwrm_redo_create_column(XLogReaderState *record)
{
	const char *tag = "[redo][create-column]";
	PGrnWALRecordRaw raw = {
		.data = XLogRecGetData(record),
		.size = XLogRecGetDataLen(record),
	};
	grn_obj tableName;
	grn_obj typeName;
	PGrnWALRecordCreateColumn walRecord = {0};
	PGrnWRMRedoData data = {
		.walRecord = (PGrnWALRecordCommon *) &walRecord,
		.db = NULL,
	};
	GRN_TEXT_INIT(&tableName, GRN_OBJ_DO_SHALLOW_COPY);
	GRN_TEXT_INIT(&typeName, GRN_OBJ_DO_SHALLOW_COPY);
	walRecord.table = &tableName;
	walRecord.type = &typeName;
	PG_TRY();
	{
		grn_obj *table = NULL;
		grn_obj *type = NULL;

		PGrnWALRecordCreateColumnRead(&walRecord, &raw);

		pgrnwrm_redo_setup(&data, tag);
		GRN_LOG(ctx,
				GRN_LOG_DEBUG,
				PGRN_TAG
				": %s %X/%08X %u(%s)/%u/%u table=<%.*s> name=<%.*s> flags=<%u> "
				"type=<%.*s>",
				tag,
				LSN_FORMAT_ARGS(record->ReadRecPtr),
				walRecord.dbID,
				pg_encoding_to_char(walRecord.dbEncoding),
				walRecord.dbTableSpaceID,
				walRecord.indexTableSpaceID,
				(int) GRN_TEXT_LEN(walRecord.table),
				GRN_TEXT_VALUE(walRecord.table),
				(int) (walRecord.nameSize),
				walRecord.name,
				walRecord.flags,
				(int) GRN_TEXT_LEN(walRecord.type),
				GRN_TEXT_VALUE(walRecord.type));
		table = PGrnLookupWithSize(GRN_TEXT_VALUE(walRecord.table),
								   GRN_TEXT_LEN(walRecord.table),
								   ERROR);
		type = PGrnLookupWithSize(GRN_TEXT_VALUE(walRecord.type),
								  GRN_TEXT_LEN(walRecord.type),
								  ERROR);
		{
#	ifdef GRN_OBJ_REMOVE_ENSURE
			char fullColumnName[GRN_TABLE_MAX_KEY_SIZE];
			snprintf(fullColumnName,
					 GRN_TABLE_MAX_KEY_SIZE,
					 "%.*s.%.*s",
					 (int) GRN_TEXT_LEN(walRecord.table),
					 GRN_TEXT_VALUE(walRecord.table),
					 (int) (walRecord.nameSize),
					 walRecord.name);
			grn_ctx_remove(ctx,
						   fullColumnName,
						   strlen(fullColumnName),
						   GRN_OBJ_REMOVE_ENSURE);
#	else
			grn_obj *column =
				grn_obj_column(ctx, table, walRecord.name, walRecord.nameSize);
			if (column)
				grn_obj_remove(ctx, column);
#	endif
			PGrnCheck("%s failed to remove existing column: <%.*s>",
					  tag,
					  (int) walRecord.nameSize,
					  walRecord.name);
		}
		PGrnCreateColumnRawWithSize(walRecord.indexTableSpaceID,
									table,
									walRecord.name,
									walRecord.nameSize,
									walRecord.flags,
									type);
		grn_obj_flush_only_opened(ctx, grn_ctx_db(ctx));
	}
	PG_FINALLY();
	{
		pgrnwrm_redo_teardown(&data);
		GRN_OBJ_FIN(ctx, &tableName);
		GRN_OBJ_FIN(ctx, &typeName);
	}
	PG_END_TRY();
}

static void
pgrnwrm_redo_set_sources(XLogReaderState *record)
{
	const char *tag = "[redo][set-sources]";
	PGrnWALRecordRaw raw = {
		.data = XLogRecGetData(record),
		.size = XLogRecGetDataLen(record),
	};
	grn_obj columnName;
	grn_obj sourceNames;
	grn_obj sourceIDs;
	PGrnWALRecordSetSources walRecord = {0};
	PGrnWRMRedoData data = {
		.walRecord = (PGrnWALRecordCommon *) &walRecord,
		.db = NULL,
	};
	GRN_TEXT_INIT(&columnName, GRN_OBJ_DO_SHALLOW_COPY);
	GRN_TEXT_INIT(&sourceNames, GRN_OBJ_VECTOR);
	GRN_RECORD_INIT(&sourceIDs, GRN_OBJ_VECTOR, GRN_ID_NIL);
	walRecord.column = &columnName;
	walRecord.sourceNames = &sourceNames;
	PG_TRY();
	{
		grn_obj *column = NULL;
		uint32_t i;
		uint32_t nSourceNames;

		PGrnWALRecordSetSourcesRead(&walRecord, &raw);

		pgrnwrm_redo_setup(&data, tag);
		GRN_LOG(ctx,
				GRN_LOG_DEBUG,
				PGRN_TAG ": %s %X/%08X %u(%s)/%u column=<%.*s> sources=<%s>",
				tag,
				LSN_FORMAT_ARGS(record->ReadRecPtr),
				walRecord.dbID,
				pg_encoding_to_char(walRecord.dbEncoding),
				walRecord.dbTableSpaceID,
				(int) GRN_TEXT_LEN(walRecord.column),
				GRN_TEXT_VALUE(walRecord.column),
				PGrnInspect(walRecord.sourceNames));
		column = PGrnLookupWithSize(GRN_TEXT_VALUE(walRecord.column),
									GRN_TEXT_LEN(walRecord.column),
									ERROR);
		nSourceNames = grn_vector_size(ctx, walRecord.sourceNames);
		for (i = 0; i < nSourceNames; i++)
		{
			const char *name;
			uint32_t nameSize = grn_vector_get_element(
				ctx, walRecord.sourceNames, i, &name, NULL, NULL);
			grn_obj *source = PGrnLookupWithSize(name, nameSize, ERROR);
			grn_id sourceID = grn_obj_id(ctx, source);
			GRN_RECORD_PUT(ctx, &sourceIDs, sourceID);
		}

		PGrnIndexColumnSetSourceIDsRaw(column, &sourceIDs);
		grn_obj_flush_only_opened(ctx, grn_ctx_db(ctx));
	}
	PG_FINALLY();
	{
		pgrnwrm_redo_teardown(&data);
		GRN_OBJ_FIN(ctx, &columnName);
		GRN_OBJ_FIN(ctx, &sourceNames);
		GRN_OBJ_FIN(ctx, &sourceIDs);
	}
	PG_END_TRY();
}

static void
pgrnwrm_redo_rename_table(XLogReaderState *record)
{
	const char *tag = "[redo][rename-table]";
	PGrnWALRecordRaw raw = {
		.data = XLogRecGetData(record),
		.size = XLogRecGetDataLen(record),
	};
	PGrnWALRecordRenameTable walRecord = {0};
	PGrnWRMRedoData data = {
		.walRecord = (PGrnWALRecordCommon *) &walRecord,
		.db = NULL,
	};
	PG_TRY();
	{
		grn_obj *table;

		PGrnWALRecordRenameTableRead(&walRecord, &raw);

		pgrnwrm_redo_setup(&data, tag);
		GRN_LOG(ctx,
				GRN_LOG_DEBUG,
				PGRN_TAG ": %s %X/%08X %u(%s)/%u name=<%.*s> new-name=<%.*s>",
				tag,
				LSN_FORMAT_ARGS(record->ReadRecPtr),
				walRecord.dbID,
				pg_encoding_to_char(walRecord.dbEncoding),
				walRecord.dbTableSpaceID,
				(int) (walRecord.nameSize),
				walRecord.name,
				(int) (walRecord.newNameSize),
				walRecord.newName);
		{
#	ifdef GRN_OBJ_REMOVE_ENSURE
			grn_ctx_remove(ctx,
						   walRecord.newName,
						   walRecord.newNameSize,
						   GRN_OBJ_REMOVE_DEPENDENT | GRN_OBJ_REMOVE_ENSURE);
#	else
			grn_obj *table =
				grn_ctx_get(ctx, walRecord.newName, walRecord.newNameSize);
			if (table)
				grn_obj_remove_dependent(ctx, table);
#	endif
			PGrnCheck("%s failed to remove existing table: <%.*s>",
					  tag,
					  (int) walRecord.newNameSize,
					  walRecord.newName);
		}
		table = PGrnLookupWithSize(walRecord.name, walRecord.nameSize, ERROR);
		PGrnRenameTableRawWithSize(
			table, walRecord.newName, walRecord.newNameSize);
		grn_obj_flush_only_opened(ctx, grn_ctx_db(ctx));
	}
	PG_FINALLY();
	{
		pgrnwrm_redo_teardown(&data);
	}
	PG_END_TRY();
}

typedef struct PGrnWRNInsertTupleData
{
	grn_ctx *ctx;
	grn_obj *table;
	const char *tableName;
	uint32 tableNameSize;
	grn_obj *valueBuffer;
	PGrnWALTuple *tuple;
	const char *tag;
} PGrnWRNInsertTupleData;

static void
pgrnwrm_redo_insert_tuple(PGrnWRNInsertTupleData *data)
{
	grn_ctx *ctx = data->ctx;
	grn_obj *table = data->table;
	PGrnWALTuple *tuple = data->tuple;
	grn_obj *valueBuffer = data->valueBuffer;
	grn_id id = GRN_ID_NIL;
	uint32 i;

	for (i = 0; i < tuple->nColumns; i++)
	{
		const char *name;
		uint32_t nameSize;

		nameSize = grn_vector_get_element(
			ctx, tuple->columnNames, i, &name, NULL, NULL);
		if (i == 0)
		{
			if (nameSize == GRN_COLUMN_NAME_KEY_LEN &&
				memcmp(name, GRN_COLUMN_NAME_KEY, GRN_COLUMN_NAME_KEY_LEN) == 0)
			{
				const char *key;
				uint32_t keySize;
				keySize = grn_vector_get_element(
					ctx, tuple->columnValues, i, &key, NULL, NULL);
				id = grn_table_add(ctx, table, key, keySize, NULL);
				PGrnCheck("%s failed to add a record: <%.*s>: <%s>",
						  data->tag,
						  (int) (data->tableNameSize),
						  data->tableName,
						  PGrnInspectKey(table, key, keySize));
				continue;
			}
			else
			{
				id = grn_table_add(ctx, table, NULL, 0, NULL);
				PGrnCheck("%s failed to add a record: <%.*s>",
						  data->tag,
						  (int) (data->tableNameSize),
						  data->tableName);
			}
		}

		{
			grn_obj *column;
			grn_id vectorValueID;
			void *vectorValue;
			grn_obj *columnValue;

			column = grn_obj_column(ctx, table, name, nameSize);
			if (!column)
			{
				PGrnCheckRCLevel(GRN_INVALID_ARGUMENT,
								 ERROR,
								 "column isn't found: <%.*s>:<%.*s>",
								 (int) (data->tableNameSize),
								 data->tableName,
								 (int) nameSize,
								 name);
			}
			vectorValueID = grn_hash_get(ctx,
										 tuple->columnVectorValues,
										 &i,
										 sizeof(uint32),
										 &vectorValue);
			if (vectorValueID == GRN_ID_NIL)
			{
				const char *value;
				uint32_t valueSize;
				grn_id domain;
				valueSize = grn_vector_get_element(
					ctx, tuple->columnValues, i, &value, NULL, &domain);
				GRN_OBJ_FIN(ctx, valueBuffer);
				GRN_OBJ_INIT(
					valueBuffer, GRN_BULK, GRN_OBJ_DO_SHALLOW_COPY, domain);
				PGrnCheck("%s failed to initialize value buffer: "
						  "<%.*s.%.*s>: <%u>",
						  data->tag,
						  (int) (data->tableNameSize),
						  data->tableName,
						  (int) nameSize,
						  name,
						  id);
				GRN_TEXT_SET_REF(valueBuffer, value, valueSize);
				columnValue = valueBuffer;
			}
			else
			{
				columnValue = vectorValue;
			}
			grn_obj_set_value(ctx, column, id, columnValue, GRN_OBJ_SET);
			if (ctx->rc != GRN_SUCCESS)
			{
				PGrnCheck("%s failed to set a column value: "
						  "<%.*s.%.*s>: <%u>: <%s>",
						  data->tag,
						  (int) (data->tableNameSize),
						  data->tableName,
						  (int) nameSize,
						  name,
						  id,
						  PGrnInspect(columnValue));
			}
		}
	}
}

static void
pgrnwrm_column_vector_values_clear(grn_ctx *ctx, grn_hash *values)
{
	GRN_HASH_EACH_BEGIN(ctx, values, cursor, id)
	{
		void *v;
		grn_obj *value;
		grn_hash_cursor_get_value(ctx, cursor, &v);
		value = v;
		GRN_OBJ_FIN(ctx, value);
		grn_hash_cursor_delete(ctx, cursor, NULL);
	}
	GRN_HASH_EACH_END(ctx, cursor);
}

static void
pgrnwrm_redo_insert(XLogReaderState *record)
{
	const char *tag = "[redo][insert]";
	PGrnWALRecordRaw raw = {
		.data = XLogRecGetData(record),
		.size = XLogRecGetDataLen(record),
	};
	grn_hash *columnVectorValues;
	grn_obj columnNames;
	grn_obj columnValues;
	grn_obj valueBuffer;
	PGrnWALRecordInsert walRecord = {0};
	PGrnWRMRedoData data = {
		.walRecord = (PGrnWALRecordCommon *) &walRecord,
		.db = NULL,
	};
	columnVectorValues = grn_hash_create(
		ctx, NULL, sizeof(uint32), sizeof(grn_obj), GRN_TABLE_HASH_KEY);
	PGrnCheck("%s failed to create a buffer for column vector values", tag);
	GRN_TEXT_INIT(&columnNames, GRN_OBJ_VECTOR);
	GRN_TEXT_INIT(&columnValues, GRN_OBJ_VECTOR);
	GRN_VOID_INIT(&valueBuffer);
	walRecord.tuple.columnNames = &columnNames;
	walRecord.tuple.columnValues = &columnValues;
	walRecord.tuple.columnVectorValues = columnVectorValues;
	PG_TRY();
	{
		PGrnWRNInsertTupleData insertData = {0};
		insertData.ctx = ctx;
		insertData.valueBuffer = &valueBuffer;
		insertData.tag = tag;

		PGrnWALRecordInsertRead(&walRecord, &raw);

		pgrnwrm_redo_setup(&data, tag);
		GRN_LOG(ctx,
				GRN_LOG_DEBUG,
				PGRN_TAG ": %s %X/%08X %u(%s)/%u table=<%.*s> columns=<%s>",
				tag,
				LSN_FORMAT_ARGS(record->ReadRecPtr),
				walRecord.dbID,
				pg_encoding_to_char(walRecord.dbEncoding),
				walRecord.dbTableSpaceID,
				(int) (walRecord.tableNameSize),
				walRecord.tableName,
				PGrnInspect(walRecord.tuple.columnNames));
		insertData.table = PGrnLookupWithSize(
			walRecord.tableName, walRecord.tableNameSize, ERROR);
		insertData.tableName = walRecord.tableName;
		insertData.tableNameSize = walRecord.tableNameSize;
		insertData.tuple = &(walRecord.tuple);
		pgrnwrm_redo_insert_tuple(&insertData);
		grn_db_touch(ctx, grn_ctx_db(ctx));
		grn_obj_flush_only_opened(ctx, grn_ctx_db(ctx));
	}
	PG_FINALLY();
	{
		pgrnwrm_redo_teardown(&data);
		GRN_OBJ_FIN(ctx, &columnNames);
		GRN_OBJ_FIN(ctx, &columnValues);
		GRN_OBJ_FIN(ctx, &valueBuffer);
		pgrnwrm_column_vector_values_clear(ctx, columnVectorValues);
		grn_hash_close(ctx, columnVectorValues);
	}
	PG_END_TRY();
}

static void
pgrnwrm_redo_delete(XLogReaderState *record)
{
	const char *tag = "[redo][delete]";
	PGrnWALRecordRaw raw = {
		.data = XLogRecGetData(record),
		.size = XLogRecGetDataLen(record),
	};
	PGrnWALRecordDelete walRecord = {0};
	PGrnWRMRedoData data = {
		.walRecord = (PGrnWALRecordCommon *) &walRecord,
		.db = NULL,
	};
	PG_TRY();
	{
		grn_obj *table;

		PGrnWALRecordDeleteRead(&walRecord, &raw);

		pgrnwrm_redo_setup(&data, tag);
		table = PGrnLookupWithSize(
			walRecord.tableName, walRecord.tableNameSize, ERROR);
		GRN_LOG(ctx,
				GRN_LOG_DEBUG,
				PGRN_TAG ": %s %X/%08X %u(%s)/%u %s",
				tag,
				LSN_FORMAT_ARGS(record->ReadRecPtr),
				walRecord.dbID,
				pg_encoding_to_char(walRecord.dbEncoding),
				walRecord.dbTableSpaceID,
				PGrnInspectKey(table, walRecord.key, walRecord.keySize));
		if (table->header.type == GRN_TABLE_NO_KEY)
		{
			const uint64_t packedCtid = *((uint64_t *) (walRecord.key));
			grn_obj *ctidColumn =
				grn_obj_column(ctx, table, "ctid", strlen("ctid"));
			grn_obj ctidValue;
			GRN_UINT64_INIT(&ctidValue, 0);
			GRN_TABLE_EACH_BEGIN(ctx, table, cursor, id)
			{
				GRN_BULK_REWIND(&ctidValue);
				grn_obj_get_value(ctx, ctidColumn, id, &ctidValue);
				if (packedCtid == GRN_UINT64_VALUE(&ctidValue))
				{
					grn_table_cursor_delete(ctx, cursor);
					break;
				}
			}
			GRN_TABLE_EACH_END(ctx, cursor);
			GRN_OBJ_FIN(ctx, &ctidValue);
		}
		else
		{
			grn_table_delete(ctx, table, walRecord.key, walRecord.keySize);
		}
		PGrnCheck("%s failed to delete a record: %s",
				  tag,
				  PGrnInspectKey(table, walRecord.key, walRecord.keySize));
		grn_db_touch(ctx, grn_ctx_db(ctx));
		grn_obj_flush_only_opened(ctx, grn_ctx_db(ctx));
	}
	PG_FINALLY();
	{
		pgrnwrm_redo_teardown(&data);
	}
	PG_END_TRY();
}

static void
pgrnwrm_redo_remove_object(XLogReaderState *record)
{
	const char *tag = "[redo][remove-object]";
	PGrnWALRecordRaw raw = {
		.data = XLogRecGetData(record),
		.size = XLogRecGetDataLen(record),
	};
	PGrnWALRecordRemoveObject walRecord = {0};
	PGrnWRMRedoData data = {
		.walRecord = (PGrnWALRecordCommon *) &walRecord,
		.db = NULL,
	};
	PG_TRY();
	{
		grn_obj *object;

		PGrnWALRecordRemoveObjectRead(&walRecord, &raw);

		pgrnwrm_redo_setup(&data, tag);
		GRN_LOG(ctx,
				GRN_LOG_DEBUG,
				PGRN_TAG ": %s %X/%08X %u(%s)/%u name=<%.*s>",
				tag,
				LSN_FORMAT_ARGS(record->ReadRecPtr),
				walRecord.dbID,
				pg_encoding_to_char(walRecord.dbEncoding),
				walRecord.dbTableSpaceID,
				(int) (walRecord.nameSize),
				walRecord.name);
		object = PGrnLookupWithSize(
			walRecord.name, walRecord.nameSize, PGRN_ERROR_LEVEL_IGNORE);
		if (object)
		{
			grn_obj_remove(ctx, object);
			PGrnCheck("%s failed to remove: <%.*s>",
					  tag,
					  (int) (walRecord.nameSize),
					  walRecord.name);
			grn_obj_flush_only_opened(ctx, grn_ctx_db(ctx));
		}
	}
	PG_FINALLY();
	{
		pgrnwrm_redo_teardown(&data);
	}
	PG_END_TRY();
}

static void
pgrnwrm_redo_register_plugin(XLogReaderState *record)
{
	const char *tag = "[redo][register-plugin]";
	PGrnWALRecordRaw raw = {
		.data = XLogRecGetData(record),
		.size = XLogRecGetDataLen(record),
	};
	PGrnWALRecordRegisterPlugin walRecord = {0};
	PGrnWRMRedoData data = {
		.walRecord = (PGrnWALRecordCommon *) &walRecord,
		.db = NULL,
	};
	PG_TRY();
	{
		PGrnWALRecordRegisterPluginRead(&walRecord, &raw);

		pgrnwrm_redo_setup(&data, tag);
		GRN_LOG(ctx,
				GRN_LOG_DEBUG,
				PGRN_TAG ": %s %X/%08X %u(%s)/%u name=<%.*s>",
				tag,
				LSN_FORMAT_ARGS(record->ReadRecPtr),
				walRecord.dbID,
				pg_encoding_to_char(walRecord.dbEncoding),
				walRecord.dbTableSpaceID,
				(int) (walRecord.nameSize),
				walRecord.name);
		PGrnRegisterPluginWithSize(walRecord.name, walRecord.nameSize, tag);
		grn_obj_flush_only_opened(ctx, grn_ctx_db(ctx));
	}
	PG_FINALLY();
	{
		pgrnwrm_redo_teardown(&data);
	}
	PG_END_TRY();
}

static void
pgrnwrm_redo_bulk_insert(XLogReaderState *record)
{
	const char *tag = "[redo][bulk-insert]";
	PGrnWALRecordRaw raw = {
		.data = XLogRecGetData(record),
		.size = XLogRecGetDataLen(record),
	};
	grn_hash *columnVectorValues;
	grn_obj columnNames;
	grn_obj columnValues;
	grn_obj valueBuffer;
	PGrnWALRecordBulkInsert walRecord = {0};
	PGrnWRMRedoData data = {
		.walRecord = (PGrnWALRecordCommon *) &walRecord,
		.db = NULL,
	};
	columnVectorValues = grn_hash_create(
		ctx, NULL, sizeof(uint32), sizeof(grn_obj), GRN_TABLE_HASH_KEY);
	PGrnCheck("%s failed to create a buffer for column vector values", tag);
	GRN_TEXT_INIT(&columnNames, GRN_OBJ_VECTOR);
	GRN_TEXT_INIT(&columnValues, GRN_OBJ_VECTOR);
	GRN_VOID_INIT(&valueBuffer);
	walRecord.tuple.columnNames = &columnNames;
	walRecord.tuple.columnValues = &columnValues;
	walRecord.tuple.columnVectorValues = columnVectorValues;
	PG_TRY();
	{
		PGrnWRNInsertTupleData insertData = {0};
		insertData.ctx = ctx;
		insertData.valueBuffer = &valueBuffer;
		insertData.tag = tag;

		PGrnWALRecordBulkInsertRead(&walRecord, &raw);

		pgrnwrm_redo_setup(&data, tag);
		GRN_LOG(ctx,
				GRN_LOG_DEBUG,
				PGRN_TAG ": %s %X/%08X %u(%s)/%u table=<%.*s> columns=<%s>",
				tag,
				LSN_FORMAT_ARGS(record->ReadRecPtr),
				walRecord.dbID,
				pg_encoding_to_char(walRecord.dbEncoding),
				walRecord.dbTableSpaceID,
				(int) (walRecord.tableNameSize),
				walRecord.tableName,
				PGrnInspect(walRecord.tuple.columnNames));
		insertData.table = PGrnLookupWithSize(
			walRecord.tableName, walRecord.tableNameSize, ERROR);
		insertData.tableName = walRecord.tableName;
		insertData.tableNameSize = walRecord.tableNameSize;
		while (PGrnWALRecordBulkInsertReadNext(&walRecord, &raw))
		{
			insertData.tuple = &(walRecord.tuple);
			pgrnwrm_redo_insert_tuple(&insertData);
			GRN_BULK_REWIND(&columnNames);
			GRN_BULK_REWIND(&columnValues);
			GRN_BULK_REWIND(&valueBuffer);
			pgrnwrm_column_vector_values_clear(ctx, columnVectorValues);
		}
		grn_db_touch(ctx, grn_ctx_db(ctx));
		grn_obj_flush_only_opened(ctx, grn_ctx_db(ctx));
	}
	PG_FINALLY();
	{
		pgrnwrm_redo_teardown(&data);
		GRN_OBJ_FIN(ctx, &columnNames);
		GRN_OBJ_FIN(ctx, &columnValues);
		GRN_OBJ_FIN(ctx, &valueBuffer);
		pgrnwrm_column_vector_values_clear(ctx, columnVectorValues);
		grn_hash_close(ctx, columnVectorValues);
	}
	PG_END_TRY();
}

static const char *
pgrnwrm_info_to_string(uint8 info)
{
	switch (info & XLR_RMGR_INFO_MASK)
	{
	case PGRN_WAL_RECORD_CREATE_TABLE:
		return "CREATE_TABLE";
	case PGRN_WAL_RECORD_CREATE_COLUMN:
		return "CREATE_COLUMN";
	case PGRN_WAL_RECORD_SET_SOURCES:
		return "SET_SOURCES";
	case PGRN_WAL_RECORD_RENAME_TABLE:
		return "RENAME_TABLE";
	case PGRN_WAL_RECORD_INSERT:
		return "INSERT";
	case PGRN_WAL_RECORD_DELETE:
		return "DELETE";
	case PGRN_WAL_RECORD_REMOVE_OBJECT:
		return "REMOVE_OBJECT";
	case PGRN_WAL_RECORD_REGISTER_PLUGIN:
		return "REGISTER_PLUGIN";
	case PGRN_WAL_RECORD_BULK_INSERT:
		return "BULK_INSERT";
	default:
		return "UNKNOWN";
	}
}

static void
pgrnwrm_redo(XLogReaderState *record)
{
	uint8 info;

	if (!StandbyMode)
		return;

	info = XLogRecGetInfo(record) & XLR_RMGR_INFO_MASK;
	GRN_LOG(ctx,
			GRN_LOG_DEBUG,
			PGRN_TAG ": [redo] <%s>(%u): <%u>",
			pgrnwrm_info_to_string(info),
			info,
			XLogRecGetDataLen(record));
	switch (info)
	{
	case PGRN_WAL_RECORD_CREATE_TABLE:
		pgrnwrm_redo_create_table(record);
		break;
	case PGRN_WAL_RECORD_CREATE_COLUMN:
		pgrnwrm_redo_create_column(record);
		break;
	case PGRN_WAL_RECORD_SET_SOURCES:
		pgrnwrm_redo_set_sources(record);
		break;
	case PGRN_WAL_RECORD_RENAME_TABLE:
		pgrnwrm_redo_rename_table(record);
		break;
	case PGRN_WAL_RECORD_INSERT:
		pgrnwrm_redo_insert(record);
		break;
	case PGRN_WAL_RECORD_DELETE:
		pgrnwrm_redo_delete(record);
		break;
	case PGRN_WAL_RECORD_REMOVE_OBJECT:
		pgrnwrm_redo_remove_object(record);
		break;
	case PGRN_WAL_RECORD_REGISTER_PLUGIN:
		pgrnwrm_redo_register_plugin(record);
		break;
	case PGRN_WAL_RECORD_BULK_INSERT:
		pgrnwrm_redo_bulk_insert(record);
		break;
	default:
		ereport(ERROR,
				errcode(ERRCODE_DATA_EXCEPTION),
				errmsg(PGRN_TAG ": [redo] unknown info %u", info));
		break;
	}
}

static void
pgrnwrm_desc(StringInfo buffer, XLogReaderState *record)
{
	uint8 info;

	if (StandbyMode)
		GRN_LOG(ctx, GRN_LOG_DEBUG, PGRN_TAG ": [desc]");

	appendStringInfo(buffer, "rmid: %d", XLogRecGetRmid(record));
	/* TODO: Add more information */
	info = XLogRecGetInfo(record) & XLR_RMGR_INFO_MASK;
	switch (info)
	{
	case PGRN_WAL_RECORD_CREATE_TABLE:
		appendStringInfo(buffer, " action: create-table");
		break;
	case PGRN_WAL_RECORD_CREATE_COLUMN:
		appendStringInfo(buffer, " action: create-column");
		break;
	case PGRN_WAL_RECORD_SET_SOURCES:
		appendStringInfo(buffer, " action: set-sources");
		break;
	case PGRN_WAL_RECORD_RENAME_TABLE:
		appendStringInfo(buffer, " action: rename-table");
		break;
	case PGRN_WAL_RECORD_INSERT:
		appendStringInfo(buffer, " action: insert");
		break;
	case PGRN_WAL_RECORD_DELETE:
		appendStringInfo(buffer, " action: delete");
		break;
	case PGRN_WAL_RECORD_REMOVE_OBJECT:
		appendStringInfo(buffer, " action: remove-object");
		break;
	case PGRN_WAL_RECORD_REGISTER_PLUGIN:
		appendStringInfo(buffer, " action: register-plugin");
		break;
	case PGRN_WAL_RECORD_BULK_INSERT:
		appendStringInfo(buffer, " action: bulk-insert");
		break;
	default:
		appendStringInfo(buffer, " action: unknown(%u)", info);
		break;
	}
}

static const char *
pgrnwrm_identify(uint8 info)
{
	if (StandbyMode)
		GRN_LOG(ctx, GRN_LOG_DEBUG, PGRN_TAG ": [identify] <%u>", info);
	return pgrnwrm_info_to_string(info);
}

static void
pgrnwrm_startup(void)
{
	if (!StandbyMode)
		return;

	grn_thread_set_get_limit_func(pgrnwrm_get_thread_limit, NULL);
	grn_default_logger_set_flags(grn_default_logger_get_flags() | GRN_LOG_PID);
	grn_default_logger_set_max_level(PGrnWRMLogLevel);
	if (!PGrnIsNoneValue(PGrnWRMLogPath))
	{
		grn_default_logger_set_path(PGrnWRMLogPath);
	}

	if (grn_init() != GRN_SUCCESS)
	{
		ereport(ERROR,
				(errcode(ERRCODE_SYSTEM_ERROR),
				 errmsg(PGRN_TAG ": failed to initialize Groonga")));
	}

	grn_set_segv_handler();
	grn_set_abrt_handler();

	{
		grn_rc rc = grn_ctx_init(ctx, 0);
		if (rc != GRN_SUCCESS)
		{
			ereport(
				ERROR,
				(errcode(PGrnGrnRCToPGErrorCode(rc)),
				 errmsg(PGRN_TAG ": failed to initialize Groonga context: %d",
						rc)));
		}
		grn_ctx_set_wal_role(ctx, GRN_WAL_ROLE_PRIMARY);
	}

	GRN_LOG(ctx, GRN_LOG_NOTICE, PGRN_TAG ": startup: <%s>", PGRN_VERSION);

	GRN_TEXT_INIT(&PGrnInspectBuffer, 0);
}

static void
pgrnwrm_cleanup(void)
{
	grn_obj *db;

	if (!StandbyMode)
		return;

	GRN_LOG(ctx, GRN_LOG_NOTICE, PGRN_TAG ": cleanup");
	GRN_OBJ_FIN(ctx, &PGrnInspectBuffer);
	db = grn_ctx_db(ctx);
	if (db)
		grn_obj_close(ctx, db);
	grn_ctx_fin(ctx);
	grn_fin();
}

static RmgrData data = {
	.rm_name = "PGroonga",
	.rm_redo = pgrnwrm_redo,
	.rm_desc = pgrnwrm_desc,
	.rm_identify = pgrnwrm_identify,
	.rm_startup = pgrnwrm_startup,
	.rm_cleanup = pgrnwrm_cleanup,
	.rm_mask = NULL,
	.rm_decode = NULL,
};
#endif

void
_PG_init(void)
{
	DefineCustomStringVariable("pgroonga_wal_resource_manager.log_path",
							   "Log path for pgroonga-wal-resource-manager.",
							   "The default is "
							   "\"${PG_DATA}/" PGrnLogPathDefault "\". "
							   "Use \"none\" to disable file output.",
							   &PGrnWRMLogPath,
							   PGrnLogPathDefault,
							   PGC_USERSET,
							   0,
							   NULL,
							   NULL,
							   NULL);

	DefineCustomEnumVariable("pgroonga_wal_resource_manager.log_level",
							 "Log level for pgroonga-wal-resource-manager.",
							 "Available log levels: "
							 "[none, emergency, alert, critical, "
							 "error, warning, notice, info, debug, dump]. "
							 "The default is notice.",
							 &PGrnWRMLogLevel,
							 GRN_LOG_DEFAULT_LEVEL,
							 PGrnWRMLogLevelEntries,
							 PGC_USERSET,
							 0,
							 NULL,
							 NULL,
							 NULL);

#ifdef PGRN_SUPPORT_WAL_RESOURCE_MANAGER
	RegisterCustomRmgr(PGRN_WAL_RESOURCE_MANAGER_ID, &data);
#endif
}
