const STORAGE_KEY = 'keel-theme';
const THEMES = {
	DARK: 'dark',
	LIGHT: 'light',
};

const nextTheme = (theme) => (theme === THEMES.LIGHT ? THEMES.DARK : THEMES.LIGHT);

const getDocumentTheme = () => {
	const theme = document.documentElement.getAttribute('data-theme');
	return theme === THEMES.LIGHT ? THEMES.LIGHT : THEMES.DARK;
};

const setDocumentTheme = (theme) => {
	document.documentElement.setAttribute('data-theme', theme);
};

const setTogglePresentation = (button, theme) => {
	const icon = button.querySelector('[data-theme-toggle-icon]');
	const label = theme === THEMES.DARK ? 'Switch to light mode' : 'Switch to dark mode';
	button.setAttribute('aria-label', label);
	button.setAttribute('title', label);

	if (icon) {
		icon.textContent = theme === THEMES.DARK ? '☀' : '☾';
	}
};

const persistThemeForUser = async (theme) => {
	const authMeta = document.querySelector('meta[name="keel-authenticated"]');
	if (!authMeta || authMeta.getAttribute('content') !== '1') {
		return;
	}

	const csrfToken = document
		.querySelector('meta[name="csrf-token"]')
		?.getAttribute('content');

	if (!csrfToken) {
		return;
	}

	try {
		await fetch('/settings/theme', {
			method: 'POST',
			headers: {
				'Content-Type': 'application/json',
				'Accept': 'application/json',
				'X-CSRF-Token': csrfToken,
			},
			body: JSON.stringify({ theme }),
		});
	} catch {
		// Keep local preference even if server persistence temporarily fails.
	}
};

const ensureFallbackToggle = () => {
	if (document.querySelector('[data-theme-toggle]') || !document.body) {
		return;
	}

	const button = document.createElement('button');
	button.type = 'button';
	button.className = 'theme-toggle-button theme-toggle-fallback';
	button.setAttribute('data-theme-toggle', '');
	button.innerHTML = '<span data-theme-toggle-icon aria-hidden="true"></span>';
	document.body.appendChild(button);
};

export const initThemeToggle = () => {
	ensureFallbackToggle();

	const toggles = Array.from(document.querySelectorAll('[data-theme-toggle]'));
	if (toggles.length === 0) {
		return;
	}

	const syncToggles = () => {
		const theme = getDocumentTheme();
		toggles.forEach((toggle) => setTogglePresentation(toggle, theme));
	};

	toggles.forEach((toggle) => {
		toggle.addEventListener('click', async () => {
			const updatedTheme = nextTheme(getDocumentTheme());
			setDocumentTheme(updatedTheme);
			try {
				localStorage.setItem(STORAGE_KEY, updatedTheme);
			} catch {
				// Ignore storage failures.
			}
			syncToggles();
			await persistThemeForUser(updatedTheme);
		});
	});

	syncToggles();
};
