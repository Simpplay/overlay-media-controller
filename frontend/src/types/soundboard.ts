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

export interface SoundboardDevice {
  device_id: number,
  name: string,
  selected: boolean
}

export interface SetSoundboardPayload {
  device_id: number
}