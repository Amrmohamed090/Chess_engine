import { useState, useEffect, useMemo } from 'react'
import { Chess } from 'chess.js'

const FILES = ['a','b','c','d','e','f','g','h']
const RANKS = ['8','7','6','5','4','3','2','1']

function parsePieces(fen) {
  const chess = new Chess(fen)
  const result = {}
  for (const f of FILES) {
    for (const r of RANKS) {
      const sq = f + r
      const p = chess.get(sq)
      if (p) result[sq] = p.color + p.type.toUpperCase()
    }
  }
  return result
}

function getLegalTargets(fen, square) {
  if (!square) return new Set()
  try {
    const chess = new Chess(fen)
    return new Set(chess.moves({ square, verbose: true }).map(m => m.to))
  } catch {
    return new Set()
  }
}

/**
 * Props:
 *   fen          – FEN string to display
 *   size         – board pixel size (default 460)
 *   orientation  – 'white' | 'black'
 *   interactive  – enable click-to-move
 *   onMove       – (from, to, promotion?) => void
 *   lastMove     – { from, to } | null
 */
export default function ChessBoard({
  fen,
  size = 460,
  orientation = 'white',
  interactive = false,
  onMove,
  lastMove = null,
}) {
  const [selected, setSelected] = useState(null)
  const [promoDialog, setPromoDialog] = useState(null) // { from, to, color }

  // Clear selection whenever the position changes
  useEffect(() => { setSelected(null) }, [fen])

  const pieces = useMemo(() => {
    try { return parsePieces(fen) } catch { return {} }
  }, [fen])

  const turn = useMemo(() => {
    try { return new Chess(fen).turn() } catch { return 'w' }
  }, [fen])

  const legalTargets = useMemo(
    () => interactive ? getLegalTargets(fen, selected) : new Set(),
    [fen, selected, interactive]
  )

  const sqSize = size / 8

  const ranks = orientation === 'white' ? RANKS : [...RANKS].reverse()
  const files = orientation === 'white' ? FILES : [...FILES].reverse()

  function clickSquare(sq) {
    if (!interactive) return
    const piece = pieces[sq]

    if (selected) {
      if (legalTargets.has(sq)) {
        const movingPiece = pieces[selected]
        const isPromo = movingPiece?.[1] === 'P' &&
          ((movingPiece[0] === 'w' && sq[1] === '8') ||
           (movingPiece[0] === 'b' && sq[1] === '1'))
        if (isPromo) {
          setPromoDialog({ from: selected, to: sq, color: movingPiece[0] })
          setSelected(null)
        } else {
          onMove?.(selected, sq)
          setSelected(null)
        }
      } else if (piece && piece[0] === turn) {
        setSelected(sq)
      } else {
        setSelected(null)
      }
    } else {
      if (piece && piece[0] === turn) setSelected(sq)
    }
  }

  function confirmPromo(type) {
    onMove?.(promoDialog.from, promoDialog.to, type)
    setPromoDialog(null)
  }

  return (
    <div style={{ position: 'relative', width: size, height: size, flexShrink: 0 }}>
      <div style={{
        display: 'grid',
        gridTemplateColumns: `repeat(8, ${sqSize}px)`,
        gridTemplateRows: `repeat(8, ${sqSize}px)`,
        width: size,
        height: size,
        border: '2px solid #3a3a3a',
        boxSizing: 'content-box',
      }}>
        {ranks.flatMap(rank => files.map(file => {
          const sq = file + rank
          const piece = pieces[sq]
          const isLight = (FILES.indexOf(file) + parseInt(rank)) % 2 === 1
          const isSelected = sq === selected
          const isTarget = legalTargets.has(sq)
          const isLastMove = lastMove && (sq === lastMove.from || sq === lastMove.to)

          let bg = isLight ? '#f0d9b5' : '#b58863'
          if (isSelected) bg = '#f6f669'
          else if (isLastMove) bg = isLight ? '#cdd26a' : '#aaa23a'

          return (
            <div
              key={sq}
              onClick={() => clickSquare(sq)}
              style={{
                width: sqSize,
                height: sqSize,
                background: bg,
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                cursor: interactive ? 'pointer' : 'default',
                position: 'relative',
                boxSizing: 'border-box',
              }}
            >
              {/* Legal move dot (empty square) */}
              {isTarget && !piece && (
                <div style={{
                  width: '34%',
                  height: '34%',
                  borderRadius: '50%',
                  background: 'rgba(0,0,0,0.18)',
                  pointerEvents: 'none',
                }} />
              )}
              {/* Legal move ring (occupied square) */}
              {isTarget && piece && (
                <div style={{
                  position: 'absolute',
                  inset: 0,
                  border: `${Math.max(3, Math.round(sqSize * 0.08))}px solid rgba(0,0,0,0.28)`,
                  borderRadius: '50%',
                  boxSizing: 'border-box',
                  pointerEvents: 'none',
                }} />
              )}
              {piece && (
                <img
                  src={`/pieces/${piece}.png`}
                  style={{
                    width: '88%',
                    height: '88%',
                    display: 'block',
                    userSelect: 'none',
                    pointerEvents: 'none',
                    position: 'relative',
                    zIndex: 1,
                  }}
                  draggable={false}
                  alt={piece}
                />
              )}
            </div>
          )
        }))}
      </div>

      {/* Promotion dialog */}
      {promoDialog && (
        <div style={{
          position: 'absolute',
          inset: 0,
          background: 'rgba(0,0,0,0.65)',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          zIndex: 20,
        }}>
          <div style={{
            background: '#2a2a2a',
            borderRadius: 8,
            padding: '16px 20px',
            display: 'flex',
            flexDirection: 'column',
            alignItems: 'center',
            gap: 12,
          }}>
            <div style={{ color: '#ddd', fontSize: 14 }}>Promote to:</div>
            <div style={{ display: 'flex', gap: 10 }}>
              {['Q','R','B','N'].map(p => (
                <div
                  key={p}
                  onClick={() => confirmPromo(p.toLowerCase())}
                  style={{
                    width: sqSize * 0.8,
                    height: sqSize * 0.8,
                    background: '#3a3a3a',
                    borderRadius: 6,
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    cursor: 'pointer',
                  }}
                >
                  <img
                    src={`/pieces/${promoDialog.color}${p}.png`}
                    style={{ width: '80%' }}
                    alt={p}
                  />
                </div>
              ))}
            </div>
          </div>
        </div>
      )}
    </div>
  )
}
