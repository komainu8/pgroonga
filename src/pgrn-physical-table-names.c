#include "pgroonga.h"

#include "pgrn-groonga.h"
#include "pgrn-physical-table-names.h"

#include <catalog/pg_inherits.h>
#include <storage/lmgr.h>
#include <utils/builtins.h>
#include <utils/lsyscache.h>

PGDLLEXPORT PG_FUNCTION_INFO_V1(pgroonga_physical_table_names);

void
PGrnInitializePhysicalTableNames(void)
{
	;
}

void
PGrnFinalizePhysicalTableNames(void)
{
	;
}

static bool
PGrnRelationIsPartitionedIndex(Relation relation)
{
	return PGRN_RELKIND_HAS_PARTITIONS(RelationGetForm(relation)->relkind);
}

/**
 * pgroonga_physical_table_name(indexName text, argument_prefix text) : array
 *
 */
Datum
pgroonga_physical_table_names(PG_FUNCTION_ARGS)
{
	const char *tag = "[pyhsical-table-names]";
	text *logical_index_name_text = PG_GETARG_TEXT_PP(0);
	text *argument_prefix = PG_GETARG_TEXT_PP(1);

	Datum logical_index_oid_datum = DirectFunctionCall1(
		regclassin, CStringGetDatum(text_to_cstring(logical_index_name_text)));
	if (!OidIsValid(logical_index_oid_datum))
	{
		PGrnCheckRC(GRN_INVALID_ARGUMENT,
					"%s nonexistent index name: <%s>",
					tag,
					text_to_cstring(logical_index_name_text));
	}
	Oid logical_index_oid = DatumGetObjectId(logical_index_oid_datum);
	if (!PGrnRelationIsPartitionedIndex(
			RelationIdGetRelation(logical_index_oid)))
	{
		PGrnCheckRC(GRN_INVALID_ARGUMENT,
					"%s the specified index is not partitioned index: <%s>",
					tag,
					text_to_cstring(logical_index_name_text));
	}

	LockRelationOid(logical_index_oid, AccessShareLock);
	List *physical_index_oids =
		find_inheritance_children(logical_index_oid, NoLock);
	ListCell *cell;

	List *physical_table_names = NIL;
	unsigned int i = 0;
	foreach (cell, physical_index_oids)
	{
		char logical_select_argument_name[256];
		snprintf(logical_select_argument_name,
				 sizeof(logical_select_argument_name),
				 "%s[%d].table",
				 text_to_cstring(argument_prefix),
				 i++);
		physical_table_names =
			lappend(physical_table_names,
					(void *) (cstring_to_text(logical_select_argument_name)));

		if (list_length(physical_table_names) == 0)
			PG_RETURN_POINTER(construct_empty_array(TEXTOID));

		Oid physical_index_oid = lfirst_oid(cell);
		if (physical_index_oid == logical_index_oid)
			continue;
		char *physical_index_name = get_rel_name(physical_index_oid);
		char table_name_buffer[GRN_TABLE_MAX_KEY_SIZE];
		text *source_table_name;

		PGrnFormatSourcesTableName(physical_index_name, table_name_buffer);
		source_table_name = cstring_to_text(table_name_buffer);
		physical_table_names =
			lappend(physical_table_names, (void *) (source_table_name));
	}
	UnlockRelationOid(logical_index_oid, AccessShareLock);

	unsigned int n_elements = list_length(physical_table_names);
	Datum *physical_table_names_datum = palloc(n_elements * sizeof(Datum));
	i = 0;
	foreach (cell, physical_table_names)
	{
		physical_table_names_datum[i++] = (Datum) lfirst(cell);
	}

	int dims[1] = {n_elements};
	int lbs[1] = {1};

	PG_RETURN_ARRAYTYPE_P(construct_md_array(physical_table_names_datum,
											 NULL,
											 1,
											 dims,
											 lbs,
											 TEXTOID,
											 -1,
											 false,
											 TYPALIGN_INT));
}
