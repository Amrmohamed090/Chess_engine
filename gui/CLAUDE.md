# Chess GUI — CLAUDE.md

## Working Instructions

- After completing any todo item, mark it as `[x]` in the Todo section below.
- Do not mark an item done unless it is fully implemented and working.
- Add new todo items as they are discovered or requested.
- Keep this file up to date as the source of truth for the GUI's state.

## Stack

- **Backend:** Flask (`chess_gui.py`) — pure JSON API, no templating long-term
- **Match logic:** `chess_match.py` — UCI engine wrapper, `play_game()`, `is_valid_move()`
- **Frontend:** currently vanilla JS + chessboard.js in `templates/index.html`
- **Planned:** migrate to React + Vite frontend, SQLite + SQLAlchemy for persistence

## File Structure

```
gui/
  chess_gui.py          # Flask app — all API routes
  chess_match.py        # UCIEngine class, play_game(), is_valid_move()
  templates/
    index.html          # Current monolithic frontend (to be replaced by React)
  pieces/               # Chess piece PNGs served by Flask
  CLAUDE.md             # This file
```

## API Routes (current)

| Method | Route            | Description                        |
|--------|------------------|------------------------------------|
| GET    | /                | Serves index.html                  |
| GET    | /state           | Current match game state           |
| GET    | /logs            | Engine UCI logs (white/black)      |
| POST   | /start           | Start engine vs engine match       |
| POST   | /start_human     | Start human vs engine match        |
| POST   | /human_move      | Submit human move                  |
| POST   | /stop            | Stop current match                 |
| POST   | /reset           | Reset match                        |
| GET    | /pgn             | Get PGN of current match           |
| POST   | /eval/start      | Start evaluation session           |
| GET    | /eval/state      | Current evaluation state + results |
| POST   | /eval/stop       | Stop evaluation                    |
| GET    | /pieces/<piece>  | Serve piece PNG images             |

## Todo
### BUGS
- [x] Fix the bug where the chess engine UI is frozen, When start is clicked, the game actually works is being played, but the moves are not shown on the board

### Persistence & Engine Registry
- [ ] Design SQLite schema (Engine, EvaluationSession, Game tables)
- [ ] Add SQLAlchemy models
- [ ] API: CRUD for engine objects (name, path, ELO)
- [ ] ELO update logic after each evaluation session
- [ ] Store completed evaluation sessions to DB
- [ ] Store individual game records (moves, result, time control) to DB

### Frontend Migration
- [x] Set up React + Vite project inside `gui/frontend/`
- [x] Set up `concurrently` — `cd frontend && npm run dev` starts both Flask + Vite
- [x] All Flask routes moved to `/api/` Blueprint; static React build served from `gui/static/`
- [x] Migrate Match tab to React component (`src/pages/Match.jsx`)
- [x] Migrate Evaluation tab to React component (`src/pages/Evaluation.jsx`)
- [ ] Add Engines page (registry: create/edit/delete engines, show ELO)
- [ ] Add History page (browse past evaluation sessions)

### Game Viewer / Navigation
- [x] PGN copy button per individual game in evaluation results table
- [ ] Click a completed evaluation game to view it on the board
- [ ] Full move navigation (arrows) for past games
- [ ] Session history browser (list of past evaluation runs with scores)

### Evaluation Improvements
- [ ] Board orientation in eval view follows engine1 (always bottom)
- [ ] Show ELO delta estimate after session ends
