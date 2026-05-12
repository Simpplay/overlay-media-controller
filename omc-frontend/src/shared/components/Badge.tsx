type BadgeVariant = 'default' | 'violet' | 'green' | 'yellow' | 'red'

const variantClasses: Record<BadgeVariant, string> = {
  default: 'bg-zinc-800 text-zinc-400',
  violet: 'bg-violet-600/20 text-violet-300 border border-violet-600/30',
  green: 'bg-emerald-600/20 text-emerald-300 border border-emerald-600/30',
  yellow: 'bg-yellow-600/20 text-yellow-300 border border-yellow-600/30',
  red: 'bg-red-600/20 text-red-300 border border-red-600/30',
}

interface BadgeProps {
  children: React.ReactNode
  variant?: BadgeVariant
  className?: string
}

export function Badge({ children, variant = 'default', className = '' }: BadgeProps) {
  return (
    <span
      className={[
        'inline-flex items-center gap-1 px-2 py-0.5 rounded-full text-xs font-medium',
        variantClasses[variant],
        className,
      ].join(' ')}
    >
      {children}
    </span>
  )
}
