"""
Chess Engine Match GUI using chessboard.js
Watch two UCI chess engines play against each other in your browser.
"""

from flask import Flask, render_template_string, jsonify, request, send_from_directory
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

# Global game state
game_state = {
    "board": chess.Board(),
    "moves": [],
    "status": "waiting",
    "message": "Configure engines and click Start",
    "running": False,
    "white_name": "",
    "black_name": "",
    "last_move": None
}

HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Chess Engine Match</title>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/@chrisoakman/chessboardjs@1.0.0/dist/chessboard-1.0.0.min.css">
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #312e2b;
            color: #fff;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            display: flex;
            gap: 30px;
        }
        .board-section {
            flex: 0 0 auto;
        }
        #board {
            width: 560px;
        }
        .info-section {
            flex: 1;
            min-width: 300px;
        }
        .panel {
            background: #262421;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
        }
        h3 {
            margin: 0 0 15px 0;
            color: #bababa;
            font-size: 14px;
            text-transform: uppercase;
        }
        label {
            display: block;
            margin-bottom: 5px;
            color: #bababa;
            font-size: 14px;
        }
        input[type="text"], input[type="number"] {
            width: 100%;
            padding: 10px;
            border: none;
            border-radius: 4px;
            background: #1a1917;
            color: #fff;
            font-size: 14px;
            box-sizing: border-box;
            margin-bottom: 15px;
        }
        button {
            padding: 12px 24px;
            border: none;
            border-radius: 4px;
            font-size: 14px;
            font-weight: bold;
            cursor: pointer;
            margin-right: 10px;
            margin-bottom: 10px;
        }
        .btn-start {
            background: #629924;
            color: #fff;
        }
        .btn-start:hover {
            background: #7cb32a;
        }
        .btn-stop {
            background: #b33430;
            color: #fff;
        }
        .btn-stop:hover {
            background: #c43c38;
        }
        .btn-reset {
            background: #4a4a4a;
            color: #fff;
        }
        .btn-reset:hover {
            background: #5a5a5a;
        }
        button:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }
        .status {
            padding: 15px;
            border-radius: 4px;
            background: #1a1917;
            font-size: 16px;
        }
        .status.thinking {
            color: #f0c36d;
        }
        .status.ended {
            color: #81b64c;
        }
        .players {
            display: flex;
            justify-content: space-between;
            margin-bottom: 15px;
            font-size: 14px;
        }
        .player {
            padding: 8px 12px;
            background: #1a1917;
            border-radius: 4px;
        }
        .player.white::before {
            content: "○ ";
        }
        .player.black::before {
            content: "● ";
        }
        .player.active {
            background: #629924;
        }
        .moves-list {
            background: #1a1917;
            border-radius: 4px;
            padding: 15px;
            max-height: 300px;
            overflow-y: auto;
            font-family: monospace;
            font-size: 14px;
            line-height: 1.8;
        }
        .move-number {
            color: #888;
            margin-right: 5px;
        }
        .move {
            color: #fff;
            margin-right: 15px;
            cursor: pointer;
            padding: 2px 4px;
            border-radius: 2px;
        }
        .move:hover {
            background: #3a3a3a;
        }
        .move.current {
            background: #629924;
        }
        .nav-controls {
            display: flex;
            justify-content: center;
            gap: 5px;
            margin-top: 15px;
        }
        .nav-btn {
            background: #262421;
            color: #bababa;
            border: none;
            border-radius: 4px;
            padding: 10px 20px;
            font-size: 18px;
            cursor: pointer;
            min-width: 50px;
        }
        .nav-btn:hover {
            background: #3a3a3a;
            color: #fff;
        }
        .nav-btn:disabled {
            opacity: 0.3;
            cursor: not-allowed;
        }
        .nav-btn:disabled:hover {
            background: #262421;
            color: #bababa;
        }
        .live-indicator {
            display: inline-block;
            background: #629924;
            color: #fff;
            padding: 4px 10px;
            border-radius: 4px;
            font-size: 12px;
            margin-left: 10px;
        }
        .live-indicator.paused {
            background: #b33430;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="board-section">
            <div id="board"></div>
            <div class="nav-controls">
                <button class="nav-btn" id="btnFirst" onclick="goToMove(0)" title="First move">&laquo;</button>
                <button class="nav-btn" id="btnPrev" onclick="goToMove(viewIndex - 1)" title="Previous move (Left Arrow)">&lsaquo;</button>
                <button class="nav-btn" id="btnNext" onclick="goToMove(viewIndex + 1)" title="Next move (Right Arrow)">&rsaquo;</button>
                <button class="nav-btn" id="btnLast" onclick="goToLive()" title="Last move">&raquo;</button>
            </div>
        </div>
        <div class="info-section">
            <div class="panel">
                <h3>Engines</h3>
                <label>White Engine:</label>
                <input type="text" id="engine1" placeholder="Path to white engine (.exe)">
                <label>Black Engine:</label>
                <input type="text" id="engine2" placeholder="Path to black engine (.exe)">
                <label>Time per move (ms):</label>
                <input type="number" id="movetime" value="500" min="100" max="60000">
            </div>

            <div class="panel">
                <button class="btn-start" id="startBtn" onclick="startGame()">Start Game</button>
                <button class="btn-stop" id="stopBtn" onclick="stopGame()" disabled>Stop</button>
                <button class="btn-reset" onclick="resetGame()">Reset</button>
            </div>

            <div class="panel">
                <h3>Players</h3>
                <div class="players">
                    <span class="player white" id="whitePlayer">White</span>
                    <span class="player black" id="blackPlayer">Black</span>
                </div>
                <div class="status" id="status">Configure engines and click Start</div>
            </div>

            <div class="panel">
                <h3>Moves</h3>
                <div class="moves-list" id="movesList"></div>
            </div>
        </div>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/jquery@3.7.1/dist/jquery.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/@chrisoakman/chessboardjs@1.0.0/dist/chessboard-1.0.0.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/chess.js@0.12.0/chess.min.js"></script>
    <script>
        var board = Chessboard('board', {
            position: 'start',
            pieceTheme: '/pieces/{piece}.png'
        });

        var polling = null;
        var currentMoves = [];      // All moves from server
        var fenHistory = ['rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'];  // FEN at each position
        var viewIndex = 0;          // Current viewing position (0 = start, moves.length = end)
        var isLive = true;          // Follow live game?

        function buildFenHistory(moves) {
            fenHistory = ['rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'];
            var tempBoard = new Chess();
            for (var i = 0; i < moves.length; i++) {
                tempBoard.move(moves[i], {sloppy: true});
                fenHistory.push(tempBoard.fen());
            }
        }

        function goToMove(index) {
            if (index < 0) index = 0;
            if (index > currentMoves.length) index = currentMoves.length;
            viewIndex = index;
            isLive = (viewIndex === currentMoves.length);
            board.position(fenHistory[viewIndex]);
            updateMovesList();
            updateNavButtons();
        }

        function goToLive() {
            isLive = true;
            viewIndex = currentMoves.length;
            board.position(fenHistory[viewIndex]);
            updateMovesList();
            updateNavButtons();
        }

        function updateNavButtons() {
            document.getElementById('btnFirst').disabled = (viewIndex === 0);
            document.getElementById('btnPrev').disabled = (viewIndex === 0);
            document.getElementById('btnNext').disabled = (viewIndex >= currentMoves.length);
            document.getElementById('btnLast').disabled = isLive;
        }

        function updateMovesList() {
            var movesHtml = '';
            var sanMoves = currentState ? currentState.san_moves : [];
            for (var i = 0; i < sanMoves.length; i += 2) {
                var moveNum = Math.floor(i / 2) + 1;
                movesHtml += '<span class="move-number">' + moveNum + '.</span>';
                var isCurrent1 = (i + 1 === viewIndex);
                movesHtml += '<span class="move' + (isCurrent1 ? ' current' : '') + '" onclick="goToMove(' + (i + 1) + ')">' + sanMoves[i] + '</span>';
                if (i + 1 < sanMoves.length) {
                    var isCurrent2 = (i + 2 === viewIndex);
                    movesHtml += '<span class="move' + (isCurrent2 ? ' current' : '') + '" onclick="goToMove(' + (i + 2) + ')">' + sanMoves[i + 1] + '</span>';
                }
            }
            document.getElementById('movesList').innerHTML = movesHtml;

            // Scroll to current move
            var currentEl = document.querySelector('.move.current');
            if (currentEl) {
                currentEl.scrollIntoView({block: 'nearest'});
            }
        }

        var currentState = null;

        function updateState(state) {
            currentState = state;
            var movesChanged = (JSON.stringify(currentMoves) !== JSON.stringify(state.moves));

            if (movesChanged) {
                currentMoves = state.moves.slice();
                buildFenHistory(currentMoves);

                // If following live, update viewIndex
                if (isLive) {
                    viewIndex = currentMoves.length;
                }
            }

            // Update board position based on viewIndex
            board.position(fenHistory[viewIndex]);

            var statusEl = document.getElementById('status');
            var statusText = state.message;
            if (!isLive && currentMoves.length > 0) {
                statusText = 'Viewing move ' + viewIndex + ' of ' + currentMoves.length;
            }
            statusEl.textContent = statusText;
            statusEl.className = 'status';
            if (state.status === 'playing') {
                statusEl.classList.add('thinking');
            } else if (state.status === 'ended') {
                statusEl.classList.add('ended');
            }

            document.getElementById('whitePlayer').textContent = state.white_name || 'White';
            document.getElementById('blackPlayer').textContent = state.black_name || 'Black';

            var viewFen = fenHistory[viewIndex];
            var isWhiteTurn = viewFen.split(' ')[1] === 'w';
            document.getElementById('whitePlayer').classList.toggle('active', isWhiteTurn && state.running && isLive);
            document.getElementById('blackPlayer').classList.toggle('active', !isWhiteTurn && state.running && isLive);

            updateMovesList();
            updateNavButtons();

            document.getElementById('startBtn').disabled = state.running;
            document.getElementById('stopBtn').disabled = !state.running;
        }

        function pollState() {
            fetch('/state')
                .then(response => response.json())
                .then(updateState)
                .catch(err => console.error('Poll error:', err));
        }

        function startGame() {
            var engine1 = document.getElementById('engine1').value;
            var engine2 = document.getElementById('engine2').value;
            var movetime = document.getElementById('movetime').value;

            if (!engine1 || !engine2) {
                alert('Please enter paths to both engines');
                return;
            }

            // Reset view state
            currentMoves = [];
            fenHistory = ['rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'];
            viewIndex = 0;
            isLive = true;

            fetch('/start', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({engine1: engine1, engine2: engine2, movetime: parseInt(movetime)})
            })
            .then(response => response.json())
            .then(data => {
                if (data.error) {
                    alert(data.error);
                } else {
                    if (polling) clearInterval(polling);
                    polling = setInterval(pollState, 200);
                }
            });
        }

        function stopGame() {
            fetch('/stop', {method: 'POST'})
                .then(response => response.json())
                .then(pollState);
        }

        function resetGame() {
            fetch('/reset', {method: 'POST'})
                .then(response => response.json())
                .then(data => {
                    currentMoves = [];
                    fenHistory = ['rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'];
                    viewIndex = 0;
                    isLive = true;
                    updateState(data);
                    if (polling) {
                        clearInterval(polling);
                        polling = null;
                    }
                });
        }

        // Keyboard navigation
        document.addEventListener('keydown', function(e) {
            if (e.target.tagName === 'INPUT') return; // Don't interfere with input fields

            if (e.key === 'ArrowLeft') {
                e.preventDefault();
                goToMove(viewIndex - 1);
            } else if (e.key === 'ArrowRight') {
                e.preventDefault();
                goToMove(viewIndex + 1);
            } else if (e.key === 'ArrowUp' || e.key === 'Home') {
                e.preventDefault();
                goToMove(0);
            } else if (e.key === 'ArrowDown' || e.key === 'End') {
                e.preventDefault();
                goToLive();
            }
        });

        pollState();
    </script>
</body>
</html>
"""


@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)


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
        "black_name": game_state["black_name"]
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
    game_state["white_name"] = os.path.basename(engine1_path)
    game_state["black_name"] = os.path.basename(engine2_path)

    thread = threading.Thread(
        target=run_game,
        args=(engine1_path, engine2_path, movetime),
        daemon=True
    )
    thread.start()

    return jsonify({"success": True})


@app.route('/stop', methods=['POST'])
def stop_game():
    game_state["running"] = False
    game_state["status"] = "ended"
    game_state["message"] = "Game stopped"
    return jsonify({"success": True})


@app.route('/reset', methods=['POST'])
def reset_game():
    game_state["running"] = False
    game_state["board"] = chess.Board()
    game_state["moves"] = []
    game_state["status"] = "waiting"
    game_state["message"] = "Configure engines and click Start"
    game_state["white_name"] = ""
    game_state["black_name"] = ""
    game_state["last_move"] = None
    return get_state()


def run_game(engine1_path, engine2_path, movetime):
    white = UCIEngine(engine1_path, os.path.basename(engine1_path))
    black = UCIEngine(engine2_path, os.path.basename(engine2_path))

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

            if game_state["board"].is_checkmate():
                winner = "White" if side == "white" else "Black"
                game_state["message"] = f"Checkmate! {winner} wins!"
                game_state["status"] = "ended"
                game_state["running"] = False
                break

            if game_state["board"].is_stalemate():
                game_state["message"] = "Stalemate - Draw"
                game_state["status"] = "ended"
                game_state["running"] = False
                break

            if game_state["board"].is_insufficient_material():
                game_state["message"] = "Draw - Insufficient material"
                game_state["status"] = "ended"
                game_state["running"] = False
                break

            if game_state["board"].can_claim_fifty_moves():
                game_state["message"] = "Draw - Fifty move rule"
                game_state["status"] = "ended"
                game_state["running"] = False
                break

            if game_state["board"].is_repetition(3):
                game_state["message"] = "Draw - Threefold repetition"
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


if __name__ == "__main__":
    print("Starting Chess GUI server...")
    print("Checking piece images...")
    download_pieces()
    print("Opening browser at http://localhost:5000")
    webbrowser.open("http://localhost:5000")
    app.run(host="localhost", port=5000, debug=False)
