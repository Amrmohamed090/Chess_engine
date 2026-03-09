import { useEffect, useRef } from 'react'

export default function EngineLog({ lines }) {
  const boxRef = useRef(null)
  const prevLen = useRef(0)

  useEffect(() => {
    const el = boxRef.current
    if (!el || lines.length === prevLen.current) return
    prevLen.current = lines.length
    const atBottom = el.scrollHeight - el.scrollTop <= el.clientHeight + 30
    el.innerHTML = lines.map(line => {
      let cls = 'log-line log-recv'
      if (line.startsWith('>>')) cls = 'log-line log-send'
      else if (line.startsWith('<< info')) cls = 'log-line log-info'
      return `<div class="${cls}">${line.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;')}</div>`
    }).join('')
    if (atBottom) el.scrollTop = el.scrollHeight
  }, [lines])

  return <div className="log-box" ref={boxRef} />
}
