/* Keel's glue for pages styled with Deck.
 *
 * Deck's [data-deck-theme] button and Deck.theme() save the choice to
 * localStorage only. A signed-in Keel user's theme also lives on the server
 * (users.theme_preference, POST /settings/theme), so it follows them to their
 * other devices. Deck fires no event when the theme changes, so this watches
 * the attribute Deck writes.
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

/* Confirmation before a destructive form posts.
 *
 * Deck styles .modal as a native <dialog> and says to open it with showModal(), but ships
 * no wiring for one: drawers get data-deck-drawer in deck-extras.js, modals get nothing.
 * A form carrying data-confirm="<dialog id>" asks first when JavaScript is available. With
 * scripts off the form posts straight through, so the action still works — it just loses
 * the confirmation step.
 */
(() => {
	document.addEventListener('submit', (event) => {
		const form = event.target.closest('form[data-confirm]');

		if (!form || form.dataset.confirmed === 'true') {
			return;
		}

		const dialog = document.getElementById(form.dataset.confirm);

		if (!dialog || typeof dialog.showModal !== 'function') {
			return;
		}

		event.preventDefault();
		dialog.showModal();
	});

	document.addEventListener('click', (event) => {
		event.target.closest('[data-modal-close]')?.closest('dialog')?.close();

		const confirmer = event.target.closest('[data-confirm-submit]');
		const form = confirmer && document.getElementById(confirmer.dataset.confirmSubmit);

		if (!form) {
			return;
		}

		form.dataset.confirmed = 'true';
		confirmer.closest('dialog')?.close();
		form.requestSubmit();
	});
})();
