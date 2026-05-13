import { Toaster } from 'react-hot-toast'
import { QueryProvider } from './app/QueryProvider'
import { Router } from './app/Router'

export default function App() {
  return (
    <QueryProvider>
      <Router />
      <Toaster
        position="bottom-right"
        toastOptions={{
          style: {
            background: '#18181b',
            color: '#f4f4f5',
            border: '1px solid #27272a',
            borderRadius: '10px',
            fontSize: '14px',
          },
          success: {
            iconTheme: { primary: '#a78bfa', secondary: '#18181b' },
          },
          error: {
            iconTheme: { primary: '#f87171', secondary: '#18181b' },
          },
        }}
      />
    </QueryProvider>
  )
}
