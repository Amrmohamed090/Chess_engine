"""
Data generation for NNUE training.

Strategy:
  1. bbc.exe plays short self-play games.
  2. At each position we send "eval" to get the static score.
  3. Positions are filtered (skip openings, endgame extremes).
  4. Output: data/positions.csv  →  fen, score (centipawns, white POV)

Requires a patched bbc.exe that handles the custom "eval" UCI command.
Patch bbc.c uci_loop() by adding before the final closing brace:

    else if (strncmp(input, "eval", 4) == 0)
    {
        printf("eval %d\n", evaluate());
        fflush(stdout);
    }

Then recompile:  gcc -O3 -o bbc.exe bbc.c
"""

import subprocess
import random
import csv
import os
import time
from pathlib import Path
from dataclasses import dataclass

ENGINE_PATH = "./bbc.exe"
OUTPUT_DIR  = Path("data")
OUTPUT_FILE = OUTPUT_DIR / "positions.csv"

# --- tunables ---
NUM_GAMES        = 500          # self-play games to generate
MOVETIME_MS      = 50           # ms per move (fast, for data gen)
MIN_MOVE         = 8            # skip the first N moves (opening)
MAX_SCORE        = 2000         # discard positions with |eval| > this (cp)
MAX_MOVES_GAME   = 150          # adjudicate at this ply
SKIP_PROB        = 0.5          # randomly skip 50 % of positions (diversity)


# ── UCI engine wrapper ────────────────────────────────────────────────────────

class Engine:
    def __init__(self, path: str):
        self.proc = subprocess.Popen(
            [path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        self._init()

    def _send(self, cmd: str):
        self.proc.stdin.write(cmd + "\n")
        self.proc.stdin.flush()

    def _read_until(self, token: str, timeout: float = 10.0) -> list[str]:
        lines = []
        deadline = time.time() + timeout
        while time.time() < deadline:
            line = self.proc.stdout.readline().strip()
            if line:
                lines.append(line)
            if token in line:
                return lines
        raise TimeoutError(f"Waited for '{token}' but got: {lines}")

    def _init(self):
        self._send("uci")
        self._read_until("uciok")
        self._send("isready")
        self._read_until("readyok")

    def new_game(self):
        self._send("ucinewgame")
        self._send("position startpos")
        self._send("isready")
        self._read_until("readyok")

    def set_position(self, moves: list[str]):
        if moves:
            self._send(f"position startpos moves {' '.join(moves)}")
        else:
            self._send("position startpos")

    def best_move(self, movetime: int = MOVETIME_MS) -> str | None:
        self._send(f"go movetime {movetime}")
        lines = self._read_until("bestmove", timeout=30)
        for line in lines:
            if line.startswith("bestmove"):
                parts = line.split()
                move = parts[1] if len(parts) > 1 else None
                return None if move in (None, "(none)") else move
        return None

    def static_eval(self) -> int | None:
        """Return static evaluation in centipawns (white POV).
        Requires the custom 'eval' command in bbc.exe."""
        self._send("eval")
        try:
            lines = self._read_until("eval ", timeout=2)
            for line in lines:
                if line.startswith("eval "):
                    return int(line.split()[1])
        except (TimeoutError, ValueError):
            pass
        return None

    def board_fen(self) -> str | None:
        """Ask engine for current FEN via d command if supported."""
        return None  # bbc.exe doesn't expose this; we track FEN in Python

    def quit(self):
        try:
            self._send("quit")
            self.proc.wait(timeout=2)
        except Exception:
            self.proc.kill()


# ── FEN tracking with python-chess ───────────────────────────────────────────

try:
    import chess
    HAS_CHESS = True
except ImportError:
    HAS_CHESS = False
    print("WARNING: python-chess not installed. FEN tracking disabled.")
    print("Install with: pip install chess")


def play_game(engine: Engine) -> list[tuple[str, int]]:
    """
    Play one self-play game. Returns list of (fen, eval_cp) pairs.
    Eval is always from white's perspective.
    """
    if not HAS_CHESS:
        return []

    board = chess.Board()
    moves: list[str] = []
    samples: list[tuple[str, int]] = []

    engine.new_game()

    for ply in range(MAX_MOVES_GAME):
        if board.is_game_over():
            break

        # get engine move
        engine.set_position(moves)
        uci_move = engine.best_move()
        if uci_move is None:
            break

        # apply move
        try:
            move = chess.Move.from_uci(uci_move)
            board.push(move)
            moves.append(uci_move)
        except Exception:
            break

        # skip opening and randomly sample
        if ply < MIN_MOVE:
            continue
        if random.random() < SKIP_PROB:
            continue

        # get static eval for the position AFTER the move
        engine.set_position(moves)
        score = engine.static_eval()
        if score is None:
            continue

        # static eval is from side-to-move POV in many engines;
        # flip to white POV if it's black's turn
        if board.turn == chess.BLACK:
            score = -score

        # filter extreme scores
        if abs(score) > MAX_SCORE:
            continue

        samples.append((board.fen(), score))

    return samples


# ── main ─────────────────────────────────────────────────────────────────────

def main():
    if not os.path.exists(ENGINE_PATH):
        raise FileNotFoundError(
            f"Engine not found: {ENGINE_PATH}\n"
            "Compile with: gcc -O3 -o bbc.exe bbc.c\n"
            "Also add the 'eval' UCI command to bbc.c (see module docstring)."
        )

    OUTPUT_DIR.mkdir(exist_ok=True)
    engine = Engine(ENGINE_PATH)

    total = 0
    with open(OUTPUT_FILE, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["fen", "score"])

        for game_num in range(1, NUM_GAMES + 1):
            samples = play_game(engine)
            for fen, score in samples:
                writer.writerow([fen, score])
                total += 1

            if game_num % 10 == 0:
                print(f"Game {game_num}/{NUM_GAMES} — {total} positions so far")

    engine.quit()
    print(f"\nDone. {total} positions saved to {OUTPUT_FILE}")


if __name__ == "__main__":
    main()
