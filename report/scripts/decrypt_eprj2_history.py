import argparse
import base64
import gzip
import json
import sqlite3
from pathlib import Path

from cryptography.hazmat.primitives.ciphers.aead import AESGCM


def decrypt_record(encoded, key_hex, iv_hex):
    encrypted = base64.b64decode(encoded)
    compressed = AESGCM(bytes.fromhex(key_hex)).decrypt(bytes.fromhex(iv_hex), encrypted, None)
    return gzip.decompress(compressed).decode("utf-8")


parser = argparse.ArgumentParser()
parser.add_argument("project")
parser.add_argument("--history-id", type=int)
parser.add_argument("--output")
args = parser.parse_args()

connection = sqlite3.connect(f"file:{args.project}?mode=ro", uri=True)
connection.row_factory = sqlite3.Row
project = connection.execute("SELECT uuid, name, branch_uuid FROM projects LIMIT 1").fetchone()
table = "project_history_" + project["branch_uuid"].replace("-", "_")
if args.history_id:
    history = connection.execute(f'SELECT * FROM "{table}" WHERE id = ?', (args.history_id,)).fetchone()
else:
    history = connection.execute(f'SELECT * FROM "{table}" ORDER BY id DESC LIMIT 1').fetchone()

print(json.dumps({"project": dict(project), "history": dict(history)}, ensure_ascii=False, indent=2))
chunks = []
for index in range(history["num"] + 1):
    record_uuid = history["uuid"] if index == 0 else f'{history["uuid"]}-{index}'
    record = connection.execute(
        "SELECT dataStr FROM history_data WHERE uuid = ?", (record_uuid,)
    ).fetchone()
    if record is None:
        raise RuntimeError(f"Missing history chunk: {record_uuid}")
    text = decrypt_record(record["dataStr"], history["key"], history["uuid"])
    chunks.append(text)
    print(f"\n--- CHUNK {index} · {record_uuid} · {len(text)} chars ---")
    print(text[:1200])

if args.output:
    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    for index, text in enumerate(chunks):
        (output / f"history-{history['id']}-chunk-{index}.txt").write_text(text, encoding="utf-8")
    print(f"\nWrote {len(chunks)} chunks to {output}")
