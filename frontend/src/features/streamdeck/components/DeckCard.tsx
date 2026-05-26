import { memo, useRef, useState, useCallback } from 'react'
import { Loader2, Maximize2, Minimize2, X, Film } from 'lucide-react'
import type { Media, Overlay } from '@/types'

interface Props {
  media: Media
  overlay: Overlay | undefined
  isPending: boolean
  onTap: (mediaId: number, isSoundboard: boolean) => void
  onToggleFullscreen: (overlay: Overlay) => void
  onClose: (overlayId: number) => void
  isSoundboard: boolean
}

const LONG_PRESS_MS = 500

export const DeckCard = memo(function DeckCard({
  media, overlay, isPending, onTap, onToggleFullscreen, onClose, isSoundboard
}: Props) {
  const [showActions, setShowActions] = useState(false)
  const [imgFailed, setImgFailed] = useState(false)
  const timerRef = useRef<ReturnType<typeof setTimeout> | null>(null)
  const didLongPress = useRef(false)

  const isActive = !!overlay

  const handlePressStart = useCallback(() => {
    didLongPress.current = false
    timerRef.current = setTimeout(() => {
      if (overlay) {
        didLongPress.current = true
        setShowActions(true)
      }
    }, LONG_PRESS_MS)
  }, [overlay])

  const cancelTimer = useCallback(() => {
    if (timerRef.current) clearTimeout(timerRef.current)
  }, [])

  const handleClick = useCallback(() => {
    cancelTimer()
    if (didLongPress.current) return
    if (!showActions) onTap(media.id, isSoundboard)
  }, [cancelTimer, showActions, media.id, onTap])

  const closeActions = useCallback((e: React.MouseEvent) => {
    e.stopPropagation()
    setShowActions(false)
  }, [])

  return (
    <div className="relative select-none touch-manipulation">
      <button
        onPointerDown={handlePressStart}
        onPointerUp={cancelTimer}
        onPointerLeave={cancelTimer}
        onClick={handleClick}
        aria-label={media.title ?? media.filename}
        className={[
          'w-full aspect-square rounded-xl overflow-hidden relative bg-zinc-800',
          'transition-transform duration-100 active:scale-95',
          isActive
            ? 'ring-2 ring-violet-500 shadow-[0_0_16px_2px_rgba(139,92,246,0.4)]'
            : 'ring-1 ring-zinc-700/60 hover:ring-zinc-500/60',
          isPending ? 'opacity-60' : '',
        ].join(' ')}
      >
        {/* Thumbnail */}
        {!imgFailed ? (
          <img
            src={`/api/media/${media.id}/thumbnail`}
            alt=""
            draggable={false}
            loading="lazy"
            onError={() => setImgFailed(true)}
            className="absolute inset-0 w-full h-full object-cover"
          />
        ) : (
          <div className="absolute inset-0 flex items-center justify-center bg-zinc-800">
            <Film className="w-8 h-8 text-zinc-600" />
          </div>
        )}

        {/* Bottom gradient */}
        <div className="absolute inset-0 bg-gradient-to-t from-black/80 via-black/10 to-transparent pointer-events-none" />

        {/* Active pulse ring */}
        {isActive && (
          <div className="absolute inset-0 rounded-xl ring-2 ring-violet-400/60 animate-pulse pointer-events-none" />
        )}

        {/* Loading overlay */}
        {isPending && (
          <div className="absolute inset-0 flex items-center justify-center bg-black/40">
            <Loader2 className="w-6 h-6 text-violet-400 animate-spin" />
          </div>
        )}

        {/* Label */}
        <div className="absolute bottom-0 left-0 right-0 px-2 py-2 pointer-events-none">
          <p className="text-xs font-semibold text-white truncate leading-tight drop-shadow">
            {media.title ?? media.filename}
          </p>
          {isActive && (
            <span className="text-[10px] text-violet-300 font-bold tracking-wide">● LIVE</span>
          )}
        </div>
      </button>

      {/* Quick actions (long press) */}
      {showActions && overlay && (
        <div
          className="absolute inset-0 rounded-xl bg-black/85 backdrop-blur-sm flex flex-col items-center justify-center gap-2 z-10 animate-fade-in"
          onClick={closeActions}
        >
          <button
            onClick={(e) => {
              e.stopPropagation()
              onToggleFullscreen(overlay)
              setShowActions(false)
            }}
            className="flex items-center gap-2 px-4 py-2.5 bg-zinc-700 hover:bg-zinc-600 active:scale-95 rounded-lg text-sm text-white transition-all w-36 justify-center"
          >
            {overlay.fullscreen
              ? <><Minimize2 className="w-4 h-4" /> Windowed</>
              : <><Maximize2 className="w-4 h-4" /> Fullscreen</>}
          </button>
          <button
            onClick={(e) => {
              e.stopPropagation()
              onClose(overlay.id)
              setShowActions(false)
            }}
            className="flex items-center gap-2 px-4 py-2.5 bg-red-700/80 hover:bg-red-600 active:scale-95 rounded-lg text-sm text-white transition-all w-36 justify-center"
          >
            <X className="w-4 h-4" /> Close
          </button>
          <button
            onClick={closeActions}
            className="text-xs text-zinc-500 hover:text-zinc-300 mt-1 transition-colors"
          >
            Cancel
          </button>
        </div>
      )}
    </div>
  )
})
