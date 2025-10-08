VALUES TEST.json_ingest('
    {
        "object_type": "table",
        "object_id": "table.queries",
        "object_ref": "1000",
        "table": [
            {
                "name": "query_id",
                "datatype": "%%INTEGER%%",
                "nullable": false,
                "primary_key": true,
                "unique": true
            }
        ]
    }
')@
