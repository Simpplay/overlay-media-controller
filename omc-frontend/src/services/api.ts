import axios from 'axios'
import toast from 'react-hot-toast'

const api = axios.create({
  baseURL: '/api',
  timeout: 15000,
})

api.interceptors.response.use(
  (response) => response,
  (error) => {
    const message =
      error.response?.data?.error ??
      error.response?.data?.message ??
      error.message ??
      'An unexpected error occurred'
    toast.error(message)
    return Promise.reject(error)
  }
)

export default api
