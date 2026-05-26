export interface Soundboard {
  media_id: number
  volume: number
  force: boolean
}

export interface PlaySoundboardPayload {
  media_id: number
  volume: number
  force: boolean
}

export interface PlaySoundboardResponse {
  state: string
}