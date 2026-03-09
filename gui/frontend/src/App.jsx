import { useState } from 'react'
import Match from './pages/Match'
import Evaluation from './pages/Evaluation'

const TABS = [
  { id: 'match', label: 'Match' },
  { id: 'eval',  label: 'Evaluation' },
]

export default function App() {
  const [tab, setTab] = useState('match')

  return (
    <>
      <div className="tab-bar">
        {TABS.map(t => (
          <button
            key={t.id}
            className={`tab-btn${tab === t.id ? ' active' : ''}`}
            onClick={() => setTab(t.id)}
          >
            {t.label}
          </button>
        ))}
      </div>

      {tab === 'match' && <Match />}
      {tab === 'eval'  && <Evaluation />}
    </>
  )
}
