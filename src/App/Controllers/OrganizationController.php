<?php

namespace Keel\App\Controllers;

use Keel\App\Models\User;
use Keel\App\Services\OrganizationService;
use Keel\Core\Auth;
use Keel\Core\Controller;
use Keel\Core\Database;
use Keel\Core\Env;
use Keel\Core\Request;
use Keel\Core\Session;

class OrganizationController extends Controller
{
    public function showOnboarding(Request $request): void
    {
        if (!$this->enabled()) {
            $this->redirect('/dashboard');
        }

        $user = $this->currentUser();
        if (!empty($user['organization_id'])) {
            $this->redirect('/dashboard');
        }

        $this->view('onboarding.organization', ['title' => 'Create Organization']);
    }

    public function createOrganization(Request $request): void
    {
        if (!$this->enabled()) {
            $this->redirect('/dashboard');
        }

        $user = $this->currentUser();
        if (!empty($user['organization_id'])) {
            $this->redirect('/dashboard');
        }

        $name = trim((string) $request->input('name'));
        if ($name === '') {
            $this->redirect('/onboarding/organization?error=missing_name');
        }

        (new OrganizationService())->create($name, (int) $user['id']);
        $this->redirect('/dashboard');
    }

    public function showSettings(Request $request): void
    {
        $user = $this->currentUser();

        $this->view('settings.organization', [
            'title' => 'Organization Settings',
            'organization' => $this->organization((int) $user['organization_id']),
            'user' => $user,
        ]);
    }

    public function showMembers(Request $request): void
    {
        $user = $this->currentUser();
        $service = new OrganizationService();

        $this->view('settings.members', [
            'title' => 'Organization Members',
            'organization' => $this->organization((int) $user['organization_id']),
            'members' => $service->members((int) $user['organization_id']),
            'pendingInvites' => $service->pendingInvites((int) $user['organization_id']),
        ]);
    }

    public function sendInvite(Request $request): void
    {
        $user = $this->currentUser();

        try {
            (new OrganizationService())->invite(
                (int) $user['organization_id'],
                (string) $request->input('email'),
                (string) $request->input('role', 'member'),
                (int) $user['id']
            );
        } catch (\RuntimeException $exception) {
            $this->redirect('/settings/members?error=' . urlencode($exception->getMessage()));
        }

        $this->redirect('/settings/members?status=invite_sent');
    }

    public function acceptInvite(Request $request): void
    {
        if (!$this->enabled()) {
            $this->redirect('/login');
        }

        try {
            $result = (new OrganizationService())->acceptInvite(
                (string) $request->input('email'),
                (string) $request->input('token')
            );
            $this->loginUser($result['user']);
        } catch (\RuntimeException $exception) {
            $this->redirect('/login?error=invalid_invite');
        }

        $this->redirect('/dashboard');
    }

    private function currentUser(): array
    {
        $user = User::find((int) Auth::id());

        if (!$user) {
            throw new \RuntimeException('Authenticated user could not be loaded.');
        }

        return $user;
    }

    private function organization(int $organizationId): ?array
    {
        $statement = Database::connection()->prepare('SELECT * FROM organizations WHERE id = ? LIMIT 1');
        $statement->execute([$organizationId]);

        return $statement->fetch() ?: null;
    }

    private function enabled(): bool
    {
        return (bool) Env::get('MULTI_TENANCY_ENABLED', false);
    }

    private function loginUser(array $user): void
    {
        Session::regenerate();
        Session::put('user_id', $user['id']);
        Session::put('user_email', $user['email']);
        Session::put('organization_id', $user['organization_id'] ?? null);
    }
}