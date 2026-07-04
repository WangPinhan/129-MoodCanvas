"""
MoodCanvas 云端服务：用户注册/登录 + 树洞多人互动。
默认监听 8765 端口，数据保存在同目录 moodcanvas_cloud.db。
"""

from __future__ import annotations

import hashlib
import secrets
import sqlite3
import uuid
from contextlib import contextmanager
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional

from fastapi import Depends, FastAPI, Header, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field

DB_PATH = Path(__file__).resolve().parent / "moodcanvas_cloud.db"

app = FastAPI(title="MoodCanvas Cloud", version="1.0.0")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat()


def hash_password(password: str, salt: str) -> str:
    return hashlib.sha256((salt + password).encode("utf-8")).hexdigest()


@contextmanager
def db_conn():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    try:
        yield conn
        conn.commit()
    finally:
        conn.close()


def init_db() -> None:
    with db_conn() as conn:
        conn.executescript(
            """
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT NOT NULL COLLATE NOCASE UNIQUE,
                password_hash TEXT NOT NULL,
                salt TEXT NOT NULL,
                nickname TEXT NOT NULL,
                created_at TEXT NOT NULL
            );
            CREATE TABLE IF NOT EXISTS sessions (
                token TEXT PRIMARY KEY,
                user_id INTEGER NOT NULL,
                created_at TEXT NOT NULL,
                FOREIGN KEY(user_id) REFERENCES users(id)
            );
            CREATE TABLE IF NOT EXISTS treehole_posts (
                id TEXT PRIMARY KEY,
                user_id INTEGER NOT NULL,
                username TEXT NOT NULL,
                nickname TEXT NOT NULL,
                mood_line TEXT NOT NULL,
                emotion TEXT NOT NULL,
                color TEXT NOT NULL,
                created_at TEXT NOT NULL,
                FOREIGN KEY(user_id) REFERENCES users(id)
            );
            CREATE TABLE IF NOT EXISTS treehole_replies (
                id TEXT PRIMARY KEY,
                post_id TEXT NOT NULL,
                user_id INTEGER NOT NULL,
                username TEXT NOT NULL,
                nickname TEXT NOT NULL,
                text TEXT NOT NULL,
                created_at TEXT NOT NULL,
                FOREIGN KEY(post_id) REFERENCES treehole_posts(id)
            );
            """
        )


class RegisterBody(BaseModel):
    username: str
    password: str
    nickname: str = ""


class LoginBody(BaseModel):
    username: str
    password: str


class PostBody(BaseModel):
    moodLine: str = Field(min_length=1, max_length=500)
    emotion: str = Field(default="心象", max_length=64)
    color: str = Field(default="#bdebd9", max_length=32)
    entryId: str = Field(default="", max_length=64)


class ReplyBody(BaseModel):
    text: str = Field(min_length=1, max_length=500)


def auth_user(authorization: Optional[str] = Header(default=None)) -> sqlite3.Row:
    if not authorization or not authorization.startswith("Bearer "):
        raise HTTPException(status_code=401, detail="请先登录。")
    token = authorization[7:].strip()
    if not token:
        raise HTTPException(status_code=401, detail="登录凭证无效。")

    with db_conn() as conn:
        row = conn.execute(
            """
            SELECT u.id, u.username, u.nickname
            FROM sessions s
            JOIN users u ON u.id = s.user_id
            WHERE s.token = ?
            """,
            (token,),
        ).fetchone()
        if row is None:
            raise HTTPException(status_code=401, detail="登录已过期，请重新登录。")
        return row


def user_payload(row: sqlite3.Row, token: str) -> dict:
    return {
        "token": token,
        "username": row["username"],
        "nickname": row["nickname"],
    }


def fetch_posts(limit: int = 100) -> list[dict]:
    with db_conn() as conn:
        posts = conn.execute(
            """
            SELECT id, username, nickname, mood_line, emotion, color, created_at
            FROM treehole_posts
            ORDER BY datetime(created_at) DESC
            LIMIT ?
            """,
            (limit,),
        ).fetchall()

        result: list[dict] = []
        for post in posts:
            replies = conn.execute(
                """
                SELECT id, username, nickname, text, created_at
                FROM treehole_replies
                WHERE post_id = ?
                ORDER BY datetime(created_at) ASC
                """,
                (post["id"],),
            ).fetchall()
            result.append(
                {
                    "id": post["id"],
                    "username": post["username"],
                    "nickname": post["nickname"],
                    "moodLine": post["mood_line"],
                    "emotion": post["emotion"],
                    "color": post["color"],
                    "createdAt": post["created_at"],
                    "replies": [
                        {
                            "id": reply["id"],
                            "username": reply["username"],
                            "nickname": reply["nickname"],
                            "text": reply["text"],
                            "createdAt": reply["created_at"],
                        }
                        for reply in replies
                    ],
                }
            )
        return result


@app.on_event("startup")
def on_startup() -> None:
    init_db()


@app.get("/api/health")
def health() -> dict:
    return {"ok": True, "service": "MoodCanvas Cloud"}


@app.post("/api/auth/register")
def register(body: RegisterBody) -> dict:
    username = body.username.strip()
    password = body.password
    nickname = body.nickname.strip() or username

    if len(username) < 3 or len(username) > 20:
        raise HTTPException(status_code=400, detail="用户名长度需在 3 到 20 个字符之间。")
    if len(password) < 6:
        raise HTTPException(status_code=400, detail="密码至少 6 位。")

    salt = secrets.token_hex(16)
    password_hash = hash_password(password, salt)
    created_at = utc_now_iso()

    with db_conn() as conn:
        exists = conn.execute(
            "SELECT id FROM users WHERE username = ? COLLATE NOCASE",
            (username,),
        ).fetchone()
        if exists is not None:
            raise HTTPException(status_code=409, detail="该用户名已被注册。")

        cursor = conn.execute(
            """
            INSERT INTO users (username, password_hash, salt, nickname, created_at)
            VALUES (?, ?, ?, ?, ?)
            """,
            (username, password_hash, salt, nickname, created_at),
        )
        user_id = cursor.lastrowid
        token = secrets.token_urlsafe(32)
        conn.execute(
            "INSERT INTO sessions (token, user_id, created_at) VALUES (?, ?, ?)",
            (token, user_id, created_at),
        )
        row = conn.execute(
            "SELECT id, username, nickname FROM users WHERE id = ?",
            (user_id,),
        ).fetchone()

    return user_payload(row, token)


@app.post("/api/auth/login")
def login(body: LoginBody) -> dict:
    username = body.username.strip()
    password = body.password

    with db_conn() as conn:
        row = conn.execute(
            "SELECT id, username, nickname, password_hash, salt FROM users WHERE username = ? COLLATE NOCASE",
            (username,),
        ).fetchone()
        if row is None:
            raise HTTPException(status_code=404, detail="用户不存在，请先注册。")

        if hash_password(password, row["salt"]) != row["password_hash"]:
            raise HTTPException(status_code=401, detail="密码不正确。")

        token = secrets.token_urlsafe(32)
        conn.execute(
            "INSERT INTO sessions (token, user_id, created_at) VALUES (?, ?, ?)",
            (token, row["id"], utc_now_iso()),
        )

    return user_payload(row, token)


@app.get("/api/treehole/posts")
def list_posts(_user: sqlite3.Row = Depends(auth_user)) -> dict:
    return {"posts": fetch_posts()}


@app.post("/api/treehole/posts")
def create_post(body: PostBody, user: sqlite3.Row = Depends(auth_user)) -> dict:
    post_id = str(uuid.uuid4())
    created_at = utc_now_iso()

    with db_conn() as conn:
        conn.execute(
            """
            INSERT INTO treehole_posts
            (id, user_id, username, nickname, mood_line, emotion, color, created_at)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                post_id,
                user["id"],
                user["username"],
                user["nickname"],
                body.moodLine.strip(),
                body.emotion.strip() or "心象",
                body.color.strip() or "#bdebd9",
                created_at,
            ),
        )

    return {
        "id": post_id,
        "username": user["username"],
        "nickname": user["nickname"],
        "moodLine": body.moodLine.strip(),
        "emotion": body.emotion.strip() or "心象",
        "color": body.color.strip() or "#bdebd9",
        "createdAt": created_at,
        "replies": [],
    }


@app.post("/api/treehole/posts/{post_id}/replies")
def create_reply(post_id: str, body: ReplyBody, user: sqlite3.Row = Depends(auth_user)) -> dict:
    reply_id = str(uuid.uuid4())
    created_at = utc_now_iso()

    with db_conn() as conn:
        post = conn.execute(
            "SELECT id FROM treehole_posts WHERE id = ?",
            (post_id,),
        ).fetchone()
        if post is None:
            raise HTTPException(status_code=404, detail="树洞帖子不存在。")

        conn.execute(
            """
            INSERT INTO treehole_replies
            (id, post_id, user_id, username, nickname, text, created_at)
            VALUES (?, ?, ?, ?, ?, ?, ?)
            """,
            (
                reply_id,
                post_id,
                user["id"],
                user["username"],
                user["nickname"],
                body.text.strip(),
                created_at,
            ),
        )

    return {
        "id": reply_id,
        "postId": post_id,
        "username": user["username"],
        "nickname": user["nickname"],
        "text": body.text.strip(),
        "createdAt": created_at,
    }
