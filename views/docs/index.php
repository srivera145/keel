<?php
$docsGroups = $docsGroups ?? [];
?>
<div class="stack stack-6">
    <p>
        This docs section mirrors what is currently implemented in the Keel codebase. Each page maps directly to real classes,
        routes, environment variables, migrations, and CLI scripts.
    </p>

    <div class="grid">
        <?php foreach ($docsGroups as $group): ?>
        <section class="card">
            <div class="card-body">
                <h2 class="card-title"><?= htmlspecialchars((string) ($group['title'] ?? 'Docs')) ?></h2>
                <ul class="stack stack-2">
                    <?php foreach (($group['links'] ?? []) as $link): ?>
                    <li><a href="<?= htmlspecialchars((string) ($link['href'] ?? '/docs')) ?>"><?= htmlspecialchars((string) ($link['label'] ?? 'Untitled')) ?></a></li>
                    <?php endforeach; ?>
                </ul>
            </div>
        </section>
        <?php endforeach; ?>
    </div>

    <p>
        If a feature is not present in the codebase, it is intentionally not documented here.
    </p>
</div>
