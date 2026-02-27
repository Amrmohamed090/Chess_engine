"""
Chess Engine Match GUI using chessboard.js
Watch two UCI chess engines play against each other, or play against the engine yourself.
"""

from flask import Flask, render_template, jsonify, request, send_from_directory
import chess
import threading
import os
import webbrowser
import urllib.request
from chess_match import UCIEngine, is_valid_move

# Setup paths
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PIECES_DIR = os.path.join(SCRIPT_DIR, "pieces")

app = Flask(__name__)

MAX_LOG_LINES = 500


def download_pieces():
    """Download chess piece images if they don't exist."""
    if not os.path.exists(PIECES_DIR):
        os.makedirs(PIECES_DIR)

    pieces = ['wK', 'wQ', 'wR', 'wB', 'wN', 'wP', 'bK', 'bQ', 'bR', 'bB', 'bN', 'bP']
    base_url = "https://raw.githubusercontent.com/oakmac/chessboardjs/master/website/img/chesspieces/wikipedia/"

    for piece in pieces:
        filepath = os.path.join(PIECES_DIR, f"{piece}.png")
        if not os.path.exists(filepath):
            url = f"{base_url}{piece}.png"
            print(f"Downloading {piece}.png...")
            try:
                urllib.request.urlretrieve(url, filepath)
            except Exception as e:
                print(f"Failed to download {piece}.png: {e}")


# Used to signal the game thread when the human submits a move
_human_move_event = threading.Event()

# Global game state
game_state = {
    "board": chess.Board(),
    "moves": [],
    "status": "waiting",
    "message": "Configure engines and click Start",
    "running": False,
    "white_name": "",
    "black_name": "",
    "last_move": None,
    "mode": "engine",         # "engine" or "human"
    "human_side": None,       # "white" or "black"
    "waiting_for_human": False,
    "pending_human_move": None,
    "white_logs": [],
    "black_logs": [],
}


def make_log_callback(side):
    """Return a log callback that appends to the appropriate engine log."""
    def callback(direction, message):
        prefix = ">>" if direction == "send" else "<<"
        entry = f"{prefix} {message}"
        log_list = game_state[f"{side}_logs"]
        log_list.append(entry)
        if len(log_list) > MAX_LOG_LINES:
            del log_list[:-MAX_LOG_LINES]
    return callback


@app.route('/')
def index():
    return render_template('index.html')


@app.route('/pieces/<piece>')
def serve_piece(piece):
    return send_from_directory(PIECES_DIR, piece)


@app.route('/state')
def get_state():
    san_moves = []
    temp_board = chess.Board()
    for uci_move in game_state["moves"]:
        try:
            move = chess.Move.from_uci(uci_move)
            san_moves.append(temp_board.san(move))
            temp_board.push(move)
        except:
            pass

    return jsonify({
        "fen": game_state["board"].fen(),
        "moves": game_state["moves"],
        "san_moves": san_moves,
        "status": game_state["status"],
        "message": game_state["message"],
        "running": game_state["running"],
        "white_name": game_state["white_name"],
        "black_name": game_state["black_name"],
        "mode": game_state["mode"],
        "human_side": game_state["human_side"],
        "waiting_for_human": game_state["waiting_for_human"],
    })


@app.route('/logs')
def get_logs():
    return jsonify({
        "white": game_state["white_logs"],
        "black": game_state["black_logs"],
    })


@app.route('/start', methods=['POST'])
def start_game():
    if game_state["running"]:
        return jsonify({"error": "Game already running"})

    data = request.json
    engine1_path = data.get("engine1")
    engine2_path = data.get("engine2")
    movetime = data.get("movetime", 500)

    if not os.path.exists(engine1_path):
        return jsonify({"error": f"Engine 1 not found: {engine1_path}"})
    if not os.path.exists(engine2_path):
        return jsonify({"error": f"Engine 2 not found: {engine2_path}"})

    game_state["board"] = chess.Board()
    game_state["moves"] = []
    game_state["running"] = True
    game_state["status"] = "playing"
    game_state["mode"] = "engine"
    game_state["human_side"] = None
    game_state["waiting_for_human"] = False
    game_state["pending_human_move"] = None
    game_state["white_name"] = os.path.basename(engine1_path)
    game_state["black_name"] = os.path.basename(engine2_path)
    game_state["white_logs"] = []
    game_state["black_logs"] = []

    thread = threading.Thread(
        target=run_game,
        args=(engine1_path, engine2_path, movetime),
        daemon=True
    )
    thread.start()

    return jsonify({"success": True})


@app.route('/start_human', methods=['POST'])
def start_human():
    if game_state["running"]:
        return jsonify({"error": "Game already running"})

    data = request.json
    engine_path = data.get("engine")
    human_side = data.get("human_side", "white")
    movetime = data.get("movetime", 500)

    if not engine_path or not os.path.exists(engine_path):
        return jsonify({"error": f"Engine not found: {engine_path}"})

    engine_name = os.path.basename(engine_path)

    game_state["board"] = chess.Board()
    game_state["moves"] = []
    game_state["running"] = True
    game_state["status"] = "playing"
    game_state["mode"] = "human"
    game_state["human_side"] = human_side
    game_state["waiting_for_human"] = False
    game_state["pending_human_move"] = None
    game_state["white_name"] = "You" if human_side == "white" else engine_name
    game_state["black_name"] = "You" if human_side == "black" else engine_name
    game_state["white_logs"] = []
    game_state["black_logs"] = []

    _human_move_event.clear()

    thread = threading.Thread(
        target=run_game_human,
        args=(engine_path, human_side, movetime),
        daemon=True
    )
    thread.start()

    return jsonify({"success": True})


@app.route('/human_move', methods=['POST'])
def human_move():
    if not game_state["running"] or not game_state["waiting_for_human"]:
        return jsonify({"error": "Not waiting for human move"})

    data = request.json
    move_str = data.get("move", "")

    if not move_str:
        return jsonify({"error": "No move provided"})

    try:
        chess_move = chess.Move.from_uci(move_str)
        if chess_move not in game_state["board"].legal_moves:
            return jsonify({"error": "Illegal move"})
    except Exception as e:
        return jsonify({"error": str(e)})

    game_state["pending_human_move"] = move_str
    _human_move_event.set()

    return jsonify({"success": True})


@app.route('/stop', methods=['POST'])
def stop_game():
    game_state["running"] = False
    game_state["status"] = "ended"
    game_state["message"] = "Game stopped"
    game_state["waiting_for_human"] = False
    _human_move_event.set()  # unblock waiting thread
    return jsonify({"success": True})


@app.route('/reset', methods=['POST'])
def reset_game():
    game_state["running"] = False
    game_state["board"] = chess.Board()
    game_state["moves"] = []
    game_state["status"] = "waiting"
    game_state["message"] = "Configure and click Start"
    game_state["white_name"] = ""
    game_state["black_name"] = ""
    game_state["last_move"] = None
    game_state["mode"] = "engine"
    game_state["human_side"] = None
    game_state["waiting_for_human"] = False
    game_state["pending_human_move"] = None
    game_state["white_logs"] = []
    game_state["black_logs"] = []
    _human_move_event.set()  # unblock waiting thread
    return get_state()


def _check_game_over(board, side, engine_name, human_side):
    """Check for game-over conditions. Returns a message string or None."""
    if board.is_checkmate():
        winner_side = side  # side that just moved wins
        if human_side:
            winner = "You win!" if winner_side == human_side else f"{engine_name} wins!"
        else:
            winner = "White wins!" if winner_side == "white" else "Black wins!"
        return f"Checkmate! {winner}"
    if board.is_stalemate():
        return "Stalemate - Draw"
    if board.is_insufficient_material():
        return "Draw - Insufficient material"
    if board.can_claim_fifty_moves():
        return "Draw - Fifty move rule"
    if board.is_repetition(3):
        return "Draw - Threefold repetition"
    return None


def run_game(engine1_path, engine2_path, movetime):
    white = UCIEngine(engine1_path, os.path.basename(engine1_path),
                      log_callback=make_log_callback("white"))
    black = UCIEngine(engine2_path, os.path.basename(engine2_path),
                      log_callback=make_log_callback("black"))

    try:
        game_state["message"] = "Starting engines..."
        white.start()
        black.start()
        white.new_game()
        black.new_game()

        current = white
        side = "white"

        while game_state["running"]:
            engine_name = white.name if side == "white" else black.name
            game_state["message"] = f"{engine_name} is thinking..."

            current.set_position(game_state["moves"])
            move = current.get_best_move(movetime=movetime)

            if not game_state["running"]:
                break

            if not is_valid_move(move):
                game_state["message"] = f"No valid move from {engine_name}"
                game_state["status"] = "ended"
                break

            try:
                chess_move = chess.Move.from_uci(move)
                if chess_move in game_state["board"].legal_moves:
                    game_state["board"].push(chess_move)
                    game_state["moves"].append(move)
                    game_state["last_move"] = move
                else:
                    game_state["message"] = f"Illegal move {move} by {engine_name}"
                    game_state["status"] = "ended"
                    break
            except Exception as e:
                game_state["message"] = f"Invalid move {move}: {e}"
                game_state["status"] = "ended"
                break

            end_msg = _check_game_over(game_state["board"], side, None, None)
            if end_msg:
                game_state["message"] = end_msg
                game_state["status"] = "ended"
                game_state["running"] = False
                break

            current = black if side == "white" else white
            side = "black" if side == "white" else "white"

    except Exception as e:
        game_state["message"] = f"Error: {e}"
        game_state["status"] = "ended"
    finally:
        white.quit()
        black.quit()
        game_state["running"] = False


def run_game_human(engine_path, human_side, movetime):
    engine_side = "black" if human_side == "white" else "white"
    engine = UCIEngine(engine_path, os.path.basename(engine_path),
                       log_callback=make_log_callback(engine_side))

    try:
        game_state["message"] = "Starting engine..."
        engine.start()
        engine.new_game()

        while game_state["running"]:
            board = game_state["board"]
            current_side = "white" if board.turn == chess.WHITE else "black"

            if current_side == human_side:
                # Human's turn — wait for /human_move
                _human_move_event.clear()
                game_state["waiting_for_human"] = True
                game_state["message"] = "Your turn"

                _human_move_event.wait()

                if not game_state["running"]:
                    break

                game_state["waiting_for_human"] = False
                move = game_state["pending_human_move"]
                game_state["pending_human_move"] = None

                if not move:
                    continue

            else:
                # Engine's turn
                game_state["waiting_for_human"] = False
                game_state["message"] = f"{engine.name} is thinking..."

                engine.set_position(game_state["moves"])
                move = engine.get_best_move(movetime=movetime)

                if not game_state["running"]:
                    break

                if not is_valid_move(move):
                    game_state["message"] = f"No valid move from {engine.name}"
                    game_state["status"] = "ended"
                    break

            # Apply the move
            try:
                chess_move = chess.Move.from_uci(move)
                if chess_move in game_state["board"].legal_moves:
                    game_state["board"].push(chess_move)
                    game_state["moves"].append(move)
                    game_state["last_move"] = move
                else:
                    game_state["message"] = f"Illegal move: {move}"
                    game_state["status"] = "ended"
                    break
            except Exception as e:
                game_state["message"] = f"Invalid move {move}: {e}"
                game_state["status"] = "ended"
                break

            end_msg = _check_game_over(game_state["board"], current_side, engine.name, human_side)
            if end_msg:
                game_state["message"] = end_msg
                game_state["status"] = "ended"
                game_state["running"] = False
                break

    except Exception as e:
        game_state["message"] = f"Error: {e}"
        game_state["status"] = "ended"
    finally:
        engine.quit()
        game_state["running"] = False
        game_state["waiting_for_human"] = False


if __name__ == "__main__":
    print("Starting Chess GUI server...")
    print("Checking piece images...")
    download_pieces()
    print("Opening browser at http://localhost:5000")
    webbrowser.open("http://localhost:5000")
    app.run(host="localhost", port=5000, debug=False)
