import { useState, useMemo, useCallback } from 'react'
import { useMediaList } from '@/hooks/useMedia'
import { useOverlayList, useCreateOverlay, useUpdateOverlay, useDeleteOverlay } from '@/hooks/useOverlays'
import { useCategoryList } from '@/hooks/useCategories'
import { CategoryFilter } from './components/CategoryFilter'
import { DeckCard } from './components/DeckCard'
import type { Overlay } from '@/types'
import { LayoutGrid } from 'lucide-react'

export function StreamDeckPage() {
  const [activeCategory, setActiveCategory] = useState<number | null>(null)
  const [pendingIds, setPendingIds] = useState<Set<number>>(new Set())

  const { data: media = [], isLoading: mediaLoading } = useMediaList()
  const { data: overlays = [] } = useOverlayList()
  const { data: categories = [] } = useCategoryList()

  const createOverlay = useCreateOverlay()
  const updateOverlay = useUpdateOverlay()
  const deleteOverlay = useDeleteOverlay()

  const filteredMedia = useMemo(() => {
    if (!activeCategory) return media
    return media.filter((m) => m.categories?.includes(activeCategory))
  }, [media, activeCategory])

  const overlayByMediaId = useMemo(() => {
    const map = new Map<number, Overlay>()
    for (const o of overlays) map.set(o.media_id, o)
    return map
  }, [overlays])

  const handleTap = useCallback((mediaId: number) => {
    if (pendingIds.has(mediaId)) return
    setPendingIds(prev => new Set(prev).add(mediaId))
    createOverlay.mutate(
      {
        media_id: mediaId,
        fullscreen: false,
        position: { x: 100, y: 100 },
        size: { width: 800, height: 600 },
      },
      {
        onSettled: () =>
          setPendingIds(prev => {
            const s = new Set(prev)
            s.delete(mediaId)
            return s
          }),
      }
    )
  }, [pendingIds, createOverlay])

  const handleToggleFullscreen = useCallback((overlay: Overlay) => {
    updateOverlay.mutate({ id: overlay.id, payload: { fullscreen: !overlay.fullscreen } })
  }, [updateOverlay])

  const handleClose = useCallback((overlayId: number) => {
    deleteOverlay.mutate(overlayId)
  }, [deleteOverlay])

  return (
    <div className="-mx-4 sm:-mx-6 -my-6">
      {/* Header */}
      <div className="px-4 sm:px-6 pt-5 pb-3 border-b border-zinc-800/50">
        <div className="flex items-center gap-2 mb-3">
          <LayoutGrid className="w-5 h-5 text-violet-400" />
          <h1 className="text-base font-bold text-zinc-100 tracking-tight">StreamDeck</h1>
          {overlays.length > 0 && (
            <span className="ml-auto text-xs text-violet-400 font-semibold">
              {overlays.length} live
            </span>
          )}
        </div>
        <CategoryFilter
          categories={categories}
          active={activeCategory}
          onSelect={setActiveCategory}
        />
      </div>

      {/* Grid */}
      <div className="px-4 sm:px-6 py-4">
        {mediaLoading ? (
          <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 lg:grid-cols-5 xl:grid-cols-6 gap-3">
            {Array.from({ length: 12 }).map((_, i) => (
              <div key={i} className="aspect-square rounded-xl bg-zinc-800 animate-pulse" />
            ))}
          </div>
        ) : filteredMedia.length === 0 ? (
          <div className="flex flex-col items-center justify-center py-24 text-zinc-600">
            <LayoutGrid className="w-10 h-10 mb-3 opacity-30" />
            <p className="text-sm">No media{activeCategory ? ' in selected category' : ''}</p>
          </div>
        ) : (
          <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 lg:grid-cols-5 xl:grid-cols-6 gap-3">
            {filteredMedia.map(m => (
              <DeckCard
                key={m.id}
                media={m}
                overlay={overlayByMediaId.get(m.id)}
                isPending={pendingIds.has(m.id)}
                onTap={handleTap}
                onToggleFullscreen={handleToggleFullscreen}
                onClose={handleClose}
              />
            ))}
          </div>
        )}
      </div>
    </div>
  )
}
