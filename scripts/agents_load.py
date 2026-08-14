#!/usr/bin/env python3
"""Load AGENTS.md instructions into AGENTS.lbdb via the ladybug Python API.

Usage:
    .venv-ladybug/bin/python scripts/agents_load.py [AGENTS.md] [AGENTS.lbdb]

The tables are defined in docs/agents/schema.cypher. This loader is
idempotent: it first clears the loader-owned tables (PromptFile, Instruction,
Snapshot, Maintainer, Event) and their relationships, then reloads.

Everything is written with parameterized queries so no string escaping is
needed, regardless of quotes/newlines/`$` in the markdown.
"""

import re
import sys
from datetime import datetime, timezone
from pathlib import Path

import ladybug

REPO = "ladybug"
PROMPT_ID = f"{REPO}/AGENTS.md"
PROMPT_PATH = "AGENTS.md"

# Node tables this loader owns (cleared on every run). Other tables
# (Rationale, Constraint, Evidence, ...) are left untouched.
OWNED_TABLES = ["PromptFile", "Instruction", "Snapshot", "Maintainer", "Event"]


def slugify(text: str, fallback: str) -> str:
    slug = re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")
    return slug or fallback


def collect_instructions(md_path: Path):
    """Return list of dicts: {section, subsection, text, short_summary}."""
    lines = md_path.read_text().splitlines()
    instructions = []
    section = subsection = ""
    in_code = False
    active_comment = None  # '#' comment line inside a code block
    group = []             # command lines of the current comment group

    def flush_group():
        nonlocal group, active_comment
        if not group:
            return
        if active_comment and len(group) <= 2:
            instructions.append(
                dict(
                    section=section,
                    subsection=subsection,
                    text=f"{active_comment}: {'; '.join(group)}",
                    short_summary=f"{section} / {active_comment[:48]}",
                )
            )
        elif active_comment:
            # many one-line commands under one comment -> one instruction each
            for c in group:
                instructions.append(
                    dict(
                        section=section,
                        subsection=subsection,
                        text=f"{active_comment}: {c}",
                        short_summary=f"{section} / {c}",
                    )
                )
        else:
            for c in group:
                instructions.append(
                    dict(
                        section=section,
                        subsection=subsection,
                        text=c,
                        short_summary=f"{section} / {c}",
                    )
                )
        group = []
        active_comment = None

    for raw in lines:
        line = raw.rstrip()

        # fenced code blocks
        if line.lstrip().startswith("```"):
            flush_group()
            in_code = not in_code
            continue

        if in_code:
            stripped = line.strip()
            if stripped.startswith("#"):
                flush_group()
                active_comment = stripped[1:].strip()
            elif stripped:
                group.append(stripped)
            continue

        if not line.strip():
            continue

        # skip the H1 document title
        if re.match(r"^#\s+", line):
            continue

        h = re.match(r"^(#{2,4})\s+(.*)$", line)
        if h:
            level = len(h.group(1))
            title = h.group(2).strip()
            if level == 2:
                section = title
                subsection = ""
            elif level >= 3:
                subsection = title
            continue

        # paragraph or bullet outside code blocks
        text = line.strip().lstrip("-*").strip()
        if not text:
            continue
        instructions.append(
            dict(
                section=section,
                subsection=subsection,
                text=text,
                short_summary=f"{section} / {text[:40]}",
            )
        )

    flush_group()
    return instructions


def clear_owned_tables(conn):
    for t in OWNED_TABLES:
        conn.execute(f"MATCH (n:{t}) DETACH DELETE n;")


def load(md_path: Path, db_path: Path):
    instructions = collect_instructions(md_path)
    md_text = md_path.read_text()
    n = len(instructions)

    db = ladybug.Database(str(db_path))
    conn = ladybug.Connection(db)
    clear_owned_tables(conn)

    now = datetime.now(timezone.utc)

    # unique ids (readable slugs, deduplicated)
    used = {}
    ids = []
    for idx, ins in enumerate(instructions):
        base = slugify(ins["text"].split(":", 1)[0][:32], f"instr-{idx}")
        if base in used:
            used[base] += 1
            base = f"{base}-{used[base]}"
        else:
            used[base] = 0
        ids.append(base)

    conn.execute(
        "CREATE (:PromptFile {id: $id, path: $path, repo: $repo, "
        "current_version: $ver, last_materialized: $ts, notes: $notes})",
        {
            "id": PROMPT_ID,
            "path": PROMPT_PATH,
            "repo": REPO,
            "ver": 1,
            "ts": now,
            "notes": "Imported from AGENTS.md (markdown); see Snapshot",
        },
    )
    conn.execute(
        "CREATE (:Snapshot {id: $id, created_at: $ts, markdown: $md, "
        "instruction_count: $cnt, excess_size_estimate: 0.0})",
        {"id": "snap-1", "ts": now, "md": md_text, "cnt": n},
    )
    conn.execute(
        "CREATE (:Maintainer {id: $id, kind: $kind, name: $name, model_or_role: $role})",
        {"id": "agent:pi", "kind": "agent", "name": "pi coding agent", "role": "coding-agent"},
    )
    conn.execute(
        "CREATE (:Event {id: $id, kind: $kind, timestamp: $ts, "
        "commit_hash: $hash, message: $msg})",
        {
            "id": "ev-1",
            "kind": "materialize",
            "ts": now,
            "hash": "none",
            "msg": f"Initial load of {PROMPT_ID}",
        },
    )
    conn.execute(
        "MATCH (pf:PromptFile {id: $fid}), (sn:Snapshot {id: $sid}) "
        "CREATE (pf)-[:HAS_SNAPSHOT {version: 1}]->(sn)",
        {"fid": PROMPT_ID, "sid": "snap-1"},
    )

    for pos, (ins, iid) in enumerate(zip(instructions, ids)):
        conn.execute(
            "MATCH (pf:PromptFile {id: $fid}), (m:Maintainer {id: $mid}), "
            "(ev:Event {id: $eid}) "
            "CREATE (pf)-[:CONTAINS {position: $pos}]->"
            "(i:Instruction {id: $iid, text: $text, short_summary: $summary, "
            "section: $section, "
            "status: 'active', created_at: $ts, last_touched: $ts, "
            "age_commits: 0, confidence: 0.9, disclosure_priority: $prio}), "
            "(i)-[:ADDED_BY {at: $ts}]->(m), "
            "(i)-[:TOUCHED_IN]->(ev)",
            {
                "fid": PROMPT_ID,
                "mid": "agent:pi",
                "eid": "ev-1",
                "pos": pos,
                "iid": iid,
                "text": ins["text"],
                "summary": ins["short_summary"],
                "section": ins["section"] or None,
                "ts": now,
                "prio": pos + 1,
            },
        )

    print(f"Loaded {n} instructions from {md_path} into {db_path}")
    return conn


def main():
    md_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("AGENTS.md")
    db_path = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("AGENTS.lbdb")
    load(md_path, db_path)


if __name__ == "__main__":
    main()