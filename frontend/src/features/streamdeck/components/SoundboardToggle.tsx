import { memo } from 'react'
import { AudioLines } from 'lucide-react'

interface Props {
  active: boolean
  onSelect: (active: boolean) => void
}

export const SoundboardToggle = memo(function SoundboardToggle({ active, onSelect }: Props) {
  return (
    <button
      onClick={() => onSelect(!active)}
      className={`
        group flex items-center gap-2 px-3 py-1.5 rounded-lg text-sm font-medium transition-all duration-200
        ${active
          ? 'bg-violet-500/10 text-violet-400 border border-violet-500/30 shadow-[0_0_10px_rgba(139,92,246,0.1)]'
          : 'bg-zinc-800/50 text-zinc-400 border border-zinc-800 hover:text-zinc-200 hover:bg-zinc-800'
        }
      `}
    >
      <AudioLines 
        className={`w-4 h-4 transition-transform duration-200 ${active ? 'animate-pulse' : 'group-hover:scale-110'}`} 
      />
      Soundboard Mode
    </button>
  )
})