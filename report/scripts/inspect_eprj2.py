import json
import os
import sqlite3
import sys


def summarize(value):
    if isinstance(value, bytes):
        return {"type": "blob", "length": len(value), "head_hex": value[:16].hex()}
    text = "" if value is None else str(value)
    return text if len(text) <= 500 else text[:500] + "…"


path = sys.argv[1]
connection = sqlite3.connect(f"file:{path}?mode=ro", uri=True)
connection.row_factory = sqlite3.Row
tables = connection.execute(
    "SELECT name, sql FROM sqlite_master WHERE type='table' ORDER BY name"
).fetchall()

for table in tables:
    name = table["name"]
    escaped = name.replace('"', '""')
    columns = connection.execute(f'PRAGMA table_info("{escaped}")').fetchall()
    count = connection.execute(f'SELECT COUNT(*) FROM "{escaped}"').fetchone()[0]
    print(f"\n=== {name} ({count} rows) ===")
    print("columns:", ", ".join(f"{column['name']}:{column['type']}" for column in columns))
    for row in connection.execute(f'SELECT * FROM "{escaped}" LIMIT 3'):
        print(json.dumps({key: summarize(row[key]) for key in row.keys()}, ensure_ascii=False))
