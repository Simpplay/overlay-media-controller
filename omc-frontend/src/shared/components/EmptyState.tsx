import type { ReactNode } from 'react'

interface EmptyStateProps {
  icon: ReactNode
  title: string
  description: string
  action?: ReactNode
}

export function EmptyState({ icon, title, description, action }: EmptyStateProps) {
  return (
    <div className="flex flex-col items-center justify-center py-16 text-center gap-3">
      <div className="text-zinc-600 mb-1">{icon}</div>
      <h3 className="text-lg font-semibold text-zinc-300">{title}</h3>
      <p className="text-sm text-zinc-500 max-w-xs">{description}</p>
      {action && <div className="mt-2">{action}</div>}
    </div>
  )
}
