<?php

declare(strict_types=1);

use Keel\Core\Database;
use Keel\Core\Env;

require dirname(__DIR__) . '/vendor/autoload.php';

if (PHP_SAPI !== 'cli') {
    fwrite(STDERR, "This script must be run from the command line.\n");
    exit(1);
}

Env::load(dirname(__DIR__));

$appEnv = strtolower((string) Env::get('APP_ENV', 'production'));
if (!in_array($appEnv, ['local', 'dev', 'development', 'testing'], true)) {
    fwrite(STDERR, "Refusing to seed demo data outside local/dev/testing environments.\n");
    exit(1);
}

// Two tenants for the Klaus demo page (/claims). Running this again resets their claims to the rows below.
$organizations = [
    'acme-motors' => [
        'name' => 'Acme Motors',
        'brand_hue' => 150,
        'users' => ['owner@acme.test' => 'owner', 'member@acme.test' => 'member'],
        'claims' => [
            ['RO-10421', 'J. Alvarez', '2024 F-150', 412.50],
            ['RO-10433', 'M. Chen', '2023 Explorer', 1288.00],
            ['RO-10440', 'D. Okafor', '2025 Bronco', 96.75],
        ],
    ],
    'globex-auto' => [
        'name' => 'Globex Auto',
        'brand_hue' => 25,
        'users' => ['owner@globex.test' => 'owner'],
        'claims' => [
            ['RO-20017', 'S. Patel', '2024 Mustang Mach-E', 2310.40],
            ['RO-20025', 'L. Novak', '2022 Escape', 640.00],
        ],
    ],
];

$pdo = Database::connection();

$findOrganization = $pdo->prepare('SELECT id FROM organizations WHERE slug = ? LIMIT 1');
$insertOrganization = $pdo->prepare('INSERT INTO organizations (name, slug, brand_hue) VALUES (?, ?, ?)');
$updateOrganization = $pdo->prepare('UPDATE organizations SET name = ?, brand_hue = ? WHERE id = ?');
$upsertUser = $pdo->prepare(
    'INSERT INTO users (email, organization_id, role, created_at) VALUES (?, ?, ?, NOW())
     ON DUPLICATE KEY UPDATE organization_id = VALUES(organization_id), role = VALUES(role)'
);
$deleteClaims = $pdo->prepare('DELETE FROM claims WHERE organization_id = ?');
$insertClaim = $pdo->prepare(
    'INSERT INTO claims (organization_id, ro_number, customer, vehicle, amount) VALUES (?, ?, ?, ?, ?)'
);

foreach ($organizations as $slug => $organization) {
    $findOrganization->execute([$slug]);
    $organizationId = (int) $findOrganization->fetchColumn();

    if ($organizationId === 0) {
        $insertOrganization->execute([$organization['name'], $slug, $organization['brand_hue']]);
        $organizationId = (int) $pdo->lastInsertId();
    } else {
        $updateOrganization->execute([$organization['name'], $organization['brand_hue'], $organizationId]);
    }

    foreach ($organization['users'] as $email => $role) {
        $upsertUser->execute([$email, $organizationId, $role]);
    }

    $deleteClaims->execute([$organizationId]);
    foreach ($organization['claims'] as [$roNumber, $customer, $vehicle, $amount]) {
        $insertClaim->execute([$organizationId, $roNumber, $customer, $vehicle, $amount]);
    }

    fwrite(STDOUT, sprintf(
        "%s (organization #%d): %s, %d claims\n",
        $organization['name'],
        $organizationId,
        implode(', ', array_map(static fn (string $email, string $role): string => "{$email} ({$role})", array_keys($organization['users']), $organization['users'])),
        count($organization['claims'])
    ));
}

fwrite(STDOUT, "Sign in at /login as any of these users (MAIL_MAILER=log puts the code in storage/logs/mail.log), then open /claims.\n");
