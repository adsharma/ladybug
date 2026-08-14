#!/usr/bin/env python3
"""Materialize the instruction graph into markdown (AGENTS.md-style).

Usage:
    .venv-ladybug/bin/python scripts/agents_materialize.py [AGENTS.lbdb] [out.md] [--with-rationale]

Reads active instructions from the graph (PromptFile -[:CONTAINS]-> Instruction,
ordered by position, grouped by section), renders markdown, then records a
new versioned Snapshot:

    * new Snapshot node  snap-<v>          with the rendered markdown
    * HAS_SNAPSHOT rel   {version: v}      from PromptFile
    * PromptFile         current_version = v, last_materialized = now
    * new Event          ev-materialize-<v> (kind 'materialize'),
                         TOUCHED_IN from every materialized instruction

Snapshots accumulate (versioned history); the loader's next run clears them.
If out.md is omitted the markdown goes to stdout.

With --with-rationale, each instruction's HAS_RATIONALE (is_primary) texts are
folded in as indented blockquotes below the bullet.
"""

import sys
from datetime import datetime, timezone
from pathlib import Path

import ladybug

PROMPT_ID = "ladybug/AGENTS.md"


def render(conn, with_rationale: bool):
    res = conn.execute(
        "MATCH (pf:PromptFile {id: $fid})-[c:CONTAINS]->(i:Instruction) "
        "WHERE i.status = 'active' "
        "RETURN c.position, i.section, i.text ORDER BY c.position",
        {"fid": PROMPT_ID},
    )
    rows = list(res)
    if not rows:
        sys.stderr.write("no active instructions found\n")
        sys.exit(1)

    pf = conn.execute(
        "MATCH (pf:PromptFile {id: $fid}) RETURN pf.repo", {"fid": PROMPT_ID}
    ).get_next()
    repo = pf[0] or "agent"

    rationales = {}
    if with_rationale:
        rr = conn.execute(
            "MATCH (i:Instruction)-[hr:HAS_RATIONALE]->(r:Rationale) "
            "RETURN i.id, r.short_form ORDER BY r.created_at"
        )
        for iid, short in rr:
            rationales.setdefault(iid, []).append(short)

    # group by section, preserving first-seen order
    sections = {}
    order = []
    for _pos, section, text in rows:
        key = section or "General"
        if key not in sections:
            sections[key] = []
            order.append(key)
        sections[key].append(text)

    out = [f"# {repo.title()} Agent Guidelines", ""]
    for key in order:
        out.append(f"## {key}")
        out.append("")
        for text in sections[key]:
            out.append(f"- {text}")
            if with_rationale:
                for why in rationales.get("", []):
                    out.append(f"  > why: {why}")
        out.append("")

    return "\n".join(out).rstrip() + "\n", len(rows)


def record_snapshot(conn, markdown: str, n: int, now):
    # next version = max existing HAS_SNAPSHOT.version + 1
    res = conn.execute(
        "MATCH (pf:PromptFile {id: $fid})-[c:HAS_SNAPSHOT]->(:Snapshot) "
        "RETURN c.version ORDER BY c.version DESC LIMIT 1",
        {"fid": PROMPT_ID},
    )
    v = res.get_next()[0] + 1 if res.has_next() else 1

    conn.execute(
        "CREATE (:Snapshot {id: $id, created_at: $ts, markdown: $md, "
        "instruction_count: $cnt, excess_size_estimate: 0.0})",
        {"id": f"snap-{v}", "ts": now, "md": markdown, "cnt": n},
    )
    conn.execute(
        "MATCH (pf:PromptFile {id: $fid}), (sn:Snapshot {id: $sid}) "
        "CREATE (pf)-[:HAS_SNAPSHOT {version: $v}]->(sn)",
        {"fid": PROMPT_ID, "sid": f"snap-{v}", "v": v},
    )
    conn.execute(
        "MATCH (pf:PromptFile {id: $fid}) "
        "SET pf.current_version = $v, pf.last_materialized = $ts",
        {"fid": PROMPT_ID, "v": v, "ts": now},
    )
    conn.execute(
        "CREATE (:Event {id: $id, kind: 'materialize', timestamp: $ts, "
        "commit_hash: 'none', message: $msg})",
        {"id": f"ev-materialize-{v}", "ts": now, "msg": f"Materialized markdown v{v}"},
    )
    conn.execute(
        "MATCH (pf:PromptFile {id: $fid})-[c:CONTAINS]->(i:Instruction) "
        "WHERE i.status = 'active' "
        "MATCH (ev:Event {id: $eid}) "
        "CREATE (i)-[:TOUCHED_IN]->(ev)",
        {"fid": PROMPT_ID, "eid": f"ev-materialize-{v}"},
    )
    return v


def main():
    db_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("AGENTS.lbdb")
    out_path = None
    with_rationale = "--with-rationale" in sys.argv
    for arg in sys.argv[2:]:
        if not arg.startswith("--"):
            out_path = Path(arg)
            break

    conn = ladybug.Connection(ladybug.Database(str(db_path)))
    markdown, n = render(conn, with_rationale)
    now = datetime.now(timezone.utc)
    v = record_snapshot(conn, markdown, n, now)

    if out_path:
        out_path.write_text(markdown)
        print(f"materialized v{v}: {n} instructions -> {out_path}")
    else:
        sys.stdout.write(markdown)
        print(f"\n// materialized v{v}: {n} instructions", file=sys.stderr)


if __name__ == "__main__":
    main()