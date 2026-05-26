import { memo } from 'react'

interface Props {
  active: boolean
  onSelect: (active: boolean) => void
}

export const SoundboardToggle = memo(function SoundboardToggle({ active, onSelect }: Props) {
  return (
    <div className="flex gap-2 overflow-x-auto pb-1 scrollbar-hide">
      <button
        onClick={() => onSelect(!active)}
        className={[
          'flex-shrink-0 px-4 py-1.5 rounded-full text-sm font-medium transition-colors',
          active
            ? 'bg-violet-600 text-white'
            : 'bg-zinc-800 text-zinc-400 hover:text-zinc-100 hover:bg-zinc-700',
        ].join(' ')}
      >
        Soundboard
      </button>
    </div>
  )
})
