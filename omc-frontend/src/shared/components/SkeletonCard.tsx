interface SkeletonCardProps {
  count?: number
}

function Skeleton({ className = '' }: { className?: string }) {
  return (
    <div className={['bg-zinc-800 animate-pulse rounded', className].join(' ')} />
  )
}

export function SkeletonCard() {
  return (
    <div className="glass rounded-xl p-4 space-y-3">
      <Skeleton className="w-full aspect-video rounded-lg" />
      <Skeleton className="h-4 w-3/4" />
      <Skeleton className="h-3 w-1/2" />
    </div>
  )
}

export function SkeletonGrid({ count = 6 }: SkeletonCardProps) {
  return (
    <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 lg:grid-cols-5 xl:grid-cols-6 gap-3">
      {Array.from({ length: count }).map((_, i) => (
        <SkeletonCard key={i} />
      ))}
    </div>
  )
}

export function SkeletonRow() {
  return (
    <div className="glass rounded-xl p-4 flex items-center gap-4">
      <Skeleton className="w-10 h-10 rounded-lg flex-shrink-0" />
      <div className="flex-1 space-y-2">
        <Skeleton className="h-4 w-1/3" />
        <Skeleton className="h-3 w-1/4" />
      </div>
    </div>
  )
}
