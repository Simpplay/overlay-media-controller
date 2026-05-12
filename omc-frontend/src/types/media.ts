export interface Media {
  id: number
  title?: string
  filename: string
  contentType: string
  categories: string[]
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

export interface MediaListParams {
  query?: string
  category?: string
}
