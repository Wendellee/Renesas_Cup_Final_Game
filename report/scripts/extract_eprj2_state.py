import argparse
import base64
import collections
import gzip
import json
import sqlite3
from pathlib import Path

from cryptography.hazmat.primitives.ciphers.aead import AESGCM


def decrypt_record(encoded, key_hex, iv_hex):
    encrypted = base64.b64decode(encoded)
    compressed = AESGCM(bytes.fromhex(key_hex)).decrypt(bytes.fromhex(iv_hex), encrypted, None)
    return gzip.decompress(compressed).decode("utf-8")


def parse_edit_line(line):
    if "||" not in line:
        return None
    header_text, payload_text = line.split("||", 1)
    try:
        header = json.loads(header_text)
    except json.JSONDecodeError:
        return None
    if payload_text.endswith("|"):
        payload_text = payload_text[:-1]
    payload = None
    if payload_text:
        try:
            payload = json.loads(payload_text)
        except json.JSONDecodeError:
            payload = payload_text
    return header, payload


parser = argparse.ArgumentParser()
parser.add_argument("project")
parser.add_argument("output")
args = parser.parse_args()

output = Path(args.output)
output.mkdir(parents=True, exist_ok=True)
connection = sqlite3.connect(f"file:{args.project}?mode=ro", uri=True)
connection.row_factory = sqlite3.Row
project = connection.execute("SELECT uuid, name, branch_uuid FROM projects LIMIT 1").fetchone()
branch = connection.execute("SELECT history_uuid FROM branches WHERE uuid = ?", (project["branch_uuid"],)).fetchone()
table = "project_history_" + project["branch_uuid"].replace("-", "_")

history_by_uuid = {
    row["uuid"]: row
    for row in connection.execute(f'SELECT * FROM "{table}" ORDER BY id')
}
current_uuid = branch["history_uuid"]
chain = []
visited = set()
while current_uuid:
    if current_uuid in visited:
        raise RuntimeError(f"History loop detected at {current_uuid}")
    visited.add(current_uuid)
    history = history_by_uuid.get(current_uuid)
    if history is None:
        raise RuntimeError(f"Missing history node: {current_uuid}")
    chain.append(history)
    current_uuid = history["parent"]
chain.reverse()

state = {}
applied_lines = 0
for history in chain:
    for index in range(history["num"] + 1):
        record_uuid = history["uuid"] if index == 0 else f'{history["uuid"]}-{index}'
        record = connection.execute(
            "SELECT dataStr FROM history_data WHERE uuid = ?", (record_uuid,)
        ).fetchone()
        if record is None:
            raise RuntimeError(f"Missing history chunk: {record_uuid}")
        text = decrypt_record(record["dataStr"], history["key"], history["uuid"])
        for line in text.splitlines():
            parsed = parse_edit_line(line)
            if parsed is None:
                continue
            header, payload = parsed
            element_type = header.get("type", "UNKNOWN")
            if element_type == "EDIT_HEAD":
                continue
            identifier = header.get("id", element_type)
            key = json.dumps([element_type, identifier], ensure_ascii=False, sort_keys=True)
            if payload is None:
                state.pop(key, None)
            else:
                state[key] = {"header": header, "payload": payload}
            applied_lines += 1

elements = list(state.values())
elements.sort(key=lambda item: (item["header"].get("type", ""), str(item["header"].get("id", ""))))
type_counts = collections.Counter(item["header"].get("type", "UNKNOWN") for item in elements)

(output / "current-state.json").write_text(
    json.dumps({"project": dict(project), "history_nodes": len(chain), "elements": elements}, ensure_ascii=False, indent=2),
    encoding="utf-8",
)
(output / "current-state.jsonl").write_text(
    "\n".join(json.dumps(item, ensure_ascii=False) for item in elements) + "\n",
    encoding="utf-8",
)
(output / "type-counts.json").write_text(
    json.dumps(type_counts, ensure_ascii=False, indent=2), encoding="utf-8"
)

print(json.dumps({
    "project": dict(project),
    "history_nodes": len(chain),
    "applied_lines": applied_lines,
    "current_elements": len(elements),
    "type_counts": type_counts,
}, ensure_ascii=False, indent=2))
