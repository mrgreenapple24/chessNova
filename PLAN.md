# chessNova — Detailed Development Plan

---

## Current State

- Move generation complete and verified (perft tests passing)
- Basic alpha-beta search implemented
- Basic static evaluation function implemented
- No opening book, no endgame tablebases, no transposition table

---

## 1. Transposition Tables (TT)

### Overview
A transposition table is a hash map keyed on Zobrist hashes of positions. It allows the engine
to avoid re-searching positions it has already evaluated, dramatically increasing effective search depth.

### 1.1 Zobrist Hashing
- [ ] Assign a random 64-bit number to every (piece, square) combination (12 × 64 = 768 values)
- [ ] Add random values for: side to move, castling rights (4 bits), en passant file (8 values)
- [ ] Incrementally update the hash on make/unmake move — never recompute from scratch
- [ ] Verify correctness: after make+unmake, hash must equal original

### 1.2 TT Entry Structure
```c
typedef struct {
    uint64_t key;       // Full Zobrist key for verification
    int16_t  score;     // Evaluation score
    uint8_t  depth;     // Depth at which this was searched
    uint8_t  flag;      // EXACT, LOWER_BOUND (beta cutoff), UPPER_BOUND (alpha)
    Move     best_move; // Best move found from this position
} TTEntry;
```

### 1.3 Table Design
- [ ] Use power-of-two table size for fast index via bitmasking (`key & (size - 1)`)
- [ ] Start with 64MB default, make size configurable via UCI option
- [ ] Use a **two-bucket** scheme per slot (always-replace + depth-preferred) to reduce collision loss
- [ ] On collision: replace if stored depth ≤ incoming depth, or if generation differs (aging)

### 1.4 Integration into Search
- [ ] Probe TT at the top of every `negamax`/`alpha_beta` call before doing any work
- [ ] On hit: if `entry.depth >= current_depth`, use score directly (adjust for mate distances)
- [ ] Store result in TT on the way back up the tree with appropriate flag
- [ ] Use `best_move` from TT hit as first move to try (feeds move ordering)

### 1.5 TT and Mate Scores
- [ ] Mate scores must be stored relative to the current node, not the root
- [ ] On store: `score += ply` if score is a mate score; on retrieve: `score -= ply`

### 1.6 Testing
- [ ] Verify no illegal moves are returned from TT after hash collision
- [ ] Check that node counts decrease significantly vs. baseline on standard positions
- [ ] Run perft through TT to confirm no incorrect cutoffs

---

## 2. Search Improvements

### 2.1 Move Ordering
Good move ordering is the single biggest driver of search efficiency. Target: approach the
theoretical minimum of `O(b^(d/2))` nodes.

- [ ] **TT move first** — always try the hash move before anything else
- [ ] **MVV-LVA** (Most Valuable Victim – Least Valuable Attacker) for captures
- [ ] **Killer moves** — store 2 non-capture moves per ply that caused a beta cutoff; try them early
- [ ] **History heuristic** — track which quiet moves improved alpha; score them proportionally
- [ ] **Countermove heuristic** — store one refutation per (piece, to-square) pair

### 2.2 Iterative Deepening (ID)
- [ ] Always search via ID: depth 1, 2, 3 … up to max depth or time limit
- [ ] Use result of previous iteration to seed move ordering for the next
- [ ] This makes time management straightforward and makes TT hits more likely

### 2.3 Aspiration Windows
- [ ] After depth ≥ 4, search with a narrow window `[prev_score - δ, prev_score + δ]`
- [ ] On fail-low or fail-high, widen the window and re-search
- [ ] Start with δ = 25–50 centipawns; widen to full window on second failure

### 2.4 Null Move Pruning
- [ ] If not in check and depth ≥ 3, try passing the turn (null move)
- [ ] Search resulting position at `depth - R - 1` (R = 2 or 3, adaptive based on depth)
- [ ] If score ≥ beta, prune the node (return beta)
- [ ] Disable in zugzwang-prone positions (e.g., only pawns + king remaining)

### 2.5 Late Move Reductions (LMR)
- [ ] After move ordering, later moves are likely worse; reduce their search depth
- [ ] Conditions: depth ≥ 3, not in check, not a capture, not a killer, move index ≥ 3–4
- [ ] Reduce by `max(1, depth/3 + log(depth)*log(move_index)/2)` (Stockfish-style formula)
- [ ] If a reduced search beats alpha, re-search at full depth

### 2.6 Futility Pruning
- [ ] Near the leaves (depth ≤ 2), if static eval + margin < alpha, skip quiet moves
- [ ] Margins per depth: depth 1 ≈ 100cp, depth 2 ≈ 300cp, depth 3 ≈ 500cp
- [ ] Never prune when in check, or when static eval is unreliable (mate threats)

### 2.7 Quiescence Search
- [ ] Ensure QSearch only considers captures (and checks, optionally)
- [ ] Apply **delta pruning**: if best possible capture can't raise alpha, prune
- [ ] Apply **SEE** (Static Exchange Evaluation) to skip clearly losing captures
- [ ] Cap QSearch depth to prevent infinite loops in rare positions

### 2.8 Check Extensions
- [ ] When the side to move is in check, extend the search by 1 ply
- [ ] Optionally extend on passed pawn pushes to 7th rank and singular moves

---

## 3. Evaluation Improvements

### 3.1 Pawn Structure
Pawn structure evaluation is permanent (pawns don't move backward) and benefits from caching.

- [ ] **Pawn hash table**: separate small hash table (1–4MB) keyed on pawn Zobrist hash
- [ ] **Doubled pawns**: penalty for 2+ pawns on the same file
- [ ] **Isolated pawns**: penalty for pawns with no friendly pawns on adjacent files
- [ ] **Backward pawns**: penalty for pawns that can't be safely advanced and have no support
- [ ] **Passed pawns**: significant bonus; scale bonus by rank (7th rank = very large bonus)
- [ ] **Pawn chains**: small bonus for pawns defending each other diagonally
- [ ] **Pawn islands**: penalty proportional to number of disconnected pawn groups
- [ ] **Candidate passed pawns**: bonus for pawns that could become passers with help

### 3.2 King Safety
- [ ] **Pawn shield**: bonus for pawns on f2/g2/h2 (or mirrored) in front of a castled king
- [ ] **Pawn storm**: penalty if opponent has pawns advancing toward your king's shelter
- [ ] **King attack zone**: count opponent pieces attacking squares near the king; scale penalty
  exponentially (one attacker = small penalty, four attackers = large penalty)
- [ ] **Open files near king**: penalty for open/semi-open files next to the king
- [ ] **No castling rights remaining**: penalty if king has not castled and rights are lost
- [ ] **Tropism**: bonus for opponent pieces close to king (complementary to attack zone)

### 3.3 Piece Mobility & Activity
- [ ] Count legal (or pseudo-legal) moves for each piece; reward mobility
- [ ] **Rook on open file**: bonus; **rook on semi-open file**: smaller bonus
- [ ] **Rook on 7th rank**: large bonus when opponent king is on 8th rank
- [ ] **Connected rooks**: bonus for rooks on the same rank/file with no pieces between
- [ ] **Knight outpost**: bonus for knights on central squares protected by a pawn with no
  opponent pawn that can challenge them
- [ ] **Bad bishop**: penalty for bishop blocked by its own pawns of the same color
- [ ] **Bishop pair**: bonus (~50cp) for having both bishops when opponent does not

### 3.4 Piece-Square Tables (PST)
- [ ] Ensure separate PSTs for **middlegame** and **endgame** for every piece type
- [ ] Interpolate using a **game phase** value derived from remaining material (tapered eval)
- [ ] Formula: `score = (mg_score * phase + eg_score * (24 - phase)) / 24`
  where phase counts queens (×4), rooks (×2), bishops (×1), knights (×1); max = 24

### 3.5 Texel Tuning
- [ ] Collect a large dataset of quiet positions with known game outcomes (W/D/L) from PGN
- [ ] Implement `sigmoid(score) = 1 / (1 + 10^(-K * score / 400))` as the expected result
- [ ] Define error: `E = Σ (result - sigmoid(score))²` over all positions
- [ ] Use gradient descent or local optimization (coordinate descent) to minimize E
- [ ] Make all evaluation weights tunable: PST values, piece values, structure bonuses/penalties
- [ ] Run tuning in a separate process/thread; do not interrupt normal operation
- [ ] Validate: tuned weights should show Elo improvement in self-play testing

---

## 4. Opening Book

### 4.1 Polyglot Format
- [ ] Study the Polyglot `.bin` format: 16-byte entries (key, move, weight, learn)
- [ ] Implement a reader that maps Polyglot move encoding to internal move representation
- [ ] Handle edge cases: castling encoding differs from internal format

### 4.2 Book Probing Logic
- [ ] At root, probe book using current Zobrist key
- [ ] If multiple book moves exist, select by weight (weighted random or always-best)
- [ ] Disable book after N consecutive book moves or if `OwnBook false` UCI option is set
- [ ] Allow users to specify a custom book path via UCI option

### 4.3 Book Sources
- [ ] Bundle a small default book (e.g., `gm2001.bin` or similar freely available book)
- [ ] Document how to replace with a larger book (e.g., `Cerebellum`, `komodo.bin`)

---

## 5. Endgame Tablebases

### 5.1 Syzygy Integration
Syzygy tables provide **WDL** (win/draw/loss) and **DTZ** (distance to zeroing move) information
for all positions with up to 7 pieces.

- [ ] Clone and build `Fathom` (the standard C Syzygy probing library)
- [ ] Integrate `Fathom` into the build system (CMake/Makefile)
- [ ] Add UCI option `SyzygyPath` to specify tablebase directory
- [ ] Probe WDL at root and in search when piece count ≤ threshold
- [ ] Use DTZ information at root to select the fastest winning move
- [ ] Adjust search: if WDL probe returns draw, do not search further (return draw score)
- [ ] Handle missing files gracefully (not all 7-piece tables may be present)

### 5.2 Gaviota (Optional)
- [ ] Gaviota provides **DTM** (distance to mate) which is more precise but larger files
- [ ] Add as an optional alternative; use if `SyzygyPath` is unset but `GaviotaPath` is set

### 5.3 50-Move Rule Interaction
- [ ] DTZ values must be checked against the 50-move counter
- [ ] A "win" that requires more moves than 50-move rule allows is actually a draw — handle this

---

## 6. Time Management

### 6.1 Time Control Modes
- [ ] Detect from UCI: `movetime`, `wtime`/`btime`/`winc`/`binc`, `movestogo`, `infinite`
- [ ] For `movetime`: search exactly that long
- [ ] For `infinite`: search until `stop` command received

### 6.2 Allocation Formula
- [ ] Estimate remaining moves: `moves_left = movestogo ?? max(30, 50 - move_number)`
- [ ] Base time: `base = time_remaining / moves_left`
- [ ] Increment bonus: add `increment * 0.9` to base
- [ ] Hard limit: never use more than `time_remaining / 2` on a single move (avoid flagging)
- [ ] Soft limit (target): `base * 0.6` — stop searching at next ID iteration boundary if exceeded

### 6.3 Dynamic Adjustment
- [ ] If the best move changes at the last iteration, extend time by up to 1.5×
- [ ] If score drops sharply between iterations (fail low), extend time
- [ ] Reduce time usage when only one legal move is available (play instantly)
- [ ] Reduce time in clearly won positions (large material advantage)

### 6.4 Node-Based Checks
- [ ] Check the clock every 2048 nodes (not every node — system calls are expensive)
- [ ] Use `clock()` or `CLOCK_MONOTONIC` consistently; avoid mixing time sources

---

## 7. UCI Protocol Completeness

- [ ] `uci` → respond with `id name`, `id author`, all `option` declarations, `uciok`
- [ ] `isready` → respond `readyok` (ensure TT is allocated, book is loaded first)
- [ ] `setoption name <x> value <y>` → handle: `Hash`, `SyzygyPath`, `OwnBook`, `MoveOverhead`
- [ ] `position startpos moves ...` and `position fen ... moves ...`
- [ ] `go` with all time control parameters
- [ ] `stop` → stop searching immediately, output best move found so far
- [ ] `quit` → clean up and exit
- [ ] `ponderhit` → if implementing pondering later
- [ ] Output `info depth score nodes nps hashfull pv` lines during search

---

## 8. Testing & Validation

### 8.1 Correctness
- [ ] Maintain perft suite; add positions with en passant, castling, promotions edge cases
- [ ] Add a regression suite: positions where the engine must find a specific move

### 8.2 Strength Testing
- [ ] Set up `cutechess-cli` or `fastchess` for automated match play
- [ ] Test each feature branch vs. previous version (SPRT with H0: elo=0, H1: elo=10)
- [ ] Track Elo history per version in a `CHANGELOG.md`

### 8.3 Debugging Tools
- [ ] Implement `bench` command: run a fixed set of positions to a fixed depth, output node count
  (used for detecting regressions and comparing builds)
- [ ] Implement `eval` command: print full evaluation breakdown for the current position
- [ ] Implement `perft divide` for debugging move generation

---

## 9. Code Quality & Maintenance

- [ ] Add `README.md` sections: build instructions, UCI options, feature list, benchmark results
- [ ] Add `ARCHITECTURE.md` describing board representation, search, eval structure
- [ ] Keep `EVALUATION.md` up to date as tuning weights change
- [ ] Use `clang-format` with a project `.clang-format` file for consistent style
- [ ] Enable `-Wall -Wextra -Wshadow -fsanitize=address,undefined` in debug builds
- [ ] Set up a `Makefile` with targets: `release`, `debug`, `profile`, `bench`

---

## Priority Order

| Priority | Task                              | Expected Elo Gain |
|----------|-----------------------------------|-------------------|
| 1        | Transposition Table               | +100–200          |
| 2        | Move Ordering (TT + killers + LMR)| +100–150          |
| 3        | Null Move Pruning                 | +50–100           |
| 4        | Evaluation: tapered eval + PSTs   | +50–100           |
| 5        | King Safety                       | +30–70            |
| 6        | Pawn Structure + pawn hash        | +20–50            |
| 7        | Time Management                   | +20–40            |
| 8        | Opening Book                      | +10–20            |
| 9        | Texel Tuning                      | +30–80            |
| 10       | Syzygy Tablebases                 | +10–30 (endgames) |
