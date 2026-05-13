import { forwardRef, type InputHTMLAttributes } from 'react'

interface InputProps extends InputHTMLAttributes<HTMLInputElement> {
  label?: string
  error?: string
}

export const Input = forwardRef<HTMLInputElement, InputProps>(
  ({ label, error, className = '', ...props }, ref) => {
    return (
      <div className="flex flex-col gap-1.5">
        {label && (
          <label className="text-sm font-medium text-zinc-300">{label}</label>
        )}
        <input
          ref={ref}
          className={[
            'w-full px-3 py-2 rounded-lg bg-zinc-900 border text-sm text-zinc-100 placeholder:text-zinc-600',
            'focus:outline-none focus:ring-2 focus:ring-violet-500 focus:border-transparent',
            'transition-colors duration-150',
            error ? 'border-red-500' : 'border-zinc-700 hover:border-zinc-600',
            className,
          ].join(' ')}
          {...props}
        />
        {error && <p className="text-xs text-red-400">{error}</p>}
      </div>
    )
  }
)

Input.displayName = 'Input'
