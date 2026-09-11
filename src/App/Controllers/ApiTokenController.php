<?php

namespace Keel\App\Controllers;

use Keel\App\Services\ApiTokenService;
use Keel\Core\Auth;
use Keel\Core\Controller;
use Keel\Core\Request;

class ApiTokenController extends Controller
{
    public function index(Request $request): void
    {
        $userId = $this->currentUserId();
        $this->view('settings.api-tokens', [
            'title' => 'API Tokens',
            'tokens' => (new ApiTokenService())->listTokens($userId),
            'newToken' => null,
            'error' => null,
        ]);
    }

    public function store(Request $request): void
    {
        $userId = $this->currentUserId();
        $name = trim((string) $request->input('name'));
        $abilities = trim((string) $request->input('abilities', '*'));
        $expiresAtRaw = trim((string) $request->input('expires_at', ''));

        if ($name === '') {
            $this->view('settings.api-tokens', [
                'title' => 'API Tokens',
                'tokens' => (new ApiTokenService())->listTokens($userId),
                'newToken' => null,
                'error' => 'Token name is required.',
            ]);
            return;
        }

        if ($abilities === '') {
            $abilities = '*';
        }

        $expiresAt = null;
        if ($expiresAtRaw !== '') {
            try {
                $expiresAt = new \DateTimeImmutable($expiresAtRaw);
            } catch (\Exception $exception) {
                $this->view('settings.api-tokens', [
                    'title' => 'API Tokens',
                    'tokens' => (new ApiTokenService())->listTokens($userId),
                    'newToken' => null,
                    'error' => 'Invalid expiration date/time.',
                ]);
                return;
            }
        }

        $result = (new ApiTokenService())->create($userId, $name, $abilities, $expiresAt);

        $this->view('settings.api-tokens', [
            'title' => 'API Tokens',
            'tokens' => (new ApiTokenService())->listTokens($userId),
            'newToken' => $result['token'],
            'error' => null,
        ]);
    }

    public function destroy(Request $request, string $id): void
    {
        $userId = $this->currentUserId();
        (new ApiTokenService())->revoke((int) $id, $userId);
        $this->redirect('/settings/api-tokens');
    }

    private function currentUserId(): int
    {
        $userId = Auth::id();

        if ($userId === null) {
            throw new \RuntimeException('Authenticated user could not be resolved.');
        }

        return $userId;
    }
}
