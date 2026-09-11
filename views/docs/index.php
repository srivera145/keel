<?php
$docsGroups = $docsGroups ?? [];
?>
<div class="space-y-5 text-sm leading-7 text-[var(--color-text-muted)]">
    <p>
        This docs section mirrors what is currently implemented in the Keel codebase. Each page maps directly to real classes,
        routes, environment variables, migrations, and CLI scripts.
    </p>

    <div class="grid gap-4 md:grid-cols-2">
        <?php foreach ($docsGroups as $group): ?>
        <section class="card rounded-2xl p-4">
            <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]"><?= htmlspecialchars((string) ($group['title'] ?? 'Docs')) ?></h2>
            <ul class="list-disc space-y-1 pl-5">
                <?php foreach (($group['links'] ?? []) as $link): ?>
                <li><a class="text-[var(--color-brand)] hover:underline" href="<?= htmlspecialchars((string) ($link['href'] ?? '/docs')) ?>"><?= htmlspecialchars((string) ($link['label'] ?? 'Untitled')) ?></a></li>
                <?php endforeach; ?>
            </ul>
        </section>
        <?php endforeach; ?>
    </div>

    <p>
        If a feature is not present in the codebase, it is intentionally not documented here.
    </p>
</div>
