export interface OverlayPosition {
  x: number
  y: number
}

export interface OverlaySize {
  width: number
  height: number
}

export interface Overlay {
  id: number
  media_id: number
  state: string
  fullscreen: boolean
  position: OverlayPosition
  size: OverlaySize
}

export interface CreateOverlayResponse {
  id: number
  media_id: number
  state: string
}

export interface UpdateOverlayResponse {
  id: number
  state: string
}

export interface CreateOverlayPayload {
  media_id: number
  fullscreen?: boolean
  position?: OverlayPosition
  size?: OverlaySize
}

export interface UpdateOverlayPayload {
  fullscreen?: boolean
  position?: OverlayPosition
  size?: OverlaySize
}
