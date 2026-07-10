<?php

namespace Keel\App\Controllers;

use Keel\App\Models\User;
use Keel\Core\Controller;
use Keel\Core\Request;
use Keel\Core\Session;
use Keel\Core\Theme;

class ThemeController extends Controller
{
    public function update(Request $request): void
    {
        $userId = (int) Session::get('user_id', 0);

        if ($userId <= 0) {
            $this->json(['success' => false, 'message' => 'Unauthenticated.'], 401);
        }

        $theme = Theme::normalize($request->input('theme'));

        if ($theme === null) {
            $this->json(['success' => false, 'message' => 'Theme must be light or dark.'], 422);
        }

        User::updateThemePreference($userId, $theme);
        Session::put('theme_preference', $theme);

        $this->json(['success' => true, 'theme' => $theme]);
    }
}
