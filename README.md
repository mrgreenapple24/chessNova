# chessNova ♟️

A high-performance chess engine focused on bitboard-based board representation.

## Current Progress

- **Bitboard Core:** 64-bit integer representation of the chess board
- **Attack Logic:** Fully implemented attacks for all pieces (leapers & sliding pieces)
- **Board State:** Piece placement, square control, and Zobrist hashing
- **Move Generation:** Full legal move generation (Perft verified)
- **Magic Bitboards:** High-performance sliding piece attacks
- **Search Engine:** Alpha-Beta with iterative deepening, quiescence search, and move ordering
- **Evaluation:** Tapered evaluation with piece-square tables
- **UCI Protocol:** Full UCI support for GUI integration

## What's Next

See [PLAN.md](./PLAN.md) for the detailed roadmap.

## Building

### Prerequisites
- CMake (3.10 or higher)
- C compiler (GCC or Clang)

### Build & Run

```bash
# Clone the repository
git clone https://github.com/mrgreenapple24/chessNova.git
cd chessNova

# Make the build script executable and run it
chmod +x build.sh
./build.sh          # Builds the engine
./build.sh run      # Builds and runs
./build.sh test     # Runs tests
./build.sh clean    # Cleans build files

# Generate documentation
doxygen Doxyfile
# Then open docs/html/index.html
