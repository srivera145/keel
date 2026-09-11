let activeModal = null;
let activeTrigger = null;

const getPanel = (modal) => modal.querySelector('[data-modal-panel]');

const openModal = (modal, trigger = null) => {
	if (!modal) {
		return;
	}

	if (activeModal && activeModal !== modal) {
		closeModal(activeModal, false);
	}

	activeModal = modal;
	activeTrigger = trigger;
	modal.classList.add('is-open');
	modal.classList.remove('hidden');
	modal.setAttribute('aria-hidden', 'false');

	const panel = getPanel(modal);
	if (panel) {
		panel.setAttribute('tabindex', '-1');
		panel.focus();
	}
};

const closeModal = (modal, returnFocus = true) => {
	if (!modal) {
		return;
	}

	modal.classList.remove('is-open');
	modal.classList.add('hidden');
	modal.setAttribute('aria-hidden', 'true');

	if (returnFocus && activeTrigger && typeof activeTrigger.focus === 'function') {
		activeTrigger.focus();
	}

	if (activeModal === modal) {
		activeModal = null;
		activeTrigger = null;
	}
};

export const initModal = (root = document) => {
	root.querySelectorAll('[data-modal-open]').forEach((trigger) => {
		trigger.addEventListener('click', () => {
			const targetId = trigger.getAttribute('data-modal-open');
			if (!targetId) {
				return;
			}
			const modal = document.getElementById(targetId);
			openModal(modal, trigger);
		});
	});

	root.querySelectorAll('[data-modal-close]').forEach((closer) => {
		closer.addEventListener('click', () => {
			const modal = closer.closest('.modal');
			closeModal(modal);
		});
	});

	document.addEventListener('keydown', (event) => {
		if (event.key === 'Escape' && activeModal) {
			closeModal(activeModal);
		}
	});

	root.querySelectorAll('.modal').forEach((modal) => {
		modal.addEventListener('click', (event) => {
			if (event.target === modal) {
				closeModal(modal);
			}
		});
	});
};
