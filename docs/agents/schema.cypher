// The prompt file itself (CLAUDE.md, AGENTS.md, …)
CREATE NODE TABLE PromptFile (
    id STRING PRIMARY KEY,                    // e.g. "repo/CLAUDE.md"
    path STRING,
    repo STRING,
    current_version INT64 DEFAULT 0,
    last_materialized TIMESTAMP,
    notes STRING
);

// Individual instructions that appear in the prompt
CREATE NODE TABLE Instruction (
    id STRING PRIMARY KEY,                    // stable UUID or content-hash
    text STRING,                              // the actual instruction text
    short_summary STRING,                     // one-line comment for Level-0 markdown
    section STRING,                           // owning markdown section (e.g. "Build Commands")
    status STRING DEFAULT 'active',           // active | deprecated | candidate_delete | experimental
    created_at TIMESTAMP DEFAULT current_timestamp(),
    last_touched TIMESTAMP DEFAULT current_timestamp(),
    age_commits INT64 DEFAULT 0,
    confidence DOUBLE DEFAULT 0.5,            // 0–1, how strongly we believe it is still needed
    disclosure_priority INT64 DEFAULT 1,      // lower = surface earlier
    embedding DOUBLE[]                        // optional vector for semantic search
);

// The “latent reasoning” made explicit
CREATE NODE TABLE Rationale (
    id STRING PRIMARY KEY,
    text STRING,                              // full natural-language “why”
    short_form STRING,                        // compressed version for comments
    created_at TIMESTAMP DEFAULT current_timestamp(),
    confidence DOUBLE DEFAULT 0.7,
    source STRING                             // 'human' | 'agent' | 'experiment'
);

// Hidden constraints / verifiers (from the paper’s model)
CREATE NODE TABLE Constraint (
    id STRING PRIMARY KEY,
    description STRING,
    verifier_type STRING,                     // e.g. "sentence_count", "style", "correctness"
    is_hard BOOLEAN DEFAULT true
);

// Concrete evidence that justified an instruction
CREATE NODE TABLE Evidence (
    id STRING PRIMARY KEY,
    kind STRING,                              // failure | success | experiment | counterfactual
    description STRING,
    outcome STRING,
    observed_at TIMESTAMP DEFAULT current_timestamp(),
    task_id STRING,
    metrics JSON                              // flexible structured results
);

// Who (or what) made the change
CREATE NODE TABLE Maintainer (
    id STRING PRIMARY KEY,                    // "human:alice" | "agent:claude-3.5" | …
    kind STRING,                              // human | agent
    name STRING,
    model_or_role STRING
);

// Versioning / commit-like events
CREATE NODE TABLE Event (
    id STRING PRIMARY KEY,
    kind STRING,                              // add | edit | delete | rewrite | materialize
    timestamp TIMESTAMP DEFAULT current_timestamp(),
    commit_hash STRING,
    message STRING
);

// Materialized markdown dumps for humans
CREATE NODE TABLE Snapshot (
    id STRING PRIMARY KEY,
    created_at TIMESTAMP DEFAULT current_timestamp(),
    markdown STRING,                          // the generated CLAUDE.md content
    instruction_count INT64,
    excess_size_estimate DOUBLE               // relative to known optimum when available
);


// Prompt structure
CREATE REL TABLE CONTAINS (FROM PromptFile TO Instruction, position INT64);
CREATE REL TABLE HAS_SNAPSHOT (FROM PromptFile TO Snapshot, version INT64);

// Provenance of an instruction
CREATE REL TABLE HAS_RATIONALE (FROM Instruction TO Rationale, is_primary BOOLEAN DEFAULT true);
CREATE REL TABLE JUSTIFIED_BY (FROM Instruction TO Evidence, strength DOUBLE);
CREATE REL TABLE COVERS (FROM Instruction TO Constraint, contribution DOUBLE);  // how much it helps satisfy the constraint

// Authorship & lifecycle
CREATE REL TABLE ADDED_BY (FROM Instruction TO Maintainer, at TIMESTAMP);
CREATE REL TABLE TOUCHED_IN (FROM Instruction TO Event);
CREATE REL TABLE SUPERSEDES (FROM Instruction TO Instruction, reason STRING);   // newer instruction replaces older

// Evidence linkage
CREATE REL TABLE OBSERVED_BY (FROM Evidence TO Maintainer);
CREATE REL TABLE TRIGGERS (FROM Evidence TO Instruction);                       // this failure caused the instruction to be added

// Higher-order
CREATE REL TABLE DEPENDS_ON (FROM Instruction TO Instruction);                  // soft dependency
CREATE REL TABLE CONFLICTS_WITH (FROM Instruction TO Instruction, severity DOUBLE);
