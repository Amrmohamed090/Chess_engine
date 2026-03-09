"""
Chess Engine GUI — Flask API backend with WebSocket support.
Frontend: React (Vite) in frontend/, proxied in dev, served from static/ in production.
"""

from flask import Flask, Blueprint, jsonify, request, send_from_directory, Response
from flask_socketio import SocketIO, emit as ws_emit
import chess
import chess.pgn
import threading
import os
from datetime import datetime
from chess_match import UCIEngine, is_valid_move

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PIECES_DIR = os.path.join(SCRIPT_DIR, "pieces")
STATIC_DIR = os.path.join(SCRIPT_DIR, "static")

app = Flask(__name__, static_folder=STATIC_DIR, static_url_path="")
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='threading')

# All game/eval routes live under /api
api = Blueprint("api", __name__, url_prefix="/api")

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
                import urllib.request
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

eval_state = {
    "running": False,
    "status": "idle",
    "message": "",
    "total_pairs": 0,
    "current_pair": 0,
    "current_game": 0,
    "engine1_name": "",
    "engine2_name": "",
    "movetimes": [],
    "games": [],
    "current_fen": chess.Board().fen(),
    "current_moves": [],
    "current_white": "",
    "current_black": "",
}


# ── WebSocket helpers ─────────────────────────────────────────────────────────

def _build_state():
    san_moves = []
    temp_board = chess.Board()
    for uci_move in game_state["moves"]:
        try:
            move = chess.Move.from_uci(uci_move)
            san_moves.append(temp_board.san(move))
            temp_board.push(move)
        except Exception:
            pass
    return {
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
    }


def _build_eval_state_data():
    e1 = eval_state["engine1_name"]
    e1_wins = e1_draws = e1_losses = 0
    for game in eval_state["games"]:
        is_e1_white = (game["white"] == e1)
        w = game["winner"]
        if w is None:
            e1_draws += 1
        elif (w == "white" and is_e1_white) or (w == "black" and not is_e1_white):
            e1_wins += 1
        else:
            e1_losses += 1
    san_moves = []
    tmp = chess.Board()
    for uci in eval_state["current_moves"]:
        try:
            m = chess.Move.from_uci(uci)
            san_moves.append(tmp.san(m))
            tmp.push(m)
        except Exception:
            pass
    return {
        "running": eval_state["running"],
        "status": eval_state["status"],
        "message": eval_state["message"],
        "total_pairs": eval_state["total_pairs"],
        "current_pair": eval_state["current_pair"],
        "current_game": eval_state["current_game"],
        "engine1_name": e1,
        "engine2_name": eval_state["engine2_name"],
        "engine1_score": e1_wins + 0.5 * e1_draws,
        "engine2_score": e1_losses + 0.5 * e1_draws,
        "engine1_wins": e1_wins,
        "engine1_draws": e1_draws,
        "engine1_losses": e1_losses,
        "movetimes": eval_state["movetimes"],
        "games": eval_state["games"],
        "current_fen": eval_state["current_fen"],
        "current_san_moves": san_moves,
        "current_white": eval_state["current_white"],
        "current_black": eval_state["current_black"],
    }


def emit_game_state():
    socketio.emit('state_update', _build_state())
    socketio.emit('logs_update', {
        "white": game_state["white_logs"],
        "black": game_state["black_logs"],
    })


def emit_eval_state():
    socketio.emit('eval_update', _build_eval_state_data())


@socketio.on('connect')
def on_connect():
    ws_emit('state_update', _build_state())
    ws_emit('logs_update', {
        "white": game_state["white_logs"],
        "black": game_state["black_logs"],
    })
    ws_emit('eval_update', _build_eval_state_data())


# ── Static serving ────────────────────────────────────────────────────────────

@app.route('/pieces/<piece>')
def serve_piece(piece):
    return send_from_directory(PIECES_DIR, piece)


@app.route('/', defaults={'path': ''})
@app.route('/<path:path>')
def serve_react(path):
    index = os.path.join(STATIC_DIR, 'index.html')
    if not os.path.exists(index):
        return (
            "<h2>Dev mode</h2><p>Flask API is running.<br>"
            "Open <a href='http://localhost:5173'>http://localhost:5173</a> for the UI.</p>"
        ), 200
    full = os.path.join(STATIC_DIR, path)
    if path and os.path.isfile(full):
        return send_from_directory(STATIC_DIR, path)
    return send_from_directory(STATIC_DIR, 'index.html')


# ── Log callback ──────────────────────────────────────────────────────────────

def make_log_callback(side):
    def callback(direction, message):
        prefix = ">>" if direction == "send" else "<<"
        entry = f"{prefix} {message}"
        log_list = game_state[f"{side}_logs"]
        log_list.append(entry)
        if len(log_list) > MAX_LOG_LINES:
            del log_list[:-MAX_LOG_LINES]
    return callback


# ── Match API ─────────────────────────────────────────────────────────────────

@api.route('/state')
def get_state():
    return jsonify(_build_state())


@api.route('/logs')
def get_logs():
    return jsonify({
        "white": game_state["white_logs"],
        "black": game_state["black_logs"],
    })


@api.route('/start', methods=['POST'])
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
    game_state["message"] = "Starting engines..."

    emit_game_state()

    thread = threading.Thread(
        target=run_game,
        args=(engine1_path, engine2_path, movetime),
        daemon=True
    )
    thread.start()

    return jsonify({"success": True})


@api.route('/start_human', methods=['POST'])
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
    game_state["message"] = "Starting engine..."

    _human_move_event.clear()
    emit_game_state()

    thread = threading.Thread(
        target=run_game_human,
        args=(engine_path, human_side, movetime),
        daemon=True
    )
    thread.start()

    return jsonify({"success": True})


@api.route('/human_move', methods=['POST'])
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


@api.route('/stop', methods=['POST'])
def stop_game():
    game_state["running"] = False
    game_state["status"] = "ended"
    game_state["message"] = "Game stopped"
    game_state["waiting_for_human"] = False
    _human_move_event.set()
    emit_game_state()
    return jsonify({"success": True})


@api.route('/reset', methods=['POST'])
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
    _human_move_event.set()
    emit_game_state()
    return jsonify({"success": True})


@api.route('/pgn')
def get_pgn():
    game = chess.pgn.Game()
    game.headers["Event"] = "Engine Match"
    game.headers["Date"] = datetime.now().strftime("%Y.%m.%d")
    game.headers["White"] = game_state["white_name"] or "White"
    game.headers["Black"] = game_state["black_name"] or "Black"

    board = game_state["board"]
    if board.is_checkmate():
        game.headers["Result"] = "0-1" if board.turn == chess.WHITE else "1-0"
    elif board.is_stalemate() or board.is_insufficient_material() or \
            board.can_claim_fifty_moves() or board.is_repetition(3):
        game.headers["Result"] = "1/2-1/2"
    else:
        game.headers["Result"] = "*"

    node = game
    temp_board = chess.Board()
    for uci_move in game_state["moves"]:
        try:
            move = chess.Move.from_uci(uci_move)
            node = node.add_variation(move)
            temp_board.push(move)
        except Exception:
            pass

    exporter = chess.pgn.StringExporter(headers=True, variations=True, comments=True)
    pgn_string = game.accept(exporter)

    return Response(pgn_string, mimetype="text/plain")


# ── Game-over helper ──────────────────────────────────────────────────────────

def _check_game_over(board, side, engine_name, human_side):
    if board.is_checkmate():
        winner_side = side
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


# ── Game threads ──────────────────────────────────────────────────────────────

def run_game(engine1_path, engine2_path, movetime):
    white = UCIEngine(engine1_path, os.path.basename(engine1_path),
                      log_callback=make_log_callback("white"))
    black = UCIEngine(engine2_path, os.path.basename(engine2_path),
                      log_callback=make_log_callback("black"))

    try:
        game_state["message"] = "Starting engines..."
        emit_game_state()
        white.start()
        black.start()
        white.new_game()
        black.new_game()

        current = white
        side = "white"

        while game_state["running"]:
            engine_name = white.name if side == "white" else black.name
            game_state["message"] = f"{engine_name} is thinking..."
            emit_game_state()

            current.set_position(game_state["moves"])
            move = current.get_best_move(movetime=movetime)

            if not game_state["running"]:
                break

            if not is_valid_move(move):
                game_state["message"] = f"No valid move from {engine_name}"
                game_state["status"] = "ended"
                emit_game_state()
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
                    emit_game_state()
                    break
            except Exception as e:
                game_state["message"] = f"Invalid move {move}: {e}"
                game_state["status"] = "ended"
                emit_game_state()
                break

            end_msg = _check_game_over(game_state["board"], side, None, None)
            if end_msg:
                game_state["message"] = end_msg
                game_state["status"] = "ended"
                game_state["running"] = False
                emit_game_state()
                break

            emit_game_state()

            current = black if side == "white" else white
            side = "black" if side == "white" else "white"

    except Exception as e:
        game_state["message"] = f"Error: {e}"
        game_state["status"] = "ended"
        emit_game_state()
    finally:
        white.quit()
        black.quit()
        game_state["running"] = False
        emit_game_state()


def run_game_human(engine_path, human_side, movetime):
    engine_side = "black" if human_side == "white" else "white"
    engine = UCIEngine(engine_path, os.path.basename(engine_path),
                       log_callback=make_log_callback(engine_side))

    try:
        game_state["message"] = "Starting engine..."
        emit_game_state()
        engine.start()
        engine.new_game()

        while game_state["running"]:
            board = game_state["board"]
            current_side = "white" if board.turn == chess.WHITE else "black"

            if current_side == human_side:
                _human_move_event.clear()
                game_state["waiting_for_human"] = True
                game_state["message"] = "Your turn"
                emit_game_state()

                _human_move_event.wait()

                if not game_state["running"]:
                    break

                game_state["waiting_for_human"] = False
                move = game_state["pending_human_move"]
                game_state["pending_human_move"] = None

                if not move:
                    continue

            else:
                game_state["waiting_for_human"] = False
                game_state["message"] = f"{engine.name} is thinking..."
                emit_game_state()

                engine.set_position(game_state["moves"])
                move = engine.get_best_move(movetime=movetime)

                if not game_state["running"]:
                    break

                if not is_valid_move(move):
                    game_state["message"] = f"No valid move from {engine.name}"
                    game_state["status"] = "ended"
                    emit_game_state()
                    break

            try:
                chess_move = chess.Move.from_uci(move)
                if chess_move in game_state["board"].legal_moves:
                    game_state["board"].push(chess_move)
                    game_state["moves"].append(move)
                    game_state["last_move"] = move
                else:
                    game_state["message"] = f"Illegal move: {move}"
                    game_state["status"] = "ended"
                    emit_game_state()
                    break
            except Exception as e:
                game_state["message"] = f"Invalid move {move}: {e}"
                game_state["status"] = "ended"
                emit_game_state()
                break

            end_msg = _check_game_over(game_state["board"], current_side, engine.name, human_side)
            if end_msg:
                game_state["message"] = end_msg
                game_state["status"] = "ended"
                game_state["running"] = False
                emit_game_state()
                break

            emit_game_state()

    except Exception as e:
        game_state["message"] = f"Error: {e}"
        game_state["status"] = "ended"
        emit_game_state()
    finally:
        engine.quit()
        game_state["running"] = False
        game_state["waiting_for_human"] = False
        emit_game_state()


# ── Evaluation ────────────────────────────────────────────────────────────────

def run_evaluation(engine1_path, engine2_path, movetimes):
    engine1_name = os.path.basename(engine1_path)
    engine2_name = os.path.basename(engine2_path)
    eval_state["engine1_name"] = engine1_name
    eval_state["engine2_name"] = engine2_name
    eval_state["total_pairs"] = len(movetimes)
    eval_state["movetimes"] = movetimes
    eval_state["games"] = []
    game_num = 0

    try:
        for pair_idx, movetime in enumerate(movetimes):
            if not eval_state["running"]:
                break

            for game_in_pair in range(1, 3):
                if not eval_state["running"]:
                    break

                eval_state["current_pair"] = pair_idx + 1
                eval_state["current_game"] = game_in_pair

                if game_in_pair == 1:
                    white_path, black_path = engine1_path, engine2_path
                    white_name, black_name = engine1_name, engine2_name
                else:
                    white_path, black_path = engine2_path, engine1_path
                    white_name, black_name = engine2_name, engine1_name

                eval_state["current_white"] = white_name
                eval_state["current_black"] = black_name
                eval_state["current_moves"] = []
                board = chess.Board()
                eval_state["current_fen"] = board.fen()
                eval_state["message"] = (
                    f"Pair {pair_idx + 1}/{len(movetimes)}, Game {game_in_pair}/2 "
                    f"({movetime}ms): {white_name} vs {black_name}"
                )
                emit_eval_state()

                white_engine = UCIEngine(white_path, white_name)
                black_engine = UCIEngine(black_path, black_name)
                moves = []
                winner = None
                reason = "unknown"

                try:
                    white_engine.start()
                    black_engine.start()
                    white_engine.new_game()
                    black_engine.new_game()

                    current_engine = white_engine
                    side = "white"

                    while eval_state["running"] and len(moves) < 400:
                        current_engine.set_position(moves)
                        move = current_engine.get_best_move(movetime=movetime)

                        if not eval_state["running"]:
                            break

                        if not is_valid_move(move):
                            winner = "black" if side == "white" else "white"
                            reason = "no valid move"
                            break

                        try:
                            chess_move = chess.Move.from_uci(move)
                            if chess_move in board.legal_moves:
                                board.push(chess_move)
                                moves.append(move)
                                eval_state["current_moves"] = moves[:]
                                eval_state["current_fen"] = board.fen()
                                emit_eval_state()
                            else:
                                winner = "black" if side == "white" else "white"
                                reason = f"illegal move: {move}"
                                break
                        except Exception as e:
                            winner = "black" if side == "white" else "white"
                            reason = f"invalid move: {move}"
                            break

                        if board.is_checkmate():
                            winner = side
                            reason = "checkmate"
                            break
                        elif board.is_stalemate():
                            winner = None
                            reason = "stalemate"
                            break
                        elif board.is_insufficient_material():
                            winner = None
                            reason = "insufficient material"
                            break
                        elif board.can_claim_fifty_moves():
                            winner = None
                            reason = "fifty move rule"
                            break
                        elif board.is_repetition(3):
                            winner = None
                            reason = "threefold repetition"
                            break

                        current_engine = black_engine if side == "white" else white_engine
                        side = "black" if side == "white" else "white"

                    if len(moves) >= 400:
                        winner = None
                        reason = "max moves reached"

                except Exception as e:
                    eval_state["message"] = f"Game error: {e}"
                    emit_eval_state()
                    continue
                finally:
                    white_engine.quit()
                    black_engine.quit()

                if not eval_state["running"] and reason == "unknown":
                    continue

                game_num += 1
                if winner == "white":
                    result_str = "1-0"
                elif winner == "black":
                    result_str = "0-1"
                else:
                    result_str = "1/2-1/2"

                eval_state["games"].append({
                    "num": game_num,
                    "pair": pair_idx + 1,
                    "game_in_pair": game_in_pair,
                    "white": white_name,
                    "black": black_name,
                    "winner": winner,
                    "reason": reason,
                    "result": result_str,
                    "movetime": movetime,
                    "num_moves": len(moves),
                })
                emit_eval_state()

        if eval_state["running"]:
            eval_state["status"] = "done"
            eval_state["message"] = f"Evaluation complete — {game_num} games played"
        else:
            eval_state["status"] = "stopped"
            eval_state["message"] = f"Evaluation stopped after {game_num} games"
    except Exception as e:
        eval_state["status"] = "error"
        eval_state["message"] = f"Error: {e}"
    finally:
        eval_state["running"] = False
        emit_eval_state()


@api.route('/eval/start', methods=['POST'])
def eval_start():
    if eval_state["running"]:
        return jsonify({"error": "Evaluation already running"})

    data = request.json
    engine1_path = data.get("engine1", "").strip()
    engine2_path = data.get("engine2", "").strip()
    num_pairs = max(1, int(data.get("num_pairs", 4)))
    time_mode = data.get("time_mode", "single")

    if not os.path.exists(engine1_path):
        return jsonify({"error": f"Engine 1 not found: {engine1_path}"})
    if not os.path.exists(engine2_path):
        return jsonify({"error": f"Engine 2 not found: {engine2_path}"})

    if time_mode == "single":
        movetime = max(50, int(data.get("movetime", 500)))
        movetimes = [movetime] * num_pairs
    else:
        min_time = max(50, int(data.get("min_time", 100)))
        max_time = max(min_time, int(data.get("max_time", 10000)))
        if num_pairs == 1:
            movetimes = [min_time]
        else:
            movetimes = [
                int(round(min_time + i * (max_time - min_time) / (num_pairs - 1)))
                for i in range(num_pairs)
            ]

    eval_state["running"] = True
    eval_state["status"] = "running"
    eval_state["message"] = "Starting evaluation..."
    eval_state["games"] = []
    eval_state["current_pair"] = 0
    eval_state["current_game"] = 0
    emit_eval_state()

    thread = threading.Thread(
        target=run_evaluation,
        args=(engine1_path, engine2_path, movetimes),
        daemon=True
    )
    thread.start()

    return jsonify({"success": True, "movetimes": movetimes})


@api.route('/eval/state')
def get_eval_state_route():
    return jsonify(_build_eval_state_data())


@api.route('/eval/stop', methods=['POST'])
def eval_stop():
    eval_state["running"] = False
    eval_state["message"] = "Stopping after current game..."
    emit_eval_state()
    return jsonify({"success": True})


app.register_blueprint(api)

if __name__ == "__main__":
    print("Starting Chess GUI API server on http://localhost:5000")
    download_pieces()
    socketio.run(app, host="localhost", port=5000, debug=False, allow_unsafe_werkzeug=True)
