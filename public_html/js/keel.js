/* Keel's glue for pages styled with Deck.
 *
 * Deck's [data-deck-theme] button and Deck.theme() save the choice to
 * localStorage only. A signed-in Keel user's theme also lives on the server
 * (users.theme_preference, POST /settings/theme), so it follows them to other
 * devices and to pages that are still on Tailwind. Deck fires no event when the
 * theme changes, so this watches the attribute Deck writes.
 */
(() => {
	const root = document.documentElement;
	const authenticated = document.querySelector('meta[name="keel-authenticated"]')?.getAttribute('content') === '1';
	const csrfToken = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content');

	if (!authenticated || !csrfToken) {
		return;
	}

	new MutationObserver((records) => {
		const theme = root.getAttribute('data-theme');

		// deck.js re-applies the saved theme on DOMContentLoaded; an unchanged value is not a choice.
		if (records[0].oldValue === theme || (theme !== 'light' && theme !== 'dark')) {
			return;
		}

		fetch('/settings/theme', {
			method: 'POST',
			headers: {
				'Content-Type': 'application/json',
				'Accept': 'application/json',
				'X-CSRF-Token': csrfToken,
			},
			body: JSON.stringify({ theme }),
		}).catch(() => {
			// Keep the local choice even if saving it fails.
		});
	}).observe(root, { attributes: true, attributeFilter: ['data-theme'], attributeOldValue: true });
})();
