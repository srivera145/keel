const splitClasses = (value) =>
	(value || '')
		.split(' ')
		.map((item) => item.trim())
		.filter(Boolean);

const applyTabStateClasses = (trigger, activeClasses, inactiveClasses, isActive) => {
	if (isActive) {
		trigger.classList.remove(...inactiveClasses);
		trigger.classList.add(...activeClasses);
	} else {
		trigger.classList.remove(...activeClasses);
		trigger.classList.add(...inactiveClasses);
	}
};

export const initTabs = (root = document) => {
	root.querySelectorAll('[data-tabs]').forEach((container) => {
		const triggers = Array.from(container.querySelectorAll('[data-tab-target]'));
		if (triggers.length === 0) {
			return;
		}

		const panelByName = new Map();
		const scopedPanels = container.querySelectorAll('[data-tab-panel]');
		const panels = scopedPanels.length > 0
			? scopedPanels
			: root.querySelectorAll('[data-tab-panel]');

		panels.forEach((panel) => {
			panelByName.set(panel.getAttribute('data-tab-panel'), panel);
		});

		const activeClasses = splitClasses(container.getAttribute('data-tabs-active-class'));
		const inactiveClasses = splitClasses(container.getAttribute('data-tabs-inactive-class'));

		const activate = (name) => {
			triggers.forEach((trigger) => {
				const isActive = trigger.getAttribute('data-tab-target') === name;
				trigger.setAttribute('aria-selected', isActive ? 'true' : 'false');
				trigger.tabIndex = isActive ? 0 : -1;
				applyTabStateClasses(trigger, activeClasses, inactiveClasses, isActive);
			});

			panelByName.forEach((panel, panelName) => {
				panel.classList.toggle('hidden', panelName !== name);
			});
		};

		triggers.forEach((trigger, index) => {
			trigger.setAttribute('role', 'tab');
			if (!trigger.id) {
				trigger.id = `tab-${index}-${Math.random().toString(36).slice(2, 8)}`;
			}
			trigger.addEventListener('click', () => {
				activate(trigger.getAttribute('data-tab-target'));
			});
		});

		const initial = triggers.find((trigger) => trigger.getAttribute('aria-selected') === 'true') || triggers[0];
		activate(initial.getAttribute('data-tab-target'));
	});
};
