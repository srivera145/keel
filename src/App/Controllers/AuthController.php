<?php

namespace Keel\App\Controllers;

use Keel\App\Services\MagicLinkService;
use Keel\App\Services\OtpService;
use Keel\Core\Activity;
use Keel\Core\Controller;
use Keel\Core\Env;
use Keel\Core\Request;
use Keel\Core\Session;

class AuthController extends Controller
{
    public function showLogin(Request $request): void
    {
        $this->view('auth.login', ['authMethod' => Env::get('AUTH_METHOD', 'both')]);
    }

    public function requestOtp(Request $request): void
    {
        $email = filter_var($request->input('email'), FILTER_VALIDATE_EMAIL);
        if (!$email) {
            $this->json(['success' => false, 'message' => 'Enter a valid email.'], 422);
        }

        $result = (new OtpService())->requestCode($email);
        $this->json($result, $result['success'] ? 200 : 422);
    }

    public function verifyOtp(Request $request): void
    {
        $email = filter_var($request->input('email'), FILTER_VALIDATE_EMAIL);
        $code = trim((string) $request->input('code'));

        if (!$email || !$code) {
            $this->json(['success' => false, 'message' => 'Email and code are required.'], 422);
        }

        $result = (new OtpService())->verifyCode($email, $code);

        if ($result['success']) {
            $this->loginUser($result['user']);
            $this->json(['success' => true, 'redirect' => $this->postLoginRedirect($result['user'])]);
        }

        $this->json($result, 422);
    }

    public function requestMagicLink(Request $request): void
    {
        $email = filter_var($request->input('email'), FILTER_VALIDATE_EMAIL);
        if (!$email) {
            $this->json(['success' => false, 'message' => 'Enter a valid email.'], 422);
        }

        $result = (new MagicLinkService())->sendLink($email);
        $this->json($result, $result['success'] ? 200 : 422);
    }

    public function verifyMagicLink(Request $request): void
    {
        $email = filter_var($request->input('email'), FILTER_VALIDATE_EMAIL);
        $token = trim((string) $request->input('token'));

        if (!$email || !$token) {
            $this->redirect('/login?error=invalid_link');
        }

        $result = (new MagicLinkService())->verifyToken($email, $token);

        if ($result['success']) {
            $this->loginUser($result['user']);
            $this->redirect($this->postLoginRedirect($result['user']));
        }

        $this->redirect('/login?error=invalid_link');
    }

    public function logout(Request $request): void
    {
        Activity::log('user.logout');
        Session::destroy();
        $this->redirect('/login');
    }

    private function loginUser(array $user): void
    {
        Session::regenerate();
        Session::put('user_id', $user['id']);
        Session::put('user_email', $user['email']);
        Session::put('organization_id', $user['organization_id'] ?? null);
        Session::put('theme_preference', $user['theme_preference'] ?? null);
        Activity::log('user.login');
    }

    private function postLoginRedirect(array $user): string
    {
        if ((bool) Env::get('MULTI_TENANCY_ENABLED', false) && empty($user['organization_id'])) {
            return '/onboarding/organization';
        }

        return '/dashboard';
    }
}
