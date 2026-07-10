/** @type {import('tailwindcss').Config} */
export default {
  content: [
    './views/**/*.php',
    './resources/js/**/*.js',
  ],
  theme: {
    extend: {
      colors: {
        brand: {
          DEFAULT: 'var(--color-brand)',
          hover: 'var(--color-brand-hover)',
          contrast: 'var(--color-brand-contrast)',
        },
        surface: {
          DEFAULT: 'var(--color-surface)',
          hover: 'var(--color-surface-hover)',
          muted: 'var(--color-surface-muted)',
        },
        text: {
          DEFAULT: 'var(--color-text)',
          muted: 'var(--color-text-muted)',
          strong: 'var(--color-text-strong)',
        },
        danger: {
          DEFAULT: 'var(--color-danger)',
          hover: 'var(--color-danger-hover)',
          soft: 'var(--color-danger-soft)',
          border: 'var(--color-danger-border)',
          text: 'var(--color-danger-text)',
        },
        success: {
          DEFAULT: 'var(--color-success)',
          soft: 'var(--color-success-soft)',
          border: 'var(--color-success-border)',
          text: 'var(--color-success-text)',
        },
        card: {
          DEFAULT: 'var(--color-card-bg)',
          border: 'var(--color-card-border)',
        },
        input: {
          border: 'var(--color-input-border)',
        },
        table: {
          head: 'var(--color-table-head)',
          'row-alt': 'var(--color-table-row-alt)',
          'row-hover': 'var(--color-table-row-hover)',
        },
        empty: {
          bg: 'var(--color-empty-state-bg)',
          border: 'var(--color-empty-state-border)',
          icon: 'var(--color-empty-state-icon-bg)',
          'icon-text': 'var(--color-empty-state-icon-text)',
        },
        modal: {
          backdrop: 'var(--color-modal-backdrop)',
        },
      },
      borderRadius: {
        md: 'var(--radius-md)',
        lg: 'var(--radius-lg)',
        xl: 'var(--radius-xl)',
      },
    },
  },
  plugins: [],
};
