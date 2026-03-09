import { useState, useCallback, useRef, useEffect } from 'react'
import { io } from 'socket.io-client'
import { Chess } from 'chess.js'
import ChessBoard from '../components/ChessBoard'
import MoveList from '../components/MoveList'
import EngineLog from '../components/EngineLog'

const START_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'

function buildFenHistory(uciMoves) {
  const game = new Chess()
  const history = [START_FEN]
  for (const m of uciMoves) {
    try {
      const result = game.move({ from: m.slice(0,2), to: m.slice(2,4), promotion: m[4] || 'q' })
      if (!result) break
    } catch { break }
    history.push(game.fen())
  }
  return history
}

export default function Match() {
  const [mode, setMode] = useState('engine')
  const [engine1, setEngine1] = useState('')
  const [engine2, setEngine2] = useState('')
  const [enginePath, setEnginePath] = useState('')
  const [humanColor, setHumanColor] = useState('white')
  const [movetime, setMovetime] = useState(500)

  const [state, setState] = useState(null)
  const [logs, setLogs] = useState({ white: [], black: [] })

  const [fenHistory, setFenHistory] = useState([START_FEN])
  const [viewIndex, setViewIndex] = useState(0)
  const [isLive, setIsLive] = useState(true)
  const isLiveRef = useRef(true)
  const prevMovesRef = useRef([])

  const [copyLabel, setCopyLabel] = useState('Copy PGN')

  // Keep isLiveRef in sync
  useEffect(() => { isLiveRef.current = isLive }, [isLive])

  // WebSocket connection
  useEffect(() => {
    const socket = io({ path: '/socket.io' })

    socket.on('state_update', (data) => {
      setState(data)
      const newMoves = data.moves ?? []
      const hist = buildFenHistory(newMoves)
      setFenHistory(hist)
      prevMovesRef.current = newMoves
      if (isLiveRef.current) setViewIndex(hist.length - 1)
    })

    socket.on('logs_update', (data) => {
      setLogs(data)
    })

    return () => socket.disconnect()
  }, [])

  const goTo = useCallback((idx) => {
    const clamped = Math.max(0, Math.min(idx, prevMovesRef.current.length))
    setViewIndex(clamped)
    const live = clamped === prevMovesRef.current.length
    setIsLive(live)
    isLiveRef.current = live
  }, [])

  const goLive = useCallback(() => {
    const len = prevMovesRef.current.length
    setViewIndex(len)
    setIsLive(true)
    isLiveRef.current = true
  }, [])

  // Keyboard nav
  useEffect(() => {
    const handler = (e) => {
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return
      if (e.key === 'ArrowLeft')  { e.preventDefault(); goTo(viewIndex - 1) }
      if (e.key === 'ArrowRight') { e.preventDefault(); goTo(viewIndex + 1) }
      if (e.key === 'ArrowUp' || e.key === 'Home') { e.preventDefault(); goTo(0) }
      if (e.key === 'ArrowDown' || e.key === 'End') { e.preventDefault(); goLive() }
    }
    window.addEventListener('keydown', handler)
    return () => window.removeEventListener('keydown', handler)
  }, [viewIndex, goTo, goLive])

  async function startGame() {
    const body = mode === 'engine'
      ? { engine1, engine2, movetime }
      : { engine: enginePath, human_side: humanColor, movetime }
    const endpoint = mode === 'engine' ? '/api/start' : '/api/start_human'
    prevMovesRef.current = []
    setFenHistory([START_FEN])
    setViewIndex(0)
    setIsLive(true)
    isLiveRef.current = true
    setLogs({ white: [], black: [] })
    try {
      const r = await fetch(endpoint, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body),
      })
      const data = await r.json()
      if (data.error) alert(data.error)
    } catch (e) {
      console.error('Start failed:', e)
    }
  }

  async function stopGame() {
    await fetch('/api/stop', { method: 'POST' })
  }

  async function resetGame() {
    prevMovesRef.current = []
    setFenHistory([START_FEN])
    setViewIndex(0)
    setIsLive(true)
    isLiveRef.current = true
    setLogs({ white: [], black: [] })
    setState(null)
    await fetch('/api/reset', { method: 'POST' })
  }

  async function copyPgn() {
    const r = await fetch('/api/pgn')
    const text = await r.text()
    await navigator.clipboard.writeText(text)
    setCopyLabel('Copied!')
    setTimeout(() => setCopyLabel('Copy PGN'), 1500)
  }

  async function handleMove(from, to, promo) {
    const uci = from + to + (promo || '')
    const r = await fetch('/api/human_move', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ move: uci }),
    })
    const data = await r.json()
    if (data.error) console.warn('Move rejected:', data.error)
  }

  const running = state?.running ?? false
  const isHuman = mode === 'human'

  const currentFen = fenHistory[viewIndex] ?? START_FEN
  const totalMoves = prevMovesRef.current.length

  const lastUci = state?.moves?.at(-1)
  const lastMove = lastUci ? { from: lastUci.slice(0,2), to: lastUci.slice(2,4) } : null

  const boardOrientation = isHuman ? humanColor : 'white'
  const currentFenTurn = currentFen.split(' ')[1]
  const isWhiteTurn = currentFenTurn === 'w'
  const whiteName = state?.white_name || 'White'
  const blackName = state?.black_name || 'Black'
  const sanMoves = state?.san_moves ?? []

  const statusClass = state?.waiting_for_human && isLive
    ? 'status-your-turn'
    : state?.status === 'playing' ? 'status-thinking'
    : state?.status === 'ended' ? 'status-ended' : ''

  const statusText = !isLive && totalMoves > 0
    ? `Viewing move ${viewIndex} of ${totalMoves}`
    : (state?.message ?? 'Configure engines and click Start')

  return (
    <div className="page" style={{ display: 'flex', gap: 20, alignItems: 'flex-start' }}>

      {/* Board column */}
      <div style={{ flex: '0 0 460px', width: 460 }}>
        <ChessBoard
          fen={currentFen}
          size={460}
          orientation={boardOrientation}
          interactive={isHuman && (state?.waiting_for_human ?? false) && isLive}
          onMove={handleMove}
          lastMove={isLive ? lastMove : null}
        />
        <div className="nav-controls">
          <button className="nav-btn" onClick={() => goTo(0)} disabled={viewIndex === 0}>«</button>
          <button className="nav-btn" onClick={() => goTo(viewIndex - 1)} disabled={viewIndex === 0}>‹</button>
          <button className="nav-btn" onClick={() => goTo(viewIndex + 1)} disabled={viewIndex >= totalMoves}>›</button>
          <button className="nav-btn" onClick={goLive} disabled={isLive}>»</button>
        </div>
      </div>

      {/* Info + Logs column */}
      <div style={{ flex: '0 0 360px', width: 360 }}>

        {/* Mode config */}
        <div className="panel">
          <h3>Game Mode</h3>
          <div className="mode-toggle">
            <button className={`mode-btn${mode === 'engine' ? ' active' : ''}`} onClick={() => setMode('engine')}>Engine vs Engine</button>
            <button className={`mode-btn${mode === 'human' ? ' active' : ''}`} onClick={() => setMode('human')}>Play vs Engine</button>
          </div>
          {mode === 'engine' ? (
            <>
              <label>White Engine:</label>
              <input type="text" value={engine1} onChange={e => setEngine1(e.target.value)} placeholder="Path to white engine" />
              <label>Black Engine:</label>
              <input type="text" value={engine2} onChange={e => setEngine2(e.target.value)} placeholder="Path to black engine" />
            </>
          ) : (
            <>
              <label>Engine Path:</label>
              <input type="text" value={enginePath} onChange={e => setEnginePath(e.target.value)} placeholder="Path to engine" />
              <label>Play as:</label>
              <select value={humanColor} onChange={e => setHumanColor(e.target.value)}>
                <option value="white">White (move first)</option>
                <option value="black">Black (engine moves first)</option>
              </select>
            </>
          )}
          <label>Time per move (ms):</label>
          <input type="number" value={movetime} onChange={e => setMovetime(Number(e.target.value))} min={50} />
        </div>

        {/* Controls */}
        <div className="panel">
          <button className="btn-start" onClick={startGame} disabled={running}>Start</button>
          <button className="btn-stop" onClick={stopGame} disabled={!running}>Stop</button>
          <button className="btn-reset" onClick={resetGame}>Reset</button>
          <button className="btn-copy" onClick={copyPgn}>{copyLabel}</button>
        </div>

        {/* Players + Status */}
        <div className="panel">
          <div className="players">
            <div className={`player-chip${isWhiteTurn && running && isLive ? ' active' : ''}`}>○ {whiteName}</div>
            <div className={`player-chip${!isWhiteTurn && running && isLive ? ' active' : ''}`}>● {blackName}</div>
          </div>
          <div className={`status-bar ${statusClass}`}>{statusText}</div>
        </div>

        {/* Moves */}
        <div className="panel">
          <h3>Moves</h3>
          <MoveList sanMoves={sanMoves} viewIndex={viewIndex} onGoTo={goTo} />
        </div>

        {/* Logs */}
        <div className="panel">
          <div style={{ display: 'flex', gap: 12 }}>
            <div style={{ flex: 1, minWidth: 0 }}>
              <h3>{whiteName} Logs</h3>
              <EngineLog lines={logs.white} />
            </div>
            <div style={{ flex: 1, minWidth: 0 }}>
              <h3>{blackName} Logs</h3>
              <EngineLog lines={logs.black} />
            </div>
          </div>
        </div>

      </div>
    </div>
  )
}
