import sqlite3
import sys
from pathlib import Path

DB_PATH = Path(__file__).resolve().parent / "moodcanvas_cloud.db"
USERNAME = sys.argv[1] if len(sys.argv) > 1 else "fyk"

conn = sqlite3.connect(DB_PATH)
conn.row_factory = sqlite3.Row
cur = conn.cursor()

row = cur.execute(
    "SELECT id, username, nickname FROM users WHERE username = ? COLLATE NOCASE",
    (USERNAME,),
).fetchone()

if not row:
    print(f"NOT_FOUND: {USERNAME}")
else:
    uid = row["id"]
    cur.execute("DELETE FROM sessions WHERE user_id = ?", (uid,))
    cur.execute("DELETE FROM treehole_replies WHERE user_id = ?", (uid,))
    cur.execute("DELETE FROM treehole_posts WHERE user_id = ?", (uid,))
    cur.execute("DELETE FROM users WHERE id = ?", (uid,))
    conn.commit()
    print(f"RESET_OK: {row['username']} ({row['nickname']})")

remaining = [r[0] for r in cur.execute("SELECT username FROM users").fetchall()]
print("REMAINING:", remaining)
conn.close()
