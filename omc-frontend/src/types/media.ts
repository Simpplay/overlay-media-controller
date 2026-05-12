export interface Media {
  id: number
  title?: string
  filename: string
  contentType: string
  categories: number[]
}

export interface UploadMediaResponse {
  id: number
  filename: string
  contentType: string
  status: string
}

export interface UploadMediaParams {
  file: File
  title?: string
  description?: string
}

export interface UpdateMediaParams {
  title: string
}

export interface MediaListParams {
  query?: string
  category?: string
}
