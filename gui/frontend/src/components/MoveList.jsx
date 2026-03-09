import { useEffect, useRef } from 'react'

export default function MoveList({ sanMoves, viewIndex, onGoTo }) {
  const currentRef = useRef(null)

  useEffect(() => {
    currentRef.current?.scrollIntoView({ block: 'nearest' })
  }, [viewIndex])

  const pairs = []
  for (let i = 0; i < sanMoves.length; i += 2) {
    pairs.push([i, sanMoves[i], sanMoves[i + 1]])
  }

  return (
    <div className="moves-box">
      {pairs.map(([i, white, black]) => (
        <span key={i}>
          <span className="move-num">{Math.floor(i / 2) + 1}.</span>
          <span
            className={`move-san${viewIndex === i + 1 ? ' current' : ''}`}
            ref={viewIndex === i + 1 ? currentRef : null}
            onClick={() => onGoTo(i + 1)}
          >{white}</span>
          {black && (
            <span
              className={`move-san${viewIndex === i + 2 ? ' current' : ''}`}
              ref={viewIndex === i + 2 ? currentRef : null}
              onClick={() => onGoTo(i + 2)}
            >{black}</span>
          )}
        </span>
      ))}
    </div>
  )
}
