import { useState, useEffect } from 'react'
import { io } from 'socket.io-client'
import ChessBoard from '../components/ChessBoard'

const START_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'

function fmtScore(s) {
  return Number.isInteger(s) ? String(s) : s.toFixed(1)
}

function calcBundleTimes(n, minT, maxT) {
  if (n === 1) return [minT]
  return Array.from({ length: n }, (_, i) => Math.round(minT + i * (maxT - minT) / (n - 1)))
}

export default function Evaluation() {
  const [engine1, setEngine1] = useState('')
  const [engine2, setEngine2] = useState('')
  const [numPairs, setNumPairs] = useState(4)
  const [timeMode, setTimeMode] = useState('single')
  const [movetime, setMovetime] = useState(500)
  const [minTime, setMinTime] = useState(100)
  const [maxTime, setMaxTime] = useState(10000)

  const [evalData, setEvalData] = useState(null)

  // WebSocket connection
  useEffect(() => {
    const socket = io({ path: '/socket.io' })
    socket.on('eval_update', (data) => {
      setEvalData(data)
    })
    return () => socket.disconnect()
  }, [])

  async function startEval() {
    if (!engine1 || !engine2) { alert('Enter both engine paths'); return }
    const body = { engine1, engine2, num_pairs: numPairs, time_mode: timeMode }
    if (timeMode === 'single') body.movetime = movetime
    else { body.min_time = minTime; body.max_time = maxTime }

    const r = await fetch('/api/eval/start', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    })
    const data = await r.json()
    if (data.error) { alert(data.error); return }
  }

  async function stopEval() {
    await fetch('/api/eval/stop', { method: 'POST' })
  }

  async function copyGamePgn(game) {
    const result = game.result
    const pgn = `[White "${game.white}"]\n[Black "${game.black}"]\n[Result "${result}"]\n\n${result}`
    await navigator.clipboard.writeText(pgn)
  }

  const running = evalData?.running ?? false
  const d = evalData
  const schedulePreview = timeMode === 'bundle'
    ? calcBundleTimes(numPairs, minTime, maxTime).map(t => `${t}ms`).join(' → ')
    : null

  return (
    <div className="page">
      <div style={{ display: 'flex', gap: 16, alignItems: 'flex-start', maxWidth: 1160, margin: '0 auto' }}>

        {/* Config */}
        <div style={{ flex: '0 0 260px', width: 260 }}>
          <div className="panel">
            <h3>Engines</h3>
            <label>Engine 1:</label>
            <input type="text" value={engine1} onChange={e => setEngine1(e.target.value)} placeholder="Path to engine 1" />
            <label>Engine 2:</label>
            <input type="text" value={engine2} onChange={e => setEngine2(e.target.value)} placeholder="Path to engine 2" />
            <label>Pairs of games:</label>
            <input type="number" value={numPairs} min={1} max={200} onChange={e => setNumPairs(Number(e.target.value))} />

            <h3>Time Control</h3>
            <div className="mode-toggle">
              <button className={`mode-btn${timeMode === 'single' ? ' active' : ''}`} onClick={() => setTimeMode('single')}>Single</button>
              <button className={`mode-btn${timeMode === 'bundle' ? ' active' : ''}`} onClick={() => setTimeMode('bundle')}>Bundle</button>
            </div>

            {timeMode === 'single' ? (
              <>
                <label>Time per move (ms):</label>
                <input type="number" value={movetime} min={50} onChange={e => setMovetime(Number(e.target.value))} />
              </>
            ) : (
              <>
                <label>Min time (ms):</label>
                <input type="number" value={minTime} min={50} onChange={e => setMinTime(Number(e.target.value))} />
                <label>Max time (ms):</label>
                <input type="number" value={maxTime} min={50} onChange={e => setMaxTime(Number(e.target.value))} />
                <div style={{ fontSize: 12, color: '#888', marginTop: -10, marginBottom: 14, lineHeight: 1.5 }}>
                  {schedulePreview}
                </div>
              </>
            )}

            <button className="btn-start" onClick={startEval} disabled={running}>Start Evaluation</button>
            <button className="btn-stop" onClick={stopEval} disabled={!running}>Stop</button>
          </div>
        </div>

        {/* Live board */}
        <div style={{ flex: '0 0 360px', width: 360 }}>
          <div className="panel" style={{ padding: 15 }}>
            <div style={{ fontSize: 14, fontWeight: 'bold', marginBottom: 8, padding: '6px 10px', background: '#1a1917', borderRadius: 4 }}>
              ● {d?.current_black || '—'}
            </div>
            <ChessBoard
              fen={d?.current_fen || START_FEN}
              size={320}
              interactive={false}
            />
            <div style={{ fontSize: 14, fontWeight: 'bold', marginTop: 8, padding: '6px 10px', background: '#1a1917', borderRadius: 4 }}>
              ○ {d?.current_white || '—'}
            </div>
            <div style={{
              marginTop: 10, background: '#1a1917', borderRadius: 4,
              padding: '8px 10px', maxHeight: 80, overflowY: 'auto', overflowX: 'hidden',
              fontFamily: 'monospace', fontSize: 13, lineHeight: 1.7, color: '#ccc',
              wordBreak: 'break-word',
            }}>
              {d?.current_san_moves?.length
                ? d.current_san_moves.reduce((acc, mv, i) => {
                    if (i % 2 === 0) acc.push(`${Math.floor(i/2)+1}. ${mv}`)
                    else acc[acc.length - 1] += ` ${mv}`
                    return acc
                  }, []).join('  ')
                : '—'}
            </div>
          </div>
        </div>

        {/* Results */}
        <div style={{ flex: 1, minWidth: 0 }}>

          {/* Score */}
          <div className="panel">
            <h3>Score</h3>
            {d?.engine1_name && d?.games?.length > 0 ? (
              <div className="score-row">
                <div className="score-engine">
                  <div style={{ fontWeight: 'bold', fontSize: 14 }}>{d.engine1_name}</div>
                  <div className="score-pts" style={{ color: '#81b64c' }}>{fmtScore(d.engine1_score)}</div>
                  <div className="score-wdl">{d.engine1_wins}W {d.engine1_draws}D {d.engine1_losses}L</div>
                </div>
                <div className="score-bar-wrap">
                  <div className="score-bar-fill" style={{ width: `${d.games.length > 0 ? (d.engine1_score / d.games.length * 100) : 50}%` }} />
                </div>
                <div className="score-engine right">
                  <div style={{ fontWeight: 'bold', fontSize: 14 }}>{d.engine2_name}</div>
                  <div className="score-pts" style={{ color: '#e84c4c' }}>{fmtScore(d.engine2_score)}</div>
                  <div className="score-wdl">{d.engine1_losses}W {d.engine1_draws}D {d.engine1_wins}L</div>
                </div>
              </div>
            ) : (
              <span style={{ color: '#666', fontSize: 14 }}>No games played yet</span>
            )}
          </div>

          {/* Status + progress */}
          <div className="panel">
            <div style={{
              fontSize: 15,
              color: d?.status === 'running' ? '#f0c36d' : d?.status === 'done' ? '#81b64c' : '#bababa'
            }}>
              {d?.message || 'Configure engines and click Start Evaluation'}
            </div>
            {running && d?.total_pairs > 0 && (() => {
              const done = (d.current_pair - 1) * 2 + (d.current_game - 1)
              const total = d.total_pairs * 2
              const pct = Math.round(done / total * 100)
              return (
                <div style={{ marginTop: 10 }}>
                  <div style={{ background: '#1a1917', borderRadius: 4, height: 6, overflow: 'hidden', marginBottom: 4 }}>
                    <div style={{ height: '100%', width: `${pct}%`, background: '#629924', transition: 'width 0.4s' }} />
                  </div>
                  <div style={{ fontSize: 12, color: '#888' }}>{done} / {total} games</div>
                </div>
              )
            })()}
          </div>

          {/* Results table */}
          <div className="panel">
            <h3>Game Results</h3>
            <div style={{ overflowX: 'auto' }}>
              {d?.games?.length > 0 ? (
                <table className="eval-table">
                  <thead>
                    <tr><th>#</th><th>Pair</th><th>Time(ms)</th><th>White</th><th>Black</th><th>Result</th><th>Reason</th><th>Moves</th><th></th></tr>
                  </thead>
                  <tbody>
                    {[...d.games].reverse().map(g => {
                      const isE1White = g.white === d.engine1_name
                      const rowCls = g.winner === null ? 'e1-draw'
                        : ((g.winner === 'white' && isE1White) || (g.winner === 'black' && !isE1White)) ? 'e1-win'
                        : 'e1-loss'
                      const resCls = g.winner === null ? 'r-draw'
                        : rowCls === 'e1-win' ? 'r-win' : 'r-loss'
                      return (
                        <tr key={g.num} className={rowCls}>
                          <td>{g.num}</td>
                          <td>{g.pair}-{g.game_in_pair}</td>
                          <td>{g.movetime}</td>
                          <td>{g.white}</td>
                          <td>{g.black}</td>
                          <td><span className={resCls}>{g.result}</span></td>
                          <td style={{ color: '#888', fontSize: 12 }}>{g.reason}</td>
                          <td>{g.num_moves}</td>
                          <td>
                            <button className="btn-copy" style={{ padding: '3px 8px', fontSize: 11, margin: 0 }}
                              onClick={() => copyGamePgn(g)}>PGN</button>
                          </td>
                        </tr>
                      )
                    })}
                  </tbody>
                </table>
              ) : (
                <p style={{ color: '#666', fontSize: 14, margin: 0 }}>No games played yet</p>
              )}
            </div>
          </div>

        </div>
      </div>
    </div>
  )
}
