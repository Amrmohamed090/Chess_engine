"""
Chess Engine Match Runner
Plays matches between two UCI chess engines to compare strength.
"""

import subprocess
import time
import os
import argparse
import sys
from dataclasses import dataclass
from typing import Optional
import re
import chess
import chess.pgn
import io
from threading import Thread
from queue import Queue, Empty


@dataclass
class EngineConfig:
    path: str
    name: str


@dataclass
class GameResult:
    winner: Optional[str]  # "white", "black", or None for draw
    reason: str
    moves: list[str]
    white_engine: str
    black_engine: str


class UCIEngine:
    def __init__(self, path: str, name: str = None):
        self.path = path
        self.name = name or os.path.basename(path)
        self.process = None
        self.stdout_queue = None
        self.reader_thread = None

    def _reader_worker(self):
        """Background thread to read stdout without blocking."""
        try:
            for line in iter(self.process.stdout.readline, ''):
                if line:
                    self.stdout_queue.put(line.strip())
                else:
                    break
        except:
            pass

    def start(self):
        self.process = subprocess.Popen(
            [self.path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )
        # Start background reader thread
        self.stdout_queue = Queue()
        self.reader_thread = Thread(target=self._reader_worker, daemon=True)
        self.reader_thread.start()

        self._send("uci")
        self._wait_for("uciok")
        self._send("isready")
        self._wait_for("readyok")

    def _send(self, command: str):
        if self.process:
            self.process.stdin.write(command + "\n")
            self.process.stdin.flush()

    def _read_line(self, timeout: float = 5.0) -> Optional[str]:
        if not self.stdout_queue:
            return None
        try:
            return self.stdout_queue.get(timeout=timeout)
        except Empty:
            return None

    def _wait_for(self, expected: str, timeout: float = 10.0) -> list[str]:
        lines = []
        start = time.time()
        while time.time() - start < timeout:
            remaining = timeout - (time.time() - start)
            if remaining <= 0:
                break
            line = self._read_line(timeout=min(0.5, remaining))
            if line:
                lines.append(line)
                if expected in line:
                    return lines
        return lines

    def new_game(self):
        self._send("ucinewgame")
        self._send("isready")
        self._wait_for("readyok")

    def set_position(self, moves: list[str] = None):
        if moves:
            self._send(f"position startpos moves {' '.join(moves)}")
        else:
            self._send("position startpos")

    def get_best_move(self, movetime: int = 100) -> Optional[str]:
        self._send(f"go movetime {movetime}")
        lines = self._wait_for("bestmove", timeout=30.0)
        for line in lines:
            if line.startswith("bestmove"):
                parts = line.split()
                if len(parts) >= 2:
                    return parts[1]
        return None

    def quit(self):
        if self.process:
            self._send("quit")
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.process.kill()
            self.process = None


def is_valid_move(move: str) -> bool:
    """Basic validation for UCI move format."""
    if not move or move == "(none)":
        return False
    if len(move) < 4 or len(move) > 5:
        return False
    return True


def generate_pgn(result: GameResult) -> str:
    """Generate PGN string from a game result using python-chess."""
    # Determine result string
    if result.winner == "white":
        result_str = "1-0"
    elif result.winner == "black":
        result_str = "0-1"
    else:
        result_str = "1/2-1/2"

    # Create a game and board
    game = chess.pgn.Game()
    game.headers["White"] = result.white_engine
    game.headers["Black"] = result.black_engine
    game.headers["Result"] = result_str

    # Add moves to the game
    board = game.board()
    node = game
    for uci_move in result.moves:
        try:
            move = chess.Move.from_uci(uci_move)
            if move in board.legal_moves:
                node = node.add_variation(move)
                board.push(move)
        except:
            break

    return str(game)


def play_game(white: UCIEngine, black: UCIEngine, movetime: int = 100, max_moves: int = 200) -> GameResult:
    """Play a single game between two engines."""
    moves = []
    board = chess.Board()
    white.new_game()
    black.new_game()

    current = white
    other = black
    side = "white"

    move_count = 0

    while move_count < max_moves:
        current.set_position(moves)
        move = current.get_best_move(movetime=movetime)

        if not is_valid_move(move):
            # Current player has no valid move (checkmate or stalemate)
            winner = "black" if side == "white" else "white"
            return GameResult(
                winner=winner,
                reason="no valid move",
                moves=moves,
                white_engine=white.name,
                black_engine=black.name
            )

        # Apply move to board
        try:
            chess_move = chess.Move.from_uci(move)
            if chess_move in board.legal_moves:
                board.push(chess_move)
            else:
                # Illegal move - loss for current player
                winner = "black" if side == "white" else "white"
                print(f"    Illegal move '{move}' by {current.name} at position: {board.fen()}")
                return GameResult(
                    winner=winner,
                    reason=f"illegal move: {move}",
                    moves=moves,
                    white_engine=white.name,
                    black_engine=black.name
                )
        except Exception as e:
            winner = "black" if side == "white" else "white"
            print(f"    Invalid move format '{move}' by {current.name}: {e}")
            return GameResult(
                winner=winner,
                reason=f"invalid move format: {move}",
                moves=moves,
                white_engine=white.name,
                black_engine=black.name
            )

        moves.append(move)
        move_count += 1

        # Check for game end conditions
        if board.is_checkmate():
            # Current player (who just moved) wins
            winner = side
            return GameResult(
                winner=winner,
                reason="checkmate",
                moves=moves,
                white_engine=white.name,
                black_engine=black.name
            )

        if board.is_stalemate():
            return GameResult(
                winner=None,
                reason="stalemate",
                moves=moves,
                white_engine=white.name,
                black_engine=black.name
            )

        if board.is_insufficient_material():
            return GameResult(
                winner=None,
                reason="insufficient material",
                moves=moves,
                white_engine=white.name,
                black_engine=black.name
            )

        if board.can_claim_fifty_moves():
            return GameResult(
                winner=None,
                reason="fifty move rule",
                moves=moves,
                white_engine=white.name,
                black_engine=black.name
            )

        if board.is_repetition(3):
            return GameResult(
                winner=None,
                reason="threefold repetition",
                moves=moves,
                white_engine=white.name,
                black_engine=black.name
            )

        # Switch sides
        current, other = other, current
        side = "black" if side == "white" else "white"

    # Max moves reached - draw
    return GameResult(
        winner=None,
        reason="max moves reached",
        moves=moves,
        white_engine=white.name,
        black_engine=black.name
    )


def run_match(engine1_path: str, engine2_path: str, num_games: int = 10,
              movetime: int = 100, verbose: bool = True) -> dict:
    """Run a match between two engines, alternating colors."""

    engine1_name = os.path.splitext(os.path.basename(engine1_path))[0]
    engine2_name = os.path.splitext(os.path.basename(engine2_path))[0]

    results = {
        engine1_name: {"wins": 0, "losses": 0, "draws": 0},
        engine2_name: {"wins": 0, "losses": 0, "draws": 0}
    }

    games_played = []

    for game_num in range(num_games):
        # Alternate colors each game
        if game_num % 2 == 0:
            white_path, black_path = engine1_path, engine2_path
            white_name, black_name = engine1_name, engine2_name
        else:
            white_path, black_path = engine2_path, engine1_path
            white_name, black_name = engine2_name, engine1_name

        white = UCIEngine(white_path, white_name)
        black = UCIEngine(black_path, black_name)

        try:
            white.start()
            black.start()

            if verbose:
                print(f"Game {game_num + 1}/{num_games}: {white_name} (white) vs {black_name} (black)")

            result = play_game(white, black, movetime=movetime)
            games_played.append(result)

            if result.winner == "white":
                results[white_name]["wins"] += 1
                results[black_name]["losses"] += 1
                if verbose:
                    print(f"  Result: {white_name} wins ({result.reason}) - {len(result.moves)} moves")
            elif result.winner == "black":
                results[black_name]["wins"] += 1
                results[white_name]["losses"] += 1
                if verbose:
                    print(f"  Result: {black_name} wins ({result.reason}) - {len(result.moves)} moves")
            else:
                results[white_name]["draws"] += 1
                results[black_name]["draws"] += 1
                if verbose:
                    print(f"  Result: Draw ({result.reason}) - {len(result.moves)} moves")

            # Print PGN after each game
            if verbose:
                print(f"\n{generate_pgn(result)}\n")

        except Exception as e:
            print(f"  Error in game {game_num + 1}: {e}")
        finally:
            white.quit()
            black.quit()

    return results, games_played


def compile_engine(source_path: str, output_path: str, compiler: str = "gcc") -> bool:
    """Compile a chess engine from source."""
    try:
        cmd = [compiler, "-O3", "-o", output_path, source_path]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Compilation failed: {result.stderr}")
            return False
        print(f"Compiled: {output_path}")
        return True
    except Exception as e:
        print(f"Compilation error: {e}")
        return False


def print_results(results: dict):
    """Print match results in a formatted table."""
    print("\n" + "=" * 50)
    print("MATCH RESULTS")
    print("=" * 50)

    for engine, stats in results.items():
        total = stats["wins"] + stats["losses"] + stats["draws"]
        score = stats["wins"] + 0.5 * stats["draws"]
        percentage = (score / total * 100) if total > 0 else 0
        print(f"\n{engine}:")
        print(f"  Wins:   {stats['wins']}")
        print(f"  Losses: {stats['losses']}")
        print(f"  Draws:  {stats['draws']}")
        print(f"  Score:  {score}/{total} ({percentage:.1f}%)")

    print("\n" + "=" * 50)


def main():
    parser = argparse.ArgumentParser(description="Chess Engine Match Runner")
    parser.add_argument("--engine1", "-e1", required=True, help="Path to first engine")
    parser.add_argument("--engine2", "-e2", required=True, help="Path to second engine")
    parser.add_argument("--games", "-g", type=int, default=10, help="Number of games to play")
    parser.add_argument("--movetime", "-t", type=int, default=100, help="Time per move in ms")
    parser.add_argument("--compile", "-c", action="store_true", help="Compile source before running")
    parser.add_argument("--source", "-s", help="Source file to compile (use with --compile)")
    parser.add_argument("--output", "-o", help="Output path for compiled engine")

    args = parser.parse_args()

    # Handle compilation if requested
    if args.compile and args.source:
        output = args.output or args.source.replace(".c", "_test.exe")
        if not compile_engine(args.source, output):
            return
        args.engine1 = output

    # Verify engines exist
    if not os.path.exists(args.engine1):
        print(f"Error: Engine not found: {args.engine1}")
        return
    if not os.path.exists(args.engine2):
        print(f"Error: Engine not found: {args.engine2}")
        return

    print(f"Starting match: {args.games} games")
    print(f"Engine 1: {args.engine1}")
    print(f"Engine 2: {args.engine2}")
    print(f"Time per move: {args.movetime}ms")
    print()

    results, games = run_match(
        args.engine1,
        args.engine2,
        num_games=args.games,
        movetime=args.movetime
    )

    print_results(results)


if __name__ == "__main__":
    main()
