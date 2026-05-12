import { memo } from 'react'
import type { CategorySummary } from '@/types'

interface Props {
  categories: CategorySummary[]
  active: number | null
  onSelect: (catId: number | null) => void
}

export const CategoryFilter = memo(function CategoryFilter({ categories, active, onSelect }: Props) {
  return (
    <div className="flex gap-2 overflow-x-auto pb-1 scrollbar-hide">
      <button
        onClick={() => onSelect(null)}
        className={[
          'flex-shrink-0 px-4 py-1.5 rounded-full text-sm font-medium transition-colors',
          active === null
            ? 'bg-violet-600 text-white'
            : 'bg-zinc-800 text-zinc-400 hover:text-zinc-100 hover:bg-zinc-700',
        ].join(' ')}
      >
        All
      </button>
      {categories.map(cat => (
        <button
          key={cat.id}
          onClick={() => onSelect(cat.id)}
          className={[
            'flex-shrink-0 px-4 py-1.5 rounded-full text-sm font-medium transition-colors',
            active === cat.id
              ? 'bg-violet-600 text-white'
              : 'bg-zinc-800 text-zinc-400 hover:text-zinc-100 hover:bg-zinc-700',
          ].join(' ')}
        >
          {cat.name}
        </button>
      ))}
    </div>
  )
})
