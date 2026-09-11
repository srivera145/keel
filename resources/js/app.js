import '../css/app.css';
import { initModal } from './components/modal';
import { initTabs } from './components/tabs';
import { initThemeToggle } from './components/theme-toggle';

const BACK_TO_TOP_ID = 'keel-back-to-top';
const BACK_TO_TOP_THRESHOLD = 320;

const shouldReduceMotion = () =>
	window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches;

const createBackToTopButton = () => {
	if (!document.body || document.getElementById(BACK_TO_TOP_ID)) {
		return null;
	}

	const button = document.createElement('button');
	button.id = BACK_TO_TOP_ID;
	button.type = 'button';
	button.className = 'keel-back-to-top';
	button.setAttribute('aria-label', 'Back to top');
	button.setAttribute('title', 'Back to top');
	button.setAttribute('aria-hidden', 'true');
	button.tabIndex = -1;
	button.innerHTML = [
		'<svg viewBox="0 0 24 24" role="presentation" aria-hidden="true" focusable="false">',
		'<path class="keel-chevron" d="M6 14l6-6 6 6" />',
		'<path class="keel-stem" d="M12 9v8" />',
		'</svg>',
	].join('');

	button.addEventListener('click', () => {
		window.scrollTo({
			top: 0,
			left: 0,
			behavior: shouldReduceMotion() ? 'auto' : 'smooth',
		});
	});

	document.body.appendChild(button);
	return button;
};

const mountBackToTop = () => {
	const button = createBackToTopButton();
	if (!button) {
		return;
	}

	const updateVisibility = () => {
		const isVisible = window.scrollY > BACK_TO_TOP_THRESHOLD;
		button.classList.toggle('is-visible', isVisible);
		button.setAttribute('aria-hidden', isVisible ? 'false' : 'true');
		button.tabIndex = isVisible ? 0 : -1;
	};

	window.addEventListener('scroll', updateVisibility, { passive: true });
	window.addEventListener('resize', updateVisibility, { passive: true });
	updateVisibility();
};

const mountCopyButtons = () => {
	const copyTextToClipboard = async (text) => {
		if (navigator.clipboard && window.isSecureContext) {
			await navigator.clipboard.writeText(text);
			return;
		}

		const textarea = document.createElement('textarea');
		textarea.value = text;
		textarea.setAttribute('readonly', '');
		textarea.style.position = 'fixed';
		textarea.style.left = '-9999px';
		document.body.appendChild(textarea);
		textarea.focus();
		textarea.select();

		const copied = document.execCommand('copy');
		document.body.removeChild(textarea);

		if (!copied) {
			throw new Error('Clipboard copy command failed.');
		}
	};

	document.querySelectorAll('[data-copy-source]').forEach((button) => {
		button.addEventListener('click', async () => {
			const sourceId = button.getAttribute('data-copy-source');
			if (!sourceId) {
				return;
			}

			const source = document.getElementById(sourceId);
			if (!source) {
				return;
			}

			const normalLabel = button.getAttribute('data-label-default') || 'Copy';
			const successLabel = button.getAttribute('data-label-success') || 'Copied!';
			const errorLabel = button.getAttribute('data-label-error') || 'Copy failed';

			try {
				await copyTextToClipboard(source.textContent.trim());
				button.textContent = successLabel;
			} catch {
				button.textContent = errorLabel;
			}

			window.setTimeout(() => {
				button.textContent = normalLabel;
			}, 2000);
		});
	});
};

const initSharedUi = () => {
	initThemeToggle();
	initTabs();
	initModal();
	mountCopyButtons();
};

if (document.readyState === 'loading') {
	document.addEventListener(
		'DOMContentLoaded',
		() => {
			mountBackToTop();
			initSharedUi();
		},
		{ once: true }
	);
} else {
	mountBackToTop();
	initSharedUi();
}
